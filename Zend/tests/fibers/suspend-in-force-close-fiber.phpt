--TEST--
Suspend in force closed fiber
--FILE--
<?php

$fiber = new Fiber(function (): void {
    try {
        Fiber::yield();
    } finally {
        Fiber::yield();
    }
});
$fiber->run();
unset($fiber);

?>
--EXPECTF--
Fatal error: Uncaught FiberError: Cannot yield in a force closed fiber in %s:%d
Stack trace:
#0 %s(%d): Fiber::yield()
#1 [internal function]: {closure}()
#2 {main}
  thrown in %s on line %d
