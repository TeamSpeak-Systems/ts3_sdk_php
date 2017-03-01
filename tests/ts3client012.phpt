--TEST--
client kicking
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
ts3client_spawnNewServerConnectionHandler(0, $connection1);
ts3client_spawnNewServerConnectionHandler(0, $connection2);
ts3client_createIdentity($identity1);
ts3client_createIdentity($identity2);
ts3client_startConnection($connection1, $identity1, $ip, $port, "${user}_1", $defaultChannelID, $defaultChannelPassword, $serverPassword);
ts3client_startConnection($connection2, $identity2, $ip, $port, "${user}_2", $defaultChannelID, $defaultChannelPassword, $serverPassword);
ts3client_getClientID($connection2, $client);
ts3client_setChannelVariableAsString($connection2, 0, CHANNEL_NAME, "new_channel");
ts3client_flushChannelCreation($connection2, 0);
ts3client_getChannelOfClient($connection2, $client, $from_channel);
if (ts3client_requestClientKickFromChannel($connection1, $client, "no reason") != ERROR_ok)
    exit("failed kicking client from channel");
ts3client_getChannelOfClient($connection1, $client, $to_channel);
if ($from_channel == $to_channel)
    exit("kicking didn't move client");
if (ts3client_requestClientKickFromServer($connection1, $client, "no reason") != ERROR_ok)
    exit("failed kicking client from server");
sleep(0.1);
if (ts3client_getConnectionStatus($connection2, $status) != ERROR_ok)
    exit("failed getting connection status");
if ($status != STATUS_DISCONNECTED)
    exit("kicking didn't remove client from server");
ts3client_stopconnection($connection1, "bye");
ts3client_destroyserverconnectionhandler($connection1);
ts3client_destroyserverconnectionhandler($connection2);
echo("passed");
?>
--EXPECT--
passed
