--TEST--
check version
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
if (ts3client_getClientLibVersion($version) != ERROR_ok)
    exit("failed getting version");
echo(explode(" ", $version)[0]);
if (ts3client_getClientLibVersionNumber($versionNumber))
    exit("failed getting version number");
if ($versionNumber < 1483228800 || $versionNumber > 2524611600)
    exit("versionNumber is most likely wrong");
?>
--EXPECT--
3.0.4
