--TEST--
Fatal error in new fiber
--FILE--
<?php

$fiber = new Fiber(function (): void {
    trigger_error("Fatal error in fiber", E_USER_ERROR);
});
$fiber->run();

?>
--EXPECTF--
Fatal error: Fatal error in fiber in %s on line %d
