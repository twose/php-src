--TEST--
Observer: Nested fibers
--SKIPIF--
<?php if (!extension_loaded('zend_test')) die('skip: zend_test extension required'); ?>
--INI--
zend_test.observer.enabled=1
zend_test.observer.fiber_switch=1
--FILE--
<?php

$fiber = new Fiber(function (): void {
    Fiber::yield();
    var_dump(1);

    $fiber = new Fiber(function (): void {
        Fiber::yield();
        var_dump(3);
        Fiber::yield();
        var_dump(5);
    });

    $fiber->run();

    Fiber::yield();
    var_dump(2);

    $fiber->resume();

    Fiber::yield();
    var_dump(4);

    $fiber->resume();
});

$fiber->run();
$fiber->resume();
$fiber->resume();
$fiber->resume();

?>
--EXPECTF--
<!-- init '%s' -->
<run fiber 2 by fiber 1>
<!-- init {closure}() -->
<yield fiber 2 to fiber 1>
<resume fiber 2 by fiber 1>
int(1)
<run fiber 3 by fiber 2>
<!-- init {closure}() -->
<yield fiber 3 to fiber 2>
<yield fiber 2 to fiber 1>
<resume fiber 2 by fiber 1>
int(2)
<resume fiber 3 by fiber 2>
int(3)
<yield fiber 3 to fiber 2>
<yield fiber 2 to fiber 1>
<resume fiber 2 by fiber 1>
int(4)
<resume fiber 3 by fiber 2>
int(5)
<exit fiber 3 to fiber 2 with code 0>
<exit fiber 2 to fiber 1 with code 0>
