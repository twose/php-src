--TEST--
Observer: Throwing fiber
--SKIPIF--
<?php if (!extension_loaded('zend_test')) die('skip: zend_test extension required'); ?>
--INI--
zend_test.observer.enabled=1
zend_test.observer.fiber_switch=1
--FILE--
<?php

$fiber = new Fiber(function (): void {
    try {
        Fiber::yield();
    } catch (Exception $exception) {
        exit(255);
    }
});

$fiber->run();
$fiber->throw(new Exception);

?>
--EXPECTF--
<!-- init '%s' -->
<run fiber 2 by fiber 1>
<!-- init {closure}() -->
<yield fiber 2 to fiber 1>
<resume fiber 2 by fiber 1>
<exit fiber 2 to fiber 1 with code 255>
