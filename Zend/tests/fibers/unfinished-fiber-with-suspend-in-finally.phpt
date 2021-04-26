--TEST--
Test unfinished fiber with yield in finally
--FILE--
<?php

$fiber = new Fiber(function (): object {
    try {
        try {
            echo "fiber\n";
            return new \stdClass;
        } finally {
            echo "inner finally\n";
            Fiber::yield();
            echo "after await\n";
        }
    } catch (Throwable $exception) {
        echo "exit exception caught!\n";
    } finally {
        echo "outer finally\n";
    }

    echo "end of fiber should not be reached\n";
});

$fiber->run();

unset($fiber); // Destroy fiber object, executing finally block.

echo "done\n";

?>
--EXPECTF--
fiber
inner finally
outer finally
done
