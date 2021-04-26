--TEST--
Test resume
--FILE--
<?php

$fiber = new Fiber(function (): void {
    $value = Fiber::yield(1);
    var_dump($value);
});

$value = $fiber->run();
var_dump($value);
$fiber->resume($value + 1);

?>
--EXPECT--
int(1)
int(2)
