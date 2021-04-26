--TEST--
Resume non-running fiber
--FILE--
<?php

$fiber = new Fiber(fn() => 'OK');
var_dump($fiber->resume());
var_dump($fiber->getReturn());

?>
--EXPECT--
NULL
string(2) "OK"
