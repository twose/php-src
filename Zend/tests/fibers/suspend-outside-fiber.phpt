--TEST--
Suspend outside fiber
--FILE--
<?php

$value = Fiber::yield(1);

?>
--EXPECTF--
Fatal error: Uncaught FiberError: Cannot yield outside of a fiber in %s:%d
Stack trace:
#0 %s(%d): Fiber::yield(1)
#1 {main}
  thrown in %s on line %d
