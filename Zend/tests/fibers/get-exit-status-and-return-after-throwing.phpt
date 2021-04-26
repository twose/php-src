--TEST--
Fiber::getExitStatus() and Fiber::getReturn() after a fiber throws
--FILE--
<?php

$fiber = new Fiber(fn() => throw new Exception('test'));
try {
    $fiber->run();
} catch (Exception $exception) {
    // never here, no way
    echo $exception->getMessage(), "\n";
}
var_dump($fiber->getExitStatus());
try {
    $fiber->getReturn();
} catch (FiberError $error) {
    var_dump($error->getMessage());
}

?>
--EXPECTF--
Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 [internal function]: {closure}()
#1 {main}
  thrown in %s on line %d
int(255)
string(35) "Fiber encountered an error and died"
