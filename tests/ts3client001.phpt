--TEST--
create identity
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
$identity="";
$result = ts3client_createIdentity($identity);
echo("passed");
?>
--EXPECT--
passed
