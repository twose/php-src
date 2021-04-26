--TEST--
Pause fiber in destruct
--FILE--
<?php

$fiber = new Fiber(function (): int {
    $object = new class() {
        public function __destruct()
        {
            Fiber::yield(2);
        }
    };

    Fiber::yield(1);

    unset($object);

    Fiber::yield(3);

    return 4;
});

var_dump($fiber->run());
var_dump($fiber->resume());
var_dump($fiber->resume());
var_dump($fiber->resume());
var_dump($fiber->getStatus() === Fiber::STATUS_DEAD);
var_dump($fiber->getReturn());

?>
--EXPECT--
int(1)
int(2)
int(3)
NULL
bool(true)
int(4)
