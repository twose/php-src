--TEST--
Fast finishing fiber does not leak
--FILE--
<?php

$fiber = new Fiber(fn() => 'test');
var_dump($fiber->getStatus() == Fiber::STATUS_SUSPENDED);
var_dump($fiber->run());
var_dump($fiber->getStatus() == Fiber::STATUS_DEAD);
var_dump($fiber->getReturn());

?>
--EXPECTF--
bool(true)
NULL
bool(true)
string(4) "test"
