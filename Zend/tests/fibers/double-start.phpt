--TEST--
Start on already running fiber
--FILE--
<?php

$fiber = new Fiber(function (): void {
    Fiber::yield();
});

$fiber->run();

$fiber->run();

?>
--EXPECTF--
Fatal error: Uncaught FiberError: Fiber can only run once in %s:%d
Stack trace:
#0 %s(%d): Fiber->run()
#1 {main}
  thrown in %s on line %d
