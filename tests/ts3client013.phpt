--TEST--
channel subscribe/unsubscribe
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
ts3client_spawnNewServerConnectionHandler(0, $connection1);
ts3client_createIdentity($identity1);
ts3client_startConnection($connection1, $identity1, $ip, $port, "${user}_1", $defaultChannelID, $defaultChannelPassword, $serverPassword);
ts3client_setChannelVariableAsString($connection1, 0, CHANNEL_NAME, "new_channel");
ts3client_flushChannelCreation($connection1, 0);
ts3client_getClientID($connection1, $client);
ts3client_spawnNewServerConnectionHandler(0, $connection2);
ts3client_createIdentity($identity2);
ts3client_startConnection($connection2, $identity2, $ip, $port, "${user}_2", $defaultChannelID, $defaultChannelPassword, $serverPassword);
ts3client_getClientList($connection2, $clients);
if (in_array($client, $clients) == true)
    exit("saw client in unsubscribed channel");
if (ts3client_requestChannelSubscribeAll($connection2) != ERROR_ok)
    exit("failed to subscribe to all channels");
ts3client_getClientList($connection2, $clients);
if (in_array($client, $clients) == false)
    exit("failed to see client in subscribed channel");
if (ts3client_requestChannelUnsubscribeAll($connection2) != ERROR_ok)
    exit("failed to unsubscribe to all channels");
ts3client_getClientList($connection2, $clients);
if (in_array($client, $clients) == true)
    exit("client still visible after unsubscribing");
ts3client_stopconnection($connection1, "bye");
ts3client_stopconnection($connection2, "bye");
ts3client_destroyserverconnectionhandler($connection1);
ts3client_destroyserverconnectionhandler($connection2);
echo("passed");
?>
--EXPECT--
passed
