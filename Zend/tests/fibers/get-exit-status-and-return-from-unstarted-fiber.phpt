--TEST--
Fiber::getExitStatus() and Fiber::getReturn() from unstarted fiber
--FILE--
<?php

$fiber = new Fiber(fn() => Fiber::yield());
try {
    $fiber->getExitStatus();
} catch (FiberError $error) {
    var_dump($error->getMessage());
}
try {
    $fiber->getReturn();
} catch (FiberError $error) {
    var_dump($error->getMessage());
}

?>
--EXPECTF--
string(29) "Fiber has never been executed"
string(29) "Fiber has never been executed"
