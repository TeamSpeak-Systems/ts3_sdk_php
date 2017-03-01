--TEST--
send text message to server
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
ts3client_spawnNewServerConnectionHandler(0, $connection);
ts3client_createIdentity($identity);
ts3client_startConnection($connection, $identity, $ip, $port, $user, $defaultChannelID, $defaultChannelPassword, $serverPassword);
if (ts3client_requestSendServerTextMsg($connection, "server message") != ERROR_ok)
    exit("failed send server text message");
ts3client_stopConnection($connection, "bye");
ts3client_destroyServerConnectionHandler($connection);
echo("passed");
?>
--EXPECT--
passed
