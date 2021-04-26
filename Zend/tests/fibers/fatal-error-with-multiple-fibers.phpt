--TEST--
Fatal error in a fiber with other active fibers
--FILE--
<?php

$fiber1 = new Fiber(function (): void {
    try {
        Fiber::yield(1);
    } finally {
        echo "not executed";
    }
});

$fiber2 = new Fiber(function (): void {
    Fiber::yield(2);
    trigger_error("Fatal error in fiber", E_USER_ERROR);
});

var_dump($fiber1->run());
var_dump($fiber2->run());
$fiber2->resume();

?>
--EXPECTF--
int(1)
int(2)

Fatal error: Fatal error in fiber in %s on line %d
