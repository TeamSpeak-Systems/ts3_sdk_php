--TEST--
get client connection status 
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
ts3client_spawnNewServerConnectionHandler(0, $connection);
ts3client_createIdentity($identity);
ts3client_startConnection($connection, $identity, $ip, $port, $user, $defaultChannelID, $defaultChannelPassword, $serverPassword);
ts3client_getClientID($connection, $client);
if (ts3client_requestConnectionInfo($connection, $client) != ERROR_ok)
    exit("failed getting connection info");
if (ts3client_getConnectionVariableAsUInt64($connection, $client, CONNECTION_BYTES_SENT_SPEECH, $speech_total) != ERROR_ok)
    exit("failed getting uint64 connection variable");
if (ts3client_getConnectionVariableAsDouble($connection, $client, CONNECTION_CLIENT2SERVER_PACKETLOSS_SPEECH, $speech_last_second) != ERROR_ok)
    exit("failed getting double connection variable");
if (ts3client_getConnectionVariableAsString($connection, $client, CONNECTION_CLIENT_IP, $ip) != ERROR_ok)
    exit("failed getting string connection variable");
if (ts3client_cleanUpConnectionInfo($connection, $client) != ERROR_client_invalid_id)
    exit("failed cleaning connection info");
if ($speech_total != 0)
    exit("recieved invalid uint64 value");
if ($speech_last_second != 0)
    exit("recieved invalid double value");
if (filter_var($ip, FILTER_VALIDATE_IP) === false)
    exit("recieved invalid string");
ts3client_stopconnection($connection, "bye");
ts3client_destroyserverconnectionhandler($connection);
echo("passed");
?>
--EXPECT--
passed
