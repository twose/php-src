/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) Zend Technologies Ltd. (http://www.zend.com)           |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
*/

#include "zend_fiber.h"
#include "zend_fiber_arginfo.h"

#include "zend_llist.h"
#include "zend_builtin_functions.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "zend_observer.h"
#include "zend_weakrefs.h"

#ifdef HAVE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#ifndef _WIN32
#include <unistd.h> /* for getpagesize() */
#include <sys/mman.h>
#if !defined(MAP_ANON) && defined(MAP_ANONYMOUS)
#define MAP_ANON MAP_ANONYMOUS
#endif
#endif

/* MAP_STACK must be used in combination with MAP_ANON and MAP_PRIVATE. */
#ifndef MAP_ANON
#undef MAP_STACK
#define MAP_STACK 0
#endif

/*
 * FreeBSD require a first (i.e. addr) argument of mmap(2) is not NULL
 * if MAP_STACK is passed.
 * http://www.FreeBSD.org/cgi/query-pr.cgi?pr=158755
 */
#if !defined(MAP_STACK) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#define MAP_STACK 0
#endif

#define ZEND_FIBER_UNWIND_EXIT_MAGIC ((zend_object *) -1)

ZEND_API zend_class_entry *zend_ce_fiber;
static zend_object_handlers zend_fiber_handlers;

static zend_class_entry *zend_ce_fiber_error;
static zend_class_entry *zend_ce_fiber_exception;

/* boost.context declarations */
typedef struct
{
	void *from;
	zval *data;
} zend_fiber_transfer;

typedef void (*zend_fiber_function)(zend_fiber_transfer transfer);

void *make_fcontext(void *stack, size_t stack_size, zend_fiber_function function);
zend_fiber_transfer jump_fcontext(void const *target_fcontext, zval *transfer_data);

/* pre-declarations */
static ZEND_STACK_ALIGNED void zend_fiber_execute(zend_fiber_transfer transfer);

/* for observe (do fast check before notify it) */
extern zend_llist zend_observers_fiber_switch_callbacks;

/* TODO: Make it a public API for other modules? */
static size_t zend_fiber_getpagesize(void)
{
	static size_t page_size = 0;

	if (!page_size) {
# ifdef _WIN32
		SYSTEM_INFO system_info;
		GetSystemInfo(&system_info);
		page_size = system_info.dwPageSize;
# else
		page_size = getpagesize();
# endif
		if (!page_size || (page_size & (page_size - 1))) {
			/* anyway, we have to return a available result */
			page_size = 4096;
		}
	}

	return page_size;
}

static zend_always_inline size_t zend_fiber_zval_slots(const zend_fiber *fiber)
{
	return (ZEND_MM_ALIGNED_SIZE(sizeof(*fiber) + zend_object_properties_size(fiber->std.ce)) + ZEND_MM_ALIGNED_SIZE(sizeof(zval)) - 1) / ZEND_MM_ALIGNED_SIZE(sizeof(zval));
}

static void zend_fiber_register(zend_fiber *fiber)
{
	zval tmp;
	GC_ADDREF(&fiber->std);
	ZVAL_OBJ(&tmp, &fiber->std);
#ifdef ZEND_FIBER_USE_STRING_ID
	if (fiber->sid != NULL) {
		zend_hash_update(&EG(fibers), fiber->sid, &tmp);
		return;
	}
#endif
	zend_hash_index_update(&EG(fibers), fiber->id, &tmp);
}

static void zend_fiber_unregister(zend_fiber *fiber)
{
	zval tmp;
	ZVAL_OBJ(&tmp, &fiber->std);

#ifdef ZEND_FIBER_USE_STRING_ID
	if (fiber->sid != NULL) {
		zend_hash_del(&EG(fibers), fiber->sid, &tmp);
		return;
	}
#endif
	zend_hash_index_del(&EG(fibers), fiber->id);
}

static bool zend_fiber_make_context(zend_fiber *fiber)
{
	uint32_t stack_size = EG(fiber_stack_size);
	void *stack, *stack_top;

#ifndef ZEND_WIN32
	stack = mmap(0, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_STACK, -1, 0);
	if (stack == MAP_FAILED) {
		zend_throw_error(zend_ce_fiber_exception, "Fiber make context failed: mmap failed: %s (%d)", strerror(errno), errno);
		return false;
	}
	if (mprotect(stack, zend_fiber_getpagesize(), PROT_NONE) != 0) {
		zend_throw_error(zend_ce_fiber_exception, "Fiber protect stack failed: mmap failed: %s (%d)", strerror(errno), errno);
		munmap(stack, stack_size);
		return false;
	}
#else
	stack = VirtualAlloc(0, stack_size, MEM_COMMIT, PAGE_READWRITE);
	if (stack == NULL) {
		DWORD err = GetLastError();
		char *errmsg = php_win32_error_to_msg(err);
		zend_throw_error(zend_ce_fiber_exception, "Fiber make context failed: VirtualAlloc failed: [0x%08lx] %s", err, errmsg[0] ? errmsg : "Unknown");
		php_win32_error_msg_free(errmsg);
		return false;
	}
	DWORD old_options;
	if (!VirtualProtect(pointer, zend_fiber_getpagesize(), PAGE_READWRITE | PAGE_GUARD, &old_options)) {
		if (stack == NULL) {
			DWORD err = GetLastError();
			char *errmsg = php_win32_error_to_msg(err);
			zend_throw_error(zend_ce_fiber_exception, "Fiber protect stack failed: VirtualProtect failed: [0x%08lx] %s", err, errmsg[0] ? errmsg : "Unknown");
			php_win32_error_msg_free(errmsg);
			return false;
		}
		VirtualFree(pointer, 0, MEM_RELEASE);
		return false;
	}
#endif
	stack_top = ((char *) stack) + stack_size;

	fiber->status = ZEND_FIBER_STATUS_SUSPENDED;
	fiber->context.stack_size = stack_size;
	fiber->context.stack = stack;
	fiber->context.ptr = make_fcontext(stack_top, stack_size, zend_fiber_execute);
	ZEND_ASSERT(fiber->context.ptr != NULL && "make_fcontext() never returns NULL");
#ifdef HAVE_VALGRIND
	fiber->context.valgrind_stack_id = VALGRIND_STACK_REGISTER(stack_top, stack);
#endif

	return true;
}

static void zend_fiber_free_context(zend_fiber *fiber)
{
#ifdef HAVE_VALGRIND
	VALGRIND_STACK_DEREGISTER(fiber->context.valgrind_stack_id);
#endif
#ifndef ZEND_WIN32
	munmap(fiber->context.stack, fiber->context.stack_size);
#else
	VirtualFree(fiber->context.stack, 0, MEM_RELEASE);
#endif
}

static zend_object *zend_fiber_create_object(zend_class_entry *ce)
{
	uint32_t vm_stack_page_size = EG(fiber_stack_page_size);
	zend_vm_stack vm_stack = emalloc(vm_stack_page_size);
	zend_fiber *fiber;

	/* use vm stack memory to store fiber object */
	fiber = (zend_fiber *) ZEND_VM_STACK_ELEMENTS(vm_stack);
	memset(fiber, 0, sizeof(*fiber) - sizeof(fiber->std));
	fiber->id = -1;
	fiber->status = ZEND_FIBER_STATUS_INIT;

	zend_object_std_init(&fiber->std, ce);
	fiber->std.handlers = &zend_fiber_handlers;

	fiber->executor.vm_stack = vm_stack;
	fiber->executor.vm_stack->top = (zval *) fiber;
	fiber->executor.vm_stack->end = (zval *) (((char *) vm_stack) + vm_stack_page_size);
	fiber->executor.vm_stack->prev = NULL;
	fiber->executor.vm_stack_top = ((zval *) fiber) + zend_fiber_zval_slots(fiber);
	fiber->executor.vm_stack_end = vm_stack->end;
	fiber->executor.vm_stack_page_size = vm_stack_page_size;

	return &fiber->std;
}

static void zend_fiber_dtor_obj(zend_object *object)
{
	zend_fiber *fiber = zend_fiber_from(object);

	if (fiber->id > 0 && fiber->status == ZEND_FIBER_STATUS_SUSPENDED) {
		EG(fiber_exception) = ZEND_FIBER_UNWIND_EXIT_MAGIC;
		zend_fiber_jump(fiber, NULL, NULL);
	}
}

static void zend_fiber_free_obj(zend_object *object)
{
	zend_fiber *fiber = zend_fiber_from(object);

	if (fiber->status > ZEND_FIBER_STATUS_INIT && fiber->id < 0) {
		/* Fiber is ready but never run */
		zval_ptr_dtor(&fiber->fci.function_name);
		zend_fiber_free_context(fiber);
	}
#ifdef ZEND_FIBER_USE_STRING_ID
	if (fiber->sid != NULL) {
		/* sid can not be persistent because it's always greater than 9 */
		zend_string_release_ex(fiber->sid, 0);
	}
#endif
	zval_ptr_dtor(&fiber->retval);

	zend_object_std_dtor(&fiber->std);
}

static zend_fiber *zend_fiber_main_create_object(void)
{
	zend_fiber *fiber;
	zend_object *object;

	fiber = (zend_fiber *) EG(vm_stack_top);
	memset(fiber, 0, sizeof(*fiber) - sizeof(fiber->std));
	fiber->id = ++EG(last_fiber_id);
	fiber->status = ZEND_FIBER_STATUS_RUNNING;

	object = &fiber->std;
	GC_SET_REFCOUNT(object, 1);
	GC_TYPE_INFO(object) = GC_OBJECT;
	object->ce = zend_ce_fiber;
	object->properties = NULL;
	object->handlers = &zend_fiber_handlers;

	EG(vm_stack_top) += zend_fiber_zval_slots(fiber);

	zend_fiber_register(fiber);
	EG(main_fiber) = fiber;

	return fiber;
}

static void zend_fiber_main_free_obj(zend_fiber *fiber)
{
	zend_object *object = &fiber->std;

	ZEND_ASSERT(fiber == EG(main_fiber));
	ZEND_ASSERT(GC_REFCOUNT(object) == 2);

	zend_fiber_unregister(fiber);
	GC_DELREF(object);

	/* may be created by get_properties() */
	if (object->properties && GC_DELREF(object->properties) == 0) {
		zend_array_destroy(object->properties);
	}

	if (UNEXPECTED(GC_FLAGS(object) & IS_OBJ_WEAKLY_REFERENCED)) {
		zend_weakrefs_notify(object);
	}
}

static bool zend_fiber_is_alive(const zend_fiber *fiber)
{
	return fiber->status > ZEND_FIBER_STATUS_INIT && fiber->status < ZEND_FIBER_STATUS_DEAD;
}

static ZEND_COLD void zend_fiber_status_error(const zend_fiber *fiber)
{
	switch (fiber->status) {
		case ZEND_FIBER_STATUS_RUNNING:
			zend_throw_error(zend_ce_fiber_error, "Fiber is running");
			break;
		case ZEND_FIBER_STATUS_INIT:
			zend_throw_error(zend_ce_fiber_error, "Fiber is not initialized, constructor was not called");
			break;
		case ZEND_FIBER_STATUS_DEAD:
			zend_throw_error(zend_ce_fiber_error, "Fiber is dead");
			break;
		default:
			ZEND_UNREACHABLE();
			break;
	}
}

/* Keep same with zend_execute_scripts() */
static ZEND_COLD void zend_fiber_handle_exception(void)
{
	zend_exception_restore();
	if (UNEXPECTED(EG(exception))) {
		if (Z_TYPE(EG(user_exception_handler)) != IS_UNDEF) {
			zend_user_exception_handler();
		}
		if (EG(exception)) {
			zend_exception_error(EG(exception), E_ERROR);
		}
	}
}

static void zend_fiber_handle_cross_exception(void)
{
	zend_object *exception = EG(fiber_exception);

	EG(fiber_exception) = NULL;
	if (exception == ZEND_FIBER_UNWIND_EXIT_MAGIC) {
		zend_throw_unwind_exit();
	} else {
		zend_throw_exception_internal(exception);
	}
}

static ZEND_STACK_ALIGNED void zend_fiber_execute(zend_fiber_transfer transfer)
{
	static const zend_execute_data dummy_execute_data;
	zend_fiber *fiber = EG(current_fiber);

	fiber->id = ++EG(last_fiber_id);
#ifdef ZEND_FIBER_USE_STRING_ID
	if (fiber->id > ZEND_LONG_MAX) {
		fiber->sid = zend_ulong_to_str(fiber->id);
	}
#endif

	zend_fiber_register(fiber);

	/* update from context */
	fiber->from->context.ptr = transfer.from;

	/* prepare function call info */
	if (transfer.data == NULL) {
		fiber->fci.param_count = 0;
		fiber->fci.params = NULL;
		fiber->fci.named_params = NULL;
	} else if (Z_TYPE_P(transfer.data) != IS_PTR) {
		fiber->fci.param_count = 1;
		fiber->fci.params = transfer.data;
		fiber->fci.named_params = NULL;
	} else {
		ZEND_ASSERT(&fiber->fci == Z_PTR_P(transfer.data));
	}
	fiber->fci.retval = &fiber->retval;

	/* call function */
	EG(current_execute_data) = (zend_execute_data *) &dummy_execute_data;
	(void) zend_call_function(&fiber->fci, &fiber->fcc);
	EG(current_execute_data) = NULL;
	if (UNEXPECTED(EG(exception) != NULL)) {
		zend_fiber_handle_exception();
	}

	/* call __destruct() first here (prevent destructing in scheduler) */
	if (fiber->std.ce->destructor != NULL) {
		EG(current_execute_data) = (zend_execute_data *) &dummy_execute_data;
		zend_objects_destroy_object(&fiber->std);
		EG(current_execute_data) = NULL;
		if (UNEXPECTED(EG(exception) != NULL)) {
			zend_fiber_handle_exception();
		}
	}
	/* do not call __destruct() anymore  */
	GC_ADD_FLAGS(&fiber->std, IS_OBJ_DESTRUCTOR_CALLED);

	/* discard all possible resources (such as "use" variables in zend_closure) */
	EG(current_execute_data) = (zend_execute_data *) &dummy_execute_data;
	zval_ptr_dtor(&fiber->fci.function_name);
	EG(current_execute_data) = NULL;
	ZVAL_NULL(&fiber->fci.function_name);
	if (UNEXPECTED(EG(exception) != NULL)) {
		zend_fiber_handle_exception();
	}

	/* release extra vm stack pages */
	zend_vm_stack stack = EG(vm_stack);
	while (stack != fiber->executor.vm_stack) {
		zend_vm_stack prev = stack->prev;
		efree(stack);
		stack = prev;
	}

	zend_fiber_unregister(fiber);
	fiber->status = ZEND_FIBER_STATUS_DEAD;

	zend_fiber_yield(NULL, NULL);

	ZEND_UNREACHABLE();
}

ZEND_API void zend_fiber_jump(zend_fiber *fiber, zval *data, zval *retval)
{
	zend_fiber *current_fiber = EG(current_fiber);

	if (zend_observers_fiber_switch_callbacks.head != NULL) {
		zend_observer_fiber_switch_notify(current_fiber, fiber);
	}

	fiber->from = current_fiber;
	if (current_fiber->previous == fiber) {
		/* if it is yield, update current status to waiting and break the previous */
		if (current_fiber->status == ZEND_FIBER_STATUS_RUNNING) {
			/* maybe finished */
			current_fiber->status = ZEND_FIBER_STATUS_SUSPENDED;
		}
		current_fiber->previous = NULL;
	} else {
		/* it is not yield, current becomes target's previous */
		ZEND_ASSERT(fiber->previous == NULL);
		fiber->previous = current_fiber;
	}
	fiber->status = ZEND_FIBER_STATUS_RUNNING;
	EG(current_fiber) = fiber;

	/* ZendVM executor jump */
	memcpy(&current_fiber->executor, &EG(bailout), ZEND_FIBER_EXECUTOR_UNALIGNED_SIZE);
	memcpy(&EG(bailout), &fiber->executor, ZEND_FIBER_EXECUTOR_UNALIGNED_SIZE);
	// TODO: error_reporting

	/* C stack jump */
	zend_fiber_transfer transfer;
	transfer = jump_fcontext(fiber->context.ptr, data);

	/* update from context */
	fiber = current_fiber->from;
	fiber->context.ptr = transfer.from;

	/* copy return value */
	if (retval != NULL) {
		if (transfer.data == NULL) {
			ZVAL_NULL(retval);
		} else {
			ZVAL_COPY(retval, transfer.data);
		}
	}

	/* close the fiber if it is finished */
	if (UNEXPECTED(fiber->status == ZEND_FIBER_STATUS_DEAD)) {
		zend_fiber_free_context(fiber);
	} else if (UNEXPECTED(EG(fiber_exception) != NULL)) {
		zend_fiber_handle_cross_exception();
	}
}

ZEND_API bool zend_fiber_resume(zend_fiber *fiber, zval *data, zval *retval)
{
	if (fiber != EG(current_fiber)->previous) {
		if (UNEXPECTED(fiber->status != ZEND_FIBER_STATUS_SUSPENDED)) {
			zend_fiber_status_error(fiber);
			return false;
		}
	}

	zend_fiber_jump(fiber, data, retval);

	return true;
}

ZEND_API bool zend_fiber_yield(zval *data, zval *retval)
{
	zend_fiber *fiber = EG(current_fiber)->previous;

	if (UNEXPECTED(fiber == NULL)) {
		zend_throw_error(zend_ce_fiber_error, "Fiber has nowhere to go");
		return false;
	}

	zend_fiber_jump(fiber, data, retval);

	return true;
}

ZEND_METHOD(Fiber, __construct)
{
	zend_fiber *fiber = zend_fiber_fetch(ZEND_THIS);

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_FUNC(fiber->fci, fiber->fcc)
	ZEND_PARSE_PARAMETERS_END();

	if (fiber->status != ZEND_FIBER_STATUS_INIT) {
		zend_throw_error(zend_ce_fiber_error, "Fiber can only construct once");
		RETURN_THROWS();
	}

	if (!zend_fiber_make_context(fiber)) {
		RETURN_THROWS();
	}

	Z_TRY_ADDREF(fiber->fci.function_name);
}

ZEND_METHOD(Fiber, run)
{
	zend_fiber *fiber = zend_fiber_fetch(ZEND_THIS);
	zend_fcall_info *fci = &fiber->fci;
	zval data;

	ZEND_PARSE_PARAMETERS_START(0, -1)
		Z_PARAM_VARIADIC_WITH_NAMED(fci->params, fci->param_count, fci->named_params)
	ZEND_PARSE_PARAMETERS_END();

	if (fiber->id > 0) {
		zend_throw_error(zend_ce_fiber_error, "Fiber can only run once");
		RETURN_THROWS();
	}

	ZVAL_PTR(&data, fci);

	if (!zend_fiber_resume(fiber, &data, return_value)) {
		RETURN_THROWS();
	}
}

ZEND_METHOD(Fiber, resume)
{
	zend_fiber *fiber = zend_fiber_fetch(ZEND_THIS);
	zval *data = NULL;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(data);
	ZEND_PARSE_PARAMETERS_END();

	if (!zend_fiber_resume(fiber, data, return_value)) {
		RETURN_THROWS();
	}
}

ZEND_METHOD(Fiber, yield)
{
	zval *data = NULL;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(data);
	ZEND_PARSE_PARAMETERS_END();

	if (!zend_fiber_yield(data, return_value)) {
		RETURN_THROWS();
	}
}

ZEND_METHOD(Fiber, throw)
{
	zend_fiber *fiber = zend_fiber_fetch(ZEND_THIS);
	zend_object *exception;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJ_OF_CLASS(exception, zend_ce_throwable)
	ZEND_PARSE_PARAMETERS_END();

	GC_ADDREF(exception);
	EG(fiber_exception) = exception;

	if (!zend_fiber_resume(fiber, NULL, return_value)) {
		EG(fiber_exception) = NULL;
		GC_DELREF(exception);
		RETURN_THROWS();
	}
}

ZEND_METHOD(Fiber, getId)
{
	zend_fiber *fiber = zend_fiber_fetch(ZEND_THIS);

	ZEND_PARSE_PARAMETERS_NONE();

	if (fiber->id < 0) {
		zend_throw_error(zend_ce_fiber_error, "Fiber id will not be assigned before execution");
		RETURN_THROWS();
	}

#ifdef ZEND_FIBER_USE_STRING_ID
	if (fiber->sid != NULL) {
		RETURN_STR_COPY(fiber->sid);
	}
#endif
	RETURN_LONG(fiber->id);
}

ZEND_METHOD(Fiber, getStatus)
{
	zend_fiber *fiber = zend_fiber_fetch(ZEND_THIS);

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_LONG(fiber->status);
}

ZEND_METHOD(Fiber, getCurrent)
{
	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_OBJ_COPY(&EG(current_fiber)->std);
}

ZEND_METHOD(Fiber, getExitStatus)
{
	zend_fiber *fiber = zend_fiber_fetch(ZEND_THIS);
	const char *error;

	ZEND_PARSE_PARAMETERS_NONE();

	if (fiber->id < 0) {
		error = "Fiber has never been executed";
	} else if (fiber->status == ZEND_FIBER_STATUS_SUSPENDED) {
		error = "Fiber is suspended and has not exited yet";
	} else if (fiber->status == ZEND_FIBER_STATUS_RUNNING) {
		error = "Fiber is running and has not exited yet";
	} else {
		error = NULL;
	}
	if (error != NULL) {
		zend_throw_error(zend_ce_fiber_error, "%s", error);
		RETURN_THROWS();
	}

	if (fiber == EG(current_fiber)) {
		RETURN_LONG(EG(exit_status));
	} else {
		RETURN_LONG(fiber->executor.exit_status);
	}
}

ZEND_METHOD(Fiber, getReturn)
{
	zend_fiber *fiber = zend_fiber_fetch(ZEND_THIS);
	const char *error;

	ZEND_PARSE_PARAMETERS_NONE();

	if (Z_ISUNDEF(fiber->retval)) {
		if (fiber->id < 0) {
			error = "Fiber has never been executed";
		} else {
			switch (fiber->status) {
				case ZEND_FIBER_STATUS_SUSPENDED:
					error = "Fiber is suspended and has not returned yet";
					break;
				case ZEND_FIBER_STATUS_RUNNING:
					error = "Fiber is running and has not returned yet";
					break;
				case ZEND_FIBER_STATUS_DEAD:
					error = "Fiber encountered an error and died";
					break;
				default:
					error = NULL;
					break;
			}
		}
		if (error != NULL) {
			zend_throw_error(zend_ce_fiber_error, "%s", error);
			RETURN_THROWS();
		}
	}

	RETURN_COPY(&fiber->retval);
}

#define ZEND_FIBER_EXECUTE_START(fiber) do { \
	const zend_fiber *_fiber = fiber; \
	if (!zend_fiber_is_alive(_fiber)) { \
		zend_throw_error(zend_ce_fiber_error, "Fiber is not alive"); \
		RETURN_THROWS(); \
	} \
	zend_execute_data *_current_execute_data = EG(current_execute_data); \
	if (_fiber != EG(current_fiber)) { \
		EG(current_execute_data) = _fiber->executor.current_execute_data; \
	}

#define ZEND_FIBER_EXECUTE_END() \
	EG(current_execute_data) = _current_execute_data; \
} while (0)

ZEND_METHOD(Fiber, getTrace)
{
	zend_fiber *fiber = zend_fiber_fetch(ZEND_THIS);
	zend_long options = DEBUG_BACKTRACE_PROVIDE_OBJECT;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(options)
	ZEND_PARSE_PARAMETERS_END();

	ZEND_FIBER_EXECUTE_START(fiber) {
		zend_fetch_debug_backtrace(return_value, 0, options, 0);
	} ZEND_FIBER_EXECUTE_END();
}

ZEND_METHOD(Fiber, getExecutingFile)
{
	zend_fiber *fiber = zend_fiber_fetch(ZEND_THIS);
	ZEND_PARSE_PARAMETERS_NONE();

	ZEND_FIBER_EXECUTE_START(fiber) {
		RETVAL_STR_COPY(zend_get_executed_filename_ex());
	} ZEND_FIBER_EXECUTE_END();
}

ZEND_METHOD(Fiber, getExecutingLine)
{
	zend_fiber *fiber = zend_fiber_fetch(ZEND_THIS);
	ZEND_PARSE_PARAMETERS_NONE();

	ZEND_FIBER_EXECUTE_START(fiber) {
		RETVAL_LONG(zend_get_executed_lineno());
	} ZEND_FIBER_EXECUTE_END();
}

static HashTable *zend_fiber_get_gc(zend_object *object, zval **table, int *n)
{
	zend_fiber *fiber = zend_fiber_from(object);
	zend_get_gc_buffer *gc_buffer = zend_get_gc_buffer_create();

	zend_get_gc_buffer_add_zval(gc_buffer, &fiber->fci.function_name);
	zend_get_gc_buffer_add_zval(gc_buffer, &fiber->retval);
	zend_get_gc_buffer_use(gc_buffer, table, n);

	return zend_std_get_properties(object);
}

void zend_fiber_init(void)
{
	EG(current_fiber) = NULL;
	EG(last_fiber_id) = 0;
	EG(fiber_exception) = NULL;
	zend_hash_init(&EG(fibers), 8, NULL, ZVAL_PTR_DTOR, 0); // TODO: Fiber::getAll()
	EG(current_fiber) = zend_fiber_main_create_object();
}
void zend_fiber_shutdown(void)
{
	zend_fiber_main_free_obj(EG(current_fiber));
	zend_hash_destroy(&EG(fibers));
}

void zend_register_fiber_ce(void) /* {{{ */
{
	zend_ce_fiber = register_class_Fiber();
	zend_ce_fiber->create_object = zend_fiber_create_object;
	zend_ce_fiber->serialize = zend_class_serialize_deny;
	zend_ce_fiber->unserialize = zend_class_unserialize_deny;
	memcpy(&zend_fiber_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	zend_fiber_handlers.offset = (ZEND_VM_STACK_HEADER_SLOTS * sizeof(zval)) + XtOffsetOf(zend_fiber, std);
	zend_fiber_handlers.dtor_obj = zend_fiber_dtor_obj;
	zend_fiber_handlers.free_obj = zend_fiber_free_obj;
	zend_fiber_handlers.clone_obj = NULL;
	zend_fiber_handlers.get_gc = zend_fiber_get_gc;

#define ZEND_FIBER_STATUS_GEN(name, value) \
	zend_declare_class_constant_long(zend_ce_fiber, ZEND_STRL("STATUS_" #name), value);
	ZEND_FIBER_STATUS_MAP(ZEND_FIBER_STATUS_GEN)
#undef ZEND_FIBER_STATUS_GEN

	zend_ce_fiber_error = register_class_FiberError(zend_ce_error);
	zend_ce_fiber_error->create_object = zend_ce_error->create_object;

	zend_ce_fiber_exception = register_class_FiberException(zend_ce_exception);
	zend_ce_fiber_exception->create_object = zend_ce_exception->create_object;
}
