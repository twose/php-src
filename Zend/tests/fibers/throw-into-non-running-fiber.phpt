--TEST--
Throw into non-running fiber
--FILE--
<?php

$fiber = new Fiber(fn() => null);

$fiber->throw(new Exception('test'));

?>
--EXPECTF--
Fatal error: Uncaught FiberError: Cannot resume a fiber that is not yielded in %s:%d
Stack trace:
#0 %s(%d): Fiber->throw(Object(Exception))
#1 {main}
  thrown in %s on line %d
