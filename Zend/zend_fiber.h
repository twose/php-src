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

#ifndef ZEND_FIBER_H
#define ZEND_FIBER_H

#include "zend.h"
#include "zend_API.h"

BEGIN_EXTERN_C()

#define ZEND_FIBER_MIN_STACK_SIZE			(128UL * 1024UL)
#define ZEND_FIBER_DEFAULT_STACK_SIZE		(2UL * 1024UL * 1024UL)

#define ZEND_FIBER_MIN_STACK_PAGE_SIZE		(4UL * 1024UL)
#define ZEND_FIBER_DEFAULT_STACK_PAGE_SIZE  (8UL * 1024UL)

#if SIZEOF_ZEND_LONG < 8
#define ZEND_FIBER_USE_STRING_ID 1
#endif

extern ZEND_API zend_class_entry *zend_ce_fiber;

typedef struct _zend_fiber zend_fiber;
typedef struct _zend_fiber_context zend_fiber_context;
typedef struct _zend_fiber_executor zend_fiber_executor;

#define ZEND_FIBER_STATUS_MAP(XX) \
	XX(INIT,	  0) \
	XX(RUNNING,   1) \
	XX(SUSPENDED, 2) \
	XX(DEAD,	  3)

typedef enum {
#define ZEND_FIBER_STATUS_GEN(name, value) ZEND_FIBER_STATUS_##name = (value),
	ZEND_FIBER_STATUS_MAP(ZEND_FIBER_STATUS_GEN)
#undef ZEND_FIBER_STATUS_GEN
} zend_fiber_status;

/* C stack context info */
struct _zend_fiber_context
{
	void *ptr;
	void *stack;
	uint32_t stack_size;
#ifdef HAVE_VALGRIND
	uint32_t valgrind_stack_id;
#endif
};

/* Zend executor globals */
struct _zend_fiber_executor
{
	JMP_BUF *bailout; /* extensions may bailout in fiber */
	zval *vm_stack_top;
	zval *vm_stack_end;
	zend_vm_stack vm_stack;
	size_t vm_stack_page_size;
	zend_execute_data *current_execute_data;
	uint32_t jit_trace_num;
	int exit_status;
};

/* If the size of it is not a multiple of the largest field,
 * subsequent fields on executor_globals may be overwritten by mistake,
 * so we define unaligned size here. */
#define ZEND_FIBER_EXECUTOR_UNALIGNED_SIZE (XtOffsetOf(zend_fiber_executor, exit_status) + sizeof(int))
ZEND_STATIC_ASSERT(
	ZEND_FIBER_EXECUTOR_UNALIGNED_SIZE ==
	XtOffsetOf(zend_executor_globals, exit_status) - XtOffsetOf(zend_executor_globals, bailout) + sizeof(int)
);

/* Fiber object */
struct _zend_fiber
{
	int64_t id;
#ifdef ZEND_FIBER_USE_STRING_ID
	zend_string *sid;
#endif
	zend_fiber_status status;
	zend_fiber *from;
	zend_fiber *previous;
	zend_fiber_context context;
	zend_fiber_executor executor;
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval retval;
	zend_object std;
};

#define zend_fiber_from(o) ((zend_fiber *)(((char *) o) - XtOffsetOf(zend_fiber, std)))
#define zend_fiber_fetch(z) zend_fiber_from(Z_OBJ_P(z))

void zend_register_fiber_ce(void);

void zend_fiber_init(void);
void zend_fiber_shutdown(void);

ZEND_API void zend_fiber_jump(zend_fiber *fiber, zval *data, zval *retval);
ZEND_API bool zend_fiber_resume(zend_fiber *fiber, zval *data, zval *retval);
ZEND_API bool zend_fiber_yield(zval *data, zval *retval);

END_EXTERN_C()

#endif
