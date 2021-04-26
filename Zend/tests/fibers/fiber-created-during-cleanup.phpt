--TEST--
Fibers created during cleanup
--FILE--
<?php

$fibers = [];
for ($i = 0; $i < 5; $i++) {
    $fibers[$i] = new Fiber(function() {
        try {
            Fiber::yield();
        } finally {
            echo "finally\n";
            $fiber2 = new Fiber(function() {
                echo "new\n";
                try {
                    Fiber::yield();
                } finally {
                    echo "new finally\n";
                }
            });
            $fiber2->run();
        }
    });
    $fibers[$i]->run();
}

?>
--EXPECT--
finally
new
new finally
finally
new
new finally
finally
new
new finally
finally
new
new finally
finally
new
new finally
