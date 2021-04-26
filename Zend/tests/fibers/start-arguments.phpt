--TEST--
Arguments to fiber callback
--FILE--
<?php

$fiber = new Fiber(function (int $x): int {
    return $x + Fiber::yield($x);
});
$x = $fiber->run(1);
$fiber->resume(0);
var_dump($fiber->getReturn());

$fiber = new Fiber(function (int $x): int {
    return $x + Fiber::yield($x);
});
$fiber->run('test');

?>
--EXPECTF--
int(1)

Fatal error: Uncaught TypeError: {closure}(): Argument #1 ($x) must be of type int, string given in %s:%d
Stack trace:
#0 [internal function]: {closure}('test')
#1 {main}
  thrown in %s on line %d
