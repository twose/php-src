--TEST--
Resume terminated fiber
--FILE--
<?php

$fiber = new Fiber(fn() => null);
$fiber->run();
$fiber->resume();

?>
--EXPECTF--
Fatal error: Uncaught FiberError: Fiber is dead in %s:%d
Stack trace:
#0 %s(%d): Fiber->resume()
#1 {main}
  thrown in %s on line %d
