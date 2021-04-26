--TEST--
Test throwing into fiber
--FILE--
<?php

$fiber = new Fiber(function (): void {
    Fiber::yield('test');
});

$value = $fiber->run();
var_dump($value);

$fiber->throw(new Exception('test'));

?>
--EXPECTF--
string(4) "test"

Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 {main}
  thrown in %s on line %d
