--TEST--
Silence operator does not leak out of fiber
--FILE--
<?php

function yield_with_warnings(): void {
    trigger_error("A", E_USER_WARNING); // Should be silenced.
    Fiber::yield();
    trigger_error("B", E_USER_WARNING); // Should be silenced.
}

$fiber = new Fiber(function (): void {
    @yield_with_warnings();
});
$fiber->run();
trigger_error("C", E_USER_WARNING);
$fiber->resume();
trigger_error("D", E_USER_WARNING);

?>
--EXPECTF--
Warning: C in %s on line %d

Warning: D in %s on line %d
