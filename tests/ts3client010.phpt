--TEST--
getting error message
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
if (ts3client_getErrorMessage(ERROR_accounting_already_started, $message) != ERROR_ok)
    exit("failed getting error message");
echo($message);
?>
--EXPECT--
virtualserver already started
