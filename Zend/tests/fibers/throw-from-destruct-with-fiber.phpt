--TEST--
Test destructor throwing with unfinished fiber
--FILE--
<?php

new class() {
    function __destruct() {
        $fiber = new Fiber(static function() {
            Fiber::yield();
        });
        $fiber->run();
        throw new Exception;
    }
};

?>
--EXPECTF--
Fatal error: Uncaught Exception in %s:%d
Stack trace:
#0 %s(%d): class@anonymous->__destruct()
#1 {main}
  thrown in %s on line %d
