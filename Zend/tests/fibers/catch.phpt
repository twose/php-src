--TEST--
Catch exception thrown into fiber
--FILE--
<?php

$fiber = new Fiber(function () {
    try {
        Fiber::yield('test');
    } catch (Exception $exception) {
        var_dump($exception->getMessage());
    }
});
$data = $fiber->run();
$fiber->throw(new Exception($data));

?>
--EXPECT--
string(4) "test"
