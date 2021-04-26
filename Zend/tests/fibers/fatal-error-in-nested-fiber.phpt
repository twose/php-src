--TEST--
Fatal error within a nested fiber
--FILE--
<?php

$fiber = new Fiber(function (): void {
    $fiber = new Fiber(function (): void {
        \Fiber::yield(2);
        trigger_error("Fatal error in nested fiber", E_USER_ERROR);
    });

    var_dump($fiber->run());

    \Fiber::yield(1);

    $fiber->resume();
});

var_dump($fiber->run());

$fiber->resume();

?>
--EXPECTF--
int(2)
int(1)

Fatal error: Fatal error in nested fiber in %s on line %d
