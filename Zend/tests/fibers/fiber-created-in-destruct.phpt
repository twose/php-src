--TEST--
Fiber created in destructor
--FILE--
<?php

$object = new class() {
    public function __destruct()
    {
        $fiber = new Fiber(static function (): int {
            Fiber::yield(1);
            return 2;
        });
        var_dump($fiber->run());
        var_dump($fiber->resume());
        var_dump($fiber->getStatus() === Fiber::STATUS_DEAD);
        var_dump($fiber->getReturn());
    }
};

?>
--EXPECT--
int(1)
NULL
bool(true)
int(2)
