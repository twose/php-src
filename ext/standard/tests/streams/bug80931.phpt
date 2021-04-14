--TEST--
Bug #80931 (HTTP request hangs if the server doesn't close connection)
--FILE--
<?php
ini_set('default_socket_timeout', -1);
$context = stream_context_create(['http' => ['protocol_version' => 1.1, 'header' => ['Connection: keep-alive']]]);
$content = file_get_contents('http://www.baidu.com', 0, $context);
$content = trim($content);
echo (str_ends_with($content, "</html>") ? 'PASS' : 'FAIL') . PHP_EOL;
?>
--EXPECT--
PASS
