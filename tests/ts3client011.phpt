--TEST--
get client variables
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
if (ts3client_requestClientVariables($connection1, $client) != ERROR_ok)
    exit("failed getting client variables");
if (ts3client_getChannelOfClient($connection1, $client, $channel_for_1) != ERROR_ok)
    exit("failed getting channel of client for 1");
if (ts3client_getChannelOfClient($connection2, $client, $channel_for_2) != ERROR_ok)
    exit("failed getting channel of client for 2");
if ($channel_for_1 != $channel_for_2)
    exit("failed getting channel of client");
if (ts3client_getClientVariableAsString($connection1, $client, CLIENT_NICKNAME, $nickname) != ERROR_ok)
    exit("failed getting nickname");
if ($nickname != "${user}_2")
    exit("got wrong username");
if (ts3client_getClientVariableAsInt($connection1, $client, CLIENT_IDLE_TIME, $idletime) != ERROR_ok)
    exit("failed getting idle time");
if ($idletime < 0 || $idletime > 10)
    exit("got wrong idle time");
if (ts3client_getClientVariableAsUInt64($connection1, $client, CLIENT_VOLUME_MODIFICATOR, $volume) != ERROR_ok)
    exit("failed getting volume modificator");
if ($volume != 0)
    exit("got wrong volume modificator");
ts3client_stopconnection($connection1, "bye");
ts3client_stopconnection($connection2, "bye");
ts3client_destroyserverconnectionhandler($connection1);
ts3client_destroyserverconnectionhandler($connection2);
echo("passed");
?>
--EXPECT--
passed
