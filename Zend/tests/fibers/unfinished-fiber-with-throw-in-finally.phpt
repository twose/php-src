--TEST--
Test unfinished fiber with yield in finally
--FILE--
<?php

$fiber = new Fiber(function (): void {
    try {
        try {
            try {
                echo "fiber\n";
                echo Fiber::yield();
                echo "after await\n";
            } catch (Throwable $exception) {
                echo "inner exit exception caught!\n";
            }
        } catch (Throwable $exception) {
            echo "exit exception caught!\n";
        } finally {
            echo "inner finally\n";
            throw new Exception("finally exception");
        }
    } catch (Exception $exception) {
        echo $exception->getMessage(), "\n";
    } finally {
        echo "outer finally\n";
    }

    try {
        echo Fiber::yield();
    } catch (FiberError $exception) {
        echo $exception->getMessage(), "\n";
    }
});

$fiber->run();

unset($fiber); // Destroy fiber object, executing finally block.

echo "done\n";

?>
--EXPECT--
fiber
inner finally
finally exception
outer finally
Cannot yield in a force closed fiber
done
