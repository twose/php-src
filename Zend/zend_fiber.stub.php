<?php

/** @generate-class-entries */

class Fiber
{
    public function __construct(callable $callback) {}

    public function run(mixed ...$args): mixed {}

    public function resume(mixed $data = null): mixed {}

    public static function yield(mixed $data = null): mixed {}

    public function throw(Throwable $exception): mixed {}

    public function getId(): int|string {}

    public function getStatus(): int {}

    public static function getCurrent(): static {}

    public function getExitStatus(): int {}

    public function getReturn(): mixed {}

    public function getExecutingFile(): string {}

    public function getExecutingLine(): int {}

    public function getTrace(int $options = DEBUG_BACKTRACE_PROVIDE_OBJECT): array {}
}

class FiberError extends Error
{
}

class FiberException extends Exception
{
}
