--TEST--
get list channels
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
ts3client_spawnNewServerConnectionHandler(0, $connection);
ts3client_createIdentity($identity);
ts3client_startConnection($connection, $identity, $ip, $port, $user, $defaultChannelID, $defaultChannelPassword, $serverPassword);
ts3client_getClientID($connection, $clientID);
ts3client_getChannelOfClient($connection, $clientID, $channelID);
if (ts3client_getChannelList($connection, $channels) != ERROR_ok)
    exit("failed getting list of channels");
if (in_array($channelID, $channels) == false)
    exit("list of channels must be wrong");
ts3client_stopConnection($connection, "bye");
ts3client_destroyServerConnectionHandler($connection);
echo("passed");
?>
--EXPECT--
passed
