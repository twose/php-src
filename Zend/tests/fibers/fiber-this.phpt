--TEST--
Fiber::getCurrent()
--FILE--
<?php

var_dump(Fiber::getCurrent());

$fiber = new Fiber(function () use (&$fiber): void {
    var_dump(Fiber::getCurrent());
    var_dump($fiber === Fiber::getCurrent());
});
$fiber->run();

?>
--EXPECTF--
object(Fiber)#%d (0) {
}
object(Fiber)#%d (0) {
}
bool(true)
