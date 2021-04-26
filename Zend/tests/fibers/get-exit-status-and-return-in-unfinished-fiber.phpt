--TEST--
Fiber::getExitStatus() and Fiber::getReturn() in unfinished fiber
--FILE--
<?php

$fiber = new Fiber(fn() => Fiber::yield('foo'));
var_dump($fiber->run());
try {
    $fiber->getReturn();
} catch (FiberError $error) {
    var_dump($error->getMessage());
}
try {
    $fiber->getExitStatus();
} catch (FiberError $error) {
    var_dump($error->getMessage());
}

?>
--EXPECTF--
string(3) "foo"
string(43) "Fiber is suspended and has not returned yet"
string(41) "Fiber is suspended and has not exited yet"
