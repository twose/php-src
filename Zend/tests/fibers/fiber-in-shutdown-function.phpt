--TEST--
Fiber in shutdown function
--FILE--
<?php

register_shutdown_function(function (): void {
    $fiber = new Fiber(function (): int {
        Fiber::yield(1);
        Fiber::yield(2);
        return 3;
    });

    var_dump($fiber->run());
    var_dump($fiber->resume());
    var_dump($fiber->resume());
    var_dump($fiber->getStatus() === Fiber::STATUS_DEAD);
    var_dump($fiber->getReturn());
});

?>
--EXPECT--
int(1)
int(2)
NULL
bool(true)
int(3)
