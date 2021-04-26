/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 3c658e17a310ad5005e0bee24df677f799acd038 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Fiber___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, callback, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_run, 0, 0, IS_MIXED, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_resume, 0, 0, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, data, IS_MIXED, 0, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_Fiber_yield arginfo_class_Fiber_resume

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_throw, 0, 1, IS_MIXED, 0)
	ZEND_ARG_OBJ_INFO(0, exception, Throwable, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_Fiber_getId, 0, 0, MAY_BE_LONG|MAY_BE_STRING)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_getStatus, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_getCurrent, 0, 0, IS_STATIC, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Fiber_getExitStatus arginfo_class_Fiber_getStatus

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_getReturn, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_getExecutingFile, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Fiber_getExecutingLine arginfo_class_Fiber_getStatus

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_getTrace, 0, 0, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_LONG, 0, "DEBUG_BACKTRACE_PROVIDE_OBJECT")
ZEND_END_ARG_INFO()


ZEND_METHOD(Fiber, __construct);
ZEND_METHOD(Fiber, run);
ZEND_METHOD(Fiber, resume);
ZEND_METHOD(Fiber, yield);
ZEND_METHOD(Fiber, throw);
ZEND_METHOD(Fiber, getId);
ZEND_METHOD(Fiber, getStatus);
ZEND_METHOD(Fiber, getCurrent);
ZEND_METHOD(Fiber, getExitStatus);
ZEND_METHOD(Fiber, getReturn);
ZEND_METHOD(Fiber, getExecutingFile);
ZEND_METHOD(Fiber, getExecutingLine);
ZEND_METHOD(Fiber, getTrace);


static const zend_function_entry class_Fiber_methods[] = {
	ZEND_ME(Fiber, __construct, arginfo_class_Fiber___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, run, arginfo_class_Fiber_run, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, resume, arginfo_class_Fiber_resume, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, yield, arginfo_class_Fiber_yield, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Fiber, throw, arginfo_class_Fiber_throw, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, getId, arginfo_class_Fiber_getId, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, getStatus, arginfo_class_Fiber_getStatus, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, getCurrent, arginfo_class_Fiber_getCurrent, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Fiber, getExitStatus, arginfo_class_Fiber_getExitStatus, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, getReturn, arginfo_class_Fiber_getReturn, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, getExecutingFile, arginfo_class_Fiber_getExecutingFile, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, getExecutingLine, arginfo_class_Fiber_getExecutingLine, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, getTrace, arginfo_class_Fiber_getTrace, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


static const zend_function_entry class_FiberError_methods[] = {
	ZEND_FE_END
};


static const zend_function_entry class_FiberException_methods[] = {
	ZEND_FE_END
};

static zend_class_entry *register_class_Fiber(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "Fiber", class_Fiber_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);

	return class_entry;
}

static zend_class_entry *register_class_FiberError(zend_class_entry *class_entry_Error)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "FiberError", class_FiberError_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Error);

	return class_entry;
}

static zend_class_entry *register_class_FiberException(zend_class_entry *class_entry_Exception)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "FiberException", class_FiberException_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Exception);

	return class_entry;
}
