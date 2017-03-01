--TEST--
get list of clients
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
ts3client_spawnNewServerConnectionHandler(0, $connection);
ts3client_createIdentity($identity);
ts3client_startConnection($connection, $identity, $ip, $port, $user, $defaultChannelID, $defaultChannelPassword, $serverPassword);
ts3client_getClientID($connection, $clientID);
if (ts3client_getClientList($connection, $clients) != ERROR_ok)
    exit("failed getting list of clients");
if (in_array($clientID, $clients) == false)
    exit("list of clients must be wrong");
ts3client_stopConnection($connection, "bye");
ts3client_destroyServerConnectionHandler($connection);
echo("passed");
?>
--EXPECT--
passed
