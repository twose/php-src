--TEST--
Fiber status methods
--FILE--
<?php

$function = function (): void {
    $fiber = Fiber::getCurrent();
    echo "Within Fiber: ";
    echo "{$fiber->getStatusName()}\n";
    Fiber::yield();
};

$fiber = new class($function) extends Fiber {
    public static array $fiberStatusMap = [];

    public function getStatusName(): string
    {
        if (!static::$fiberStatusMap) {
            $constants = (new ReflectionClass('Fiber'))->getConstants(ReflectionClassConstant::IS_PUBLIC);
            foreach ($constants as $name => $value) {
                if (str_starts_with($name, 'STATUS_')) {
                    static::$fiberStatusMap[$value] = substr($name, strlen('STATUS_'));
                }
            }
        }

        return static::$fiberStatusMap[$this->getStatus()];
    }
};

echo "Before Start: {$fiber->getStatusName()}\n";
$fiber->run();
echo "After Start: {$fiber->getStatusName()}\n";
$fiber->resume();
echo "After Resume: {$fiber->getStatusName()}\n";

?>
--EXPECT--
Before Start: SUSPENDED
Within Fiber: RUNNING
After Start: SUSPENDED
After Resume: DEAD
