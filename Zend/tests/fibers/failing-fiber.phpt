--TEST--
Test throwing from fiber
--FILE--
<?php

$fiber = new Fiber(function (): void {
    Fiber::yield('test');
    throw new Exception('test');
});

$data = $fiber->run();
var_dump($data);

$fiber->resume($data);

?>
--EXPECTF--
string(4) "test"

Fatal error: Uncaught Exception: test in %s:%d
Stack trace:
#0 [internal function]: {closure}()
#1 {main}
  thrown in %s on line %d
