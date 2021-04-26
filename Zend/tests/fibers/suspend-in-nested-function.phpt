--TEST--
Suspend within nested function call
--FILE--
<?php

function user_yield(): int
{
    return Fiber::yield(1);
}

$fiber = new Fiber(function (): int {
    $value = user_yield();
    return Fiber::yield($value);
});

var_dump($fiber->run());
var_dump($fiber->resume(2));
var_dump($fiber->resume(3));
var_dump($fiber->getReturn());

echo "done\n";

?>
--EXPECT--
int(1)
int(2)
NULL
int(3)
done
