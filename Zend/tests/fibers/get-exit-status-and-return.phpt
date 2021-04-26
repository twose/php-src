--TEST--
Test fiber return and exit status
--FILE--
<?php

$fiber = new Fiber(function (): string {
    return Fiber::yield('x') . 'z';
});

$value = $fiber->run();
var_dump($value);
$fiber->resume($value . 'y');
var_dump($fiber->getReturn());
var_dump($fiber->getExitStatus());

?>
--EXPECT--
string(1) "x"
string(3) "xyz"
int(0)
