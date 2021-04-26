--TEST--
Silence operator does not leak into fiber
--FILE--
<?php

$fiber = @new Fiber(function (): void {
    trigger_error("A", E_USER_WARNING);
    Fiber::yield();
    trigger_error("C", E_USER_WARNING);
});
@$fiber->run();
trigger_error("B", E_USER_WARNING);
@$fiber->resume();
trigger_error("D", E_USER_WARNING);

?>
--EXPECTF--
Warning: A in %s on line %d

Warning: B in %s on line %d

Warning: C in %s on line %d

Warning: D in %s on line %d
