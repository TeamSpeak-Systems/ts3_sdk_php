--TEST--
get own client and channel
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
ts3client_spawnNewServerConnectionHandler(0, $connection);
ts3client_createIdentity($identity);
ts3client_startConnection($connection, $identity, $ip, $port, $user, $defaultChannelID, $defaultChannelPassword, $serverPassword);
if (ts3client_getClientID($connection, $clientID) != ERROR_ok)
    exit("failed getting clientID");
echo("got client id\n");
if (ts3client_getChannelOfClient($connection, $clientID, $channelID) != ERROR_ok)
    exit("failed getting channelID");
echo("got channel id\n");
ts3client_stopConnection($connection, "bye");
ts3client_destroyServerConnectionHandler($connection);
echo("passed");
?>
--EXPECT--
got client id
got channel id
passed
