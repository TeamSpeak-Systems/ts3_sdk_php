--TEST--
channel managment
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
ts3client_spawnNewServerConnectionHandler(0, $connection1);
ts3client_spawnNewServerConnectionHandler(0, $connection2);
ts3client_createIdentity($identity1);
ts3client_createIdentity($identity2);
ts3client_startConnection($connection1, $identity1, $ip, $port, "${user}_1", $defaultChannelID, $defaultChannelPassword, $serverPassword);
ts3client_startConnection($connection2, $identity2, $ip, $port, "${user}_2", $defaultChannelID, $defaultChannelPassword, $serverPassword);
if (ts3client_setChannelVariableAsString($connection1, 0, CHANNEL_NAME, "new_channel") != ERROR_ok)
    exit("failed to setting  new channel name");
if (ts3client_flushChannelCreation($connection1, 0) != ERROR_ok)
    exit("failed to create new channel");
if (ts3client_getChannelList($connection1, $channels) != ERROR_ok)
    exit("failed getting channel list");
echo("created channel\n");
foreach ($channels as $channel)
{
    if (ts3client_getChannelVariableAsString($connection1, $channel, CHANNEL_NAME, $name) != ERROR_ok)
        exit("failed getting channel name");
    if ($name == "new_channel")
    {
        if (ts3client_getClientID($connection2, $client) != ERROR_ok)
            exit("failed getting clientID");
        if (ts3client_requestClientMove($connection1, $client, $channel,"") != ERROR_ok)
            exit("failed moving client");
        echo("moved client\n");

        if (ts3client_requestChannelDelete($connection1, $channel, false) == ERROR_ok)
            exit("channel delete did not fail");

        if (ts3client_requestChannelDelete($connection1, $channel, true) != ERROR_ok)
            exit("failed to delete channel");

        echo("delete channel\n");
        break;
    }
}


ts3client_stopconnection($connection1, "bye");
ts3client_stopconnection($connection2, "bye");
ts3client_destroyserverconnectionhandler($connection1);
ts3client_destroyserverconnectionhandler($connection2);
echo("passed");
?>
--EXPECT--
created channel
moved client
delete channel
passed
