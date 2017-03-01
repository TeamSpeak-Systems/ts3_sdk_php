--TEST--
moving client/channels, and deleting channels
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
ts3client_spawnNewServerConnectionHandler(0, $connection);
ts3client_createIdentity($identity);
ts3client_startConnection($connection, $identity, $ip, $port, $user, $defaultChannelID, $defaultChannelPassword, $serverPassword);
ts3client_getClientID($connection, $self);
ts3client_setChannelVariableAsString($connection, 0, CHANNEL_NAME, "parent");
ts3client_flushChannelCreation($connection, 0);
ts3client_getChannelOfClient($connection, $self, $parent);
ts3client_setChannelVariableAsString($connection, 0, CHANNEL_NAME, "child_1");
ts3client_flushChannelCreation($connection, $parent);
ts3client_getChannelOfClient($connection, $self, $child_1);
ts3client_setChannelVariableAsString($connection, 0, CHANNEL_NAME, "child_2");
ts3client_flushChannelCreation($connection, $child_1);
ts3client_getChannelOfClient($connection, $self, $child_2);
if (ts3client_requestChannelMove($connection, $child_2, $parent, $child_1) != ERROR_ok)
    exit("failed moving channel");
if (ts3client_requestClientMove($connection, $self, $parent, "") != ERROR_ok)
    exit("failed moving client");
if (ts3client_requestChannelDelete($connection, $parent, true) != ERROR_ok)
    exit("failed deleting channel");
ts3client_stopConnection($connection, "bye");
ts3client_destroyServerConnectionHandler($connection);
echo("passed");
?>
--EXPECT--
passed
