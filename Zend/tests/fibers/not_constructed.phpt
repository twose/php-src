--TEST--
Fiber without construction
--FILE--
<?php

$fiber = new class (function () {
    try {
        Fiber::yield('test');
    } catch (Exception $exception) {
        var_dump($exception->getMessage());
    }
}) extends Fiber {
    public function __construct(callable $callback) {
        // do nothing
    }
};
try {
    var_dump($fiber->run());
} catch (FiberError $error) {
    var_dump($error->getMessage());
}
try {
    var_dump($fiber->resume());
} catch (FiberError $error) {
    var_dump($error->getMessage());
}
try {
    var_dump($fiber->throw(new Exception()));
} catch (FiberError $error) {
    var_dump($error->getMessage());
}
try {
    var_dump($fiber->getId());
} catch (FiberError $error) {
    var_dump($error->getMessage());
}
var_dump($fiber->getStatus() === Fiber::STATUS_INIT);
try {
    var_dump($fiber->getExitStatus());
} catch (FiberError $error) {
    var_dump($error->getMessage());
}
try {
    var_dump($fiber->getReturn());
} catch (FiberError $error) {
    var_dump($error->getMessage());
}
try {
    var_dump($fiber->getExecutingFile());
} catch (FiberError $error) {
    var_dump($error->getMessage());
}
try {
    var_dump($fiber->getExecutingLine());
} catch (FiberError $error) {
    var_dump($error->getMessage());
}
try {
    var_dump($fiber->getTrace());
} catch (FiberError $error) {
    var_dump($error->getMessage());
}

?>
--EXPECT--
string(52) "Fiber is not initialized, constructor was not called"
string(52) "Fiber is not initialized, constructor was not called"
string(52) "Fiber is not initialized, constructor was not called"
string(46) "Fiber id will not be assigned before execution"
bool(true)
string(29) "Fiber has never been executed"
string(29) "Fiber has never been executed"
string(18) "Fiber is not alive"
string(18) "Fiber is not alive"
string(18) "Fiber is not alive"
