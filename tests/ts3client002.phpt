--TEST--
connect and disconnect
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
if (ts3client_spawnNewServerConnectionHandler(0, $connection) != ERROR_ok)
    exit("failed spawning new connection");
echo("created server connection\n");
if (ts3client_createIdentity($identity) != ERROR_ok)
    exit("failed creating identity");
echo("created identity\n");
if (ts3client_startConnection($connection, $identity, $ip, $port, $user, $defaultChannelID, $defaultChannelPassword, $serverPassword) != ERROR_ok)
    exit("failed starting connection");
echo("started connection\n");
if (ts3client_stopConnection($connection, "bye") != ERROR_ok)
    exit("failed stopping connection");
echo("stopped connection\n");
if (ts3client_destroyServerConnectionHandler($connection) != ERROR_ok)
    exit("failed destroying connectiong");
echo("passed");
?>
--EXPECT--
created server connection
created identity
started connection
stopped connection
passed
