--TEST--
Resume running fiber
--FILE--
<?php

$fiber = new Fiber(function (): void {
    $current = Fiber::getCurrent();
    $current->resume();
});

$fiber->run();

?>
--EXPECTF--
Fatal error: Uncaught FiberError: Fiber is running in %s:%d
Stack trace:
#0 %s(%d): Fiber->resume()
#1 [internal function]: {closure}()
#2 {main}
  thrown in %s on line %d
