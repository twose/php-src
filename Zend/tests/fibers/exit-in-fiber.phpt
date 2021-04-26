--TEST--
Exit only terminate the current fiber
--FILE--
<?php

$fiber = new Fiber(function (): void {
    Fiber::yield();
    echo "Resumed\n";
    exit();
});
$fiber->run();
$fiber->resume();

echo "Done\n";

?>
--EXPECT--
Resumed
Done
