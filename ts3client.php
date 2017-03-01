<?php

/**
 * Simplified TeamSpeak Client Library SDK for PHP.
 */

/**
 * The complete Client Lib version string.
 * @param string $result <p>
 * Clientlib version string.
 * </p>
 * @return string ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getClientLibVersion(&$result) {}

/**
 * Get the version number, which is a part of the complete version string.
 * @param int $result <p>
 * Clientlib version number.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getClientLibVersionNumber(&$result) {}

/**
 * To create a new server connection handler and receive its ID.
 * @param int $port <p>
 * Port the client should bind on. Specify zero to let the operating system chose any free port. In most cases passing zero is the best choice.
 * </p>
 * @param int $result <p>
 * Clientlib version number.
 * </p>
 * @return int If port is specified, the function return value should be checked for ERROR_unable_to_bind_network_port. Handle this error by switching to an alternative port until a "free" port is hit and the function returns ERROR_ok.
 * @ts3client
 */
function ts3client_spawnNewServerConnectionHandler($port, &$result) {}

/**
 * To destroy a server connection handler.
 * @param int $serverConnectionHandlerID <p>
 * ID of the server connection handler to destroy.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_destroyServerConnectionHandler($serverConnectionHandlerID) {}

/**
 * Creates new client identity, should be saved to the configuration and be reused.
 * @param string $result <p>
 * The identity string.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_createIdentity(&$result) {}

/**
 * Get the unique-id of an identity.
 * @param string $identityString <p>
 * The identity string.
 * </p>
 * @param string $result <p>
 * The unique-id string.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_identityStringToUniqueIdentifier($identityString, &$result) {}

/**
 * Get a printable error string for a specific error code.
 * @param int $errorCode <p>
 * The error code returned from all ts3client functions.
 * </p>
 * @param string $error <p>
 * The error message string.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getErrorMessage($errorCode, &$error) {}

/**
 * Connect to a TeamSpeak 3 server
 * @param int $serverConnectionHandlerID <p>
 * Unique identifier for this server connection. Created with ts3client_spawnNewServerConnectionHandler.
 * </p>
 * @param string $identity <p>
 * The clients identity. 
 * This string has to be created by calling ts3client_createIdentity. Please note an application should create the identity only once, store the string locally and reuse it for future connections.
 * </p>
 * @param string $ip <p>
 * Hostname or IP of the TeamSpeak 3 server.
 * If you pass a hostname instead of an IP, the Client Lib will try to resolve it to an IP, but the function may block for an unusually long period of time while resolving is taking place. If you are relying on the function to return quickly, we recommend to resolve the hostname yourself (e.g. asynchronously) and then call ts3client_startConnection with the IP instead of the hostname.
 * </p>
 * @param int $port <p>
 * UDP port of the TeamSpeak 3 server, by default 9987. TeamSpeak 3 uses UDP. Support for TCP might be added in the future.
 * </p>
 * @param string $nickname <p>
 * On login, the client attempts to take this nickname on the connected server. Note this is not necessarily the actually assigned nickname, as the server can modifiy the nickname ("gandalf_1" instead the requested "gandalf") or refuse blocked names.
 * </p>
 * @param int $defaultChannelID <p>
 * ID of a channel on the TeamSpeak 3 server. 
 * If the channel exists and the user has sufficient rights and supplies the correct password if required, the channel will be joined on login.
 * Pass 0 to join the servers default channel.
 * </p>
 * @param string $defaultChannelPassword <p>
 * Password for the default channel. Pass an empty string if no password is required or no default channel is specified
 * </p>
 * @param string $serverPassword <p>
 * Password for the server. Pass an empty string if the server does not require a password.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_startConnection($serverConnectionHandlerID, $identity, $ip, $port, $nickname, $defaultChannelID, $defaultChannelPassword, $serverPassword) {}

/**
 * Disconnect from a TeamSpeak 3 server.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param string $quitMessage <p>
 * A message like for example "leaving".
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_stopConnection($serverConnectionHandlerID, $quitMessage) {}

/**
 * Switch your own or another client to a certain channel.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * ID of the client to move.
 * </p>
 * @param int $newChannelID <p>
 * ID of the channel the client wants to join.
 * </p>
 * @param string $password <p>
 * An optional password, required for password-protected channels. Pass an empty string if no password is given
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestClientMove($serverConnectionHandlerID, $clientID, $newChannelID, $password) {}

/**
 * Get the latest data for a given client.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * ID of the client whose variables are requested.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestClientVariables($serverConnectionHandlerID, $clientID) {}

/**
 * Kick a client from a channel.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * The ID of the client to be kicked.
 * </p>
 * @param int $kickReason <p>
 * A short message explaining why the client is kicked from the channel.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestClientKickFromChannel($serverConnectionHandlerID, $clientID, $kickReason) {}

/**
 * Kick a client from the server.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * The ID of the client to be kicked.
 * </p>
 * @param int $kickReason <p>
 * A short message explaining why the client is kicked from the server.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestClientKickFromServer($serverConnectionHandlerID, $clientID, $kickReason) {}

/**
 * Remove a channel.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $channelID <p>
 * The ID of the channel to be deleted.
 * </p>
 * @param bool $force <p>
 * If true, the channel will be deleted even when it is not empty. 
 * Clients within the deleted channel are transfered to the default channel. Any contained subchannels are removed as well.
 * If false, the server will refuse to delete a channel that is not empty.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestChannelDelete($serverConnectionHandlerID, $channelID, $force) {}

/**
 * Move a channel to a new parent channel.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param string $channelID <p>
 * ID of the channel to be moved.
 * </p>
 * @param string $newChannelParentID <p>
 * ID of the parent channel where the moved channel is to be inserted as child. Use 0 to insert as top-level channel
 * </p>
 * @param string $newChannelOrder <p>
 * Channel order defining where the channel should be sorted under the new parent. Pass 0 to sort the channel right after the parent. See the chapter Channel sorting for details.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestChannelMove($serverConnectionHandlerID, $channelID, $newChannelParentID, $newChannelOrder) {}

/**
 * Send a private text message to a client.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param string $message <p>
 * String containing the text message.
 * </p>
 * @param int $targetClientID <p>
 * Id of the target client.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestSendPrivateTextMsg($serverConnectionHandlerID, $message, $targetClientID) {}

/**
 * Send a text message to a channel.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param string $message <p>
 * String containing the text message.
 * </p>
 * @param int $targetChannelID <p>
 * Id of the target channel.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestSendChannelTextMsg($serverConnectionHandlerID, $message, $targetChannelID) {}

/**
 * Send a text message to the virtual server.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param string $message <p>
 * String containing the text message
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestSendServerTextMsg($serverConnectionHandlerID, $message) {}

/**
 * Request more up to date connection information.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * The ID of the client.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestConnectionInfo($serverConnectionHandlerID, $clientID) {}

/**
 * Subscribe to all channels on the server.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestChannelSubscribeAll($serverConnectionHandlerID) {}

/**
 * Unsubscribe from all channels on the server.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestChannelUnsubscribeAll($serverConnectionHandlerID) {}

/**
 * Query the clientID of own connected client. Available after the state STATUS_CONNECTED has been reached.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $result <p>
 * The client ID.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getClientID($serverConnectionHandlerID, &$result) {}

/**
 * Check if a connection to a given server connection handler is established.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $result <p>
 * STATUS_DISCONNECTED when disconnected, STATUS_CONNECTION_ESTABLISHED when fully connected, or something in between.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getConnectionStatus($serverConnectionHandlerID, &$result) {}

/**
 * Get a connection variable as UInt64.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * The client ID.
 * </p>
 * @param int $flag <p>
 * Connection propery to query.
 * </p>
 * @param int $result <p>
 * The result as int.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getConnectionVariableAsUInt64($serverConnectionHandlerID, $clientID, $flag, &$result) {}

/**
 * Get a connection variable as double.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * The client ID.
 * </p>
 * @param int $flag <p>
 * Connection propery to query.
 * </p>
 * @param double $result <p>
 * The result as double.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getConnectionVariableAsDouble($serverConnectionHandlerID, $clientID, $flag, &$result) {}

/**
 * Get a connection variable as string.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * The client ID.
 * </p>
 * @param int $flag <p>
 * Connection propery to query.
 * </p>
 * @param string $result <p>
 * The result as string.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getConnectionVariableAsString($serverConnectionHandlerID, $clientID, $flag, &$result) {}

/**
 * remove requested connection information.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * The client ID.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_cleanUpConnectionInfo($serverConnectionHandlerID, $clientID)  {}

/**
 * Request more up to date server connection information.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestServerConnectionInfo($serverConnectionHandlerID) {}

/**
 * Get a server connection variable as UInt64.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $flag <p>
 * Connection propery to query.
 * </p>
 * @param double $result <p>
 * The result as double.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getServerConnectionVariableAsUInt64($serverConnectionHandlerID, $flag, &$result) {}

/**
 * Get a server connection variable as float.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $flag <p>
 * Connection propery to query.
 * </p>
 * @param double $result <p>
 * The result as float.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getServerConnectionVariableAsFloat($serverConnectionHandlerID, $flag, &$result) {}

/**
 * Get various information related about the own client as int.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $flag <p>
 * Client propery to query.
 * </p>
 * @param int $result <p>
 * The result as int.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getClientSelfVariableAsInt($serverConnectionHandlerID, $flag, &$result) {}

/**
 * Get various information related about the own client as string.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $flag <p>
 * Client propery to query.
 * </p>
 * @param string $result <p>
 * The result as string.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getClientSelfVariableAsString($serverConnectionHandlerID, $flag, &$result) {}

/**
 * Set various information related about the own client as int.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $flag <p>
 * Client propery to set.
 * </p>
 * @param int $value <p>
 * Value the client property should be changed to.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_setClientSelfVariableAsInt($serverConnectionHandlerID, $flag, $value) {}

/**
 * Set various information related about the own client as string.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $flag <p>
 * Client propery to set.
 * </p>
 * @param string $value <p>
 * Value the client property should be changed to.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_setClientSelfVariableAsString($serverConnectionHandlerID, $flag, $value) {}

/**
 * Sent changes to the TeamSpeak 3 server. Must be called after modifying one or more client variables.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_flushClientSelfUpdates($serverConnectionHandlerID) {}

/**
 * Query client related information as int.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * ID of the client whose property is queried.
 * </p>
 * @param int $flag <p>
 * Client propery to query.
 * </p>
 * @param int $result <p>
 * The result as int.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getClientVariableAsInt($serverConnectionHandlerID, $clientID, $flag, &$result) {}

/**
 * Query client related information as UInt64.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * ID of the client whose property is queried.
 * </p>
 * @param int $flag <p>
 * Client propery to query.
 * </p>
 * @param int $result <p>
 * The result as UInt64.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getClientVariableAsUInt64($serverConnectionHandlerID, $clientID, $flag, &$result) {}

/**
 * Query client related information as string.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * ID of the client whose property is queried.
 * </p>
 * @param int $flag <p>
 * Client propery to query.
 * </p>
 * @param string $result <p>
 * The result as string.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getClientVariableAsString($serverConnectionHandlerID, $clientID, $flag, &$result) {}

/**
 * Get a list of all currently visible clients on the virtual server.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int[] $result <p>
 * Array of client IDs.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getClientList($serverConnectionHandlerID, &$result) {}

/**
 * Query the channel ID the specified client.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $clientID <p>
 * ID of the client whose channel ID is requested.
 * </p>
 * @param int $result <p>
 * ID of the channel.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getChannelOfClient($serverConnectionHandlerID, $clientID, &$result) {}

/**
 * Query information related to a channel as int.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $channelID <p>
 * ID of the channel whose property is queried.
 * </p>
 * @param int $flag <p>
 * Channel propery to query.
 * </p>
 * @param int $result <p>
 * The result as int.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getChannelVariableAsInt($serverConnectionHandlerID, $channelID, $flag, &$result) {}

/**
 * Query information related to a channel as UInt64.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $channelID <p>
 * ID of the channel whose property is queried.
 * </p>
 * @param int $flag <p>
 * Channel propery to query.
 * </p>
 * @param int $result <p>
 * The result as UInt64.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getChannelVariableAsUInt64($serverConnectionHandlerID, $channelID, $flag, &$result) {}

/**
 * Query information related to a channel as string.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $channelID <p>
 * ID of the channel whose property is queried.
 * </p>
 * @param int $flag <p>
 * Channel propery to query.
 * </p>
 * @param string $result <p>
 * The result as string.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getChannelVariableAsString($serverConnectionHandlerID, $channelID, $flag, &$result) {}

/**
 * Set information related to a channel as int.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $channelID <p>
 * ID of the channel whose property is set.
 * </p>
 * @param int $flag <p>
 * Channel propery to query.
 * </p>
 * @param int $value <p>
 * The value as int.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_setChannelVariableAsInt($serverConnectionHandlerID, $channelID, $flag, $value) {}

/**
 * Set information related to a channel as UInt64.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $channelID <p>
 * ID of the channel whose property is set.
 * </p>
 * @param int $flag <p>
 * Channel propery to query.
 * </p>
 * @param int $value <p>
 * The value as UInt64.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_setChannelVariableAsUInt64($serverConnectionHandlerID, $channelID, $flag, $value) {}
/**
 * Set information related to a channel as string.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $channelID <p>
 * ID of the channel whose property is set.
 * </p>
 * @param int $flag <p>
 * Channel propery to query.
 * </p>
 * @param string $value <p>
 * The value as string.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_setChannelVariableAsString($serverConnectionHandlerID, $channelID, $flag, $value) {}

/**
 * Sent changes to the TeamSpeak 3 server. Must be called after modifying one or more channel variables.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $channelID <p>
 * ID of the channel.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_flushChannelUpdates($serverConnectionHandlerID, $channelID) {}

/**
 * Flush new channel information to the server, and create a new channel.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $channelParentID <p>
 * ID of the parent channel, if the new channel is to be created as subchannel. Pass zero if the channel should be created as top-level channel.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_flushChannelCreation($serverConnectionHandlerID, $channelParentID) {}

/**
 * Get a list of all channels on the virtual server.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int[] $result <p>
 * Array of channel IDs. 
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getChannelList($serverConnectionHandlerID, &$result) {}

/**
 * Get a list of all clients in the channel, if the channel is currently subscribed.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param string $channelID <p>
 * ID of the channel whose client list is requested.
 * </p>
 * @param string $result <p>
 * Array of client IDs.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getChannelClientList($serverConnectionHandlerID, $channelID, &$result) {}

/**
 * Get the parent channel of a given channel.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param string $channelID <p>
 * ID of the channel whose parent channel ID is requested.
 * </p>
 * @param string $result <p>
 * ID of the parent channel of the specified channel.
 * If the specified channel has no parent channel, result will be set to the reserved channel ID 0.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getParentChannelOfChannel($serverConnectionHandlerID, $channelID, &$result) {}

/**
 * Query the time in seconds since the last client has left a temporary channel.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $channelID <p>
 * ID of the channel to query.
 * </p>
 * @param int $result <p>
 * Time in seconds.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getChannelEmptySecs($serverConnectionHandlerID, $channelID, &$result) {}

/**
 * Set information related to a virtual server as int.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $flag <p>
 * Virtual server propery to query.
 * </p>
 * @param int $result <p>
 * The result as int.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getServerVariableAsInt($serverConnectionHandlerID, $flag, &$result) {}

/**
 * Set information related to a virtual server as UInt64.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $flag <p>
 * Virtual server propery to query.
 * </p>
 * @param int $result <p>
 * The result as UInt64.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getServerVariableAsUInt64($serverConnectionHandlerID, $flag, &$result) {}

/**
 * Set information related to a virtual server as string.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @param int $flag <p>
 * Virtual server propery to query.
 * </p>
 * @param string $result <p>
 * The result as string.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_getServerVariableAsString($serverConnectionHandlerID, $flag, &$result) {}

/**
 * Refresh the server information.
 * @param int $serverConnectionHandlerID <p>
 * The unique ID for this server connection handler.
 * </p>
 * @return int ERROR_ok on success, otherwise an error code.
 * @ts3client
 */
function ts3client_requestServerVariables($serverConnectionHandlerID) {}


/** @var int ERROR_ok */
const ERROR_ok = 0;
/** @var int ERROR_undefined */
const ERROR_undefined = 0;
/** @var int ERROR_not_implemented */
const ERROR_not_implemented = 0;
/** @var int ERROR_ok_no_update */
const ERROR_ok_no_update = 0;
/** @var int ERROR_dont_notify */
const ERROR_dont_notify = 0;
/** @var int ERROR_lib_time_limit_reached */
const ERROR_lib_time_limit_reached = 0;
/** @var int ERROR_command_not_found */
const ERROR_command_not_found = 0;
/** @var int ERROR_unable_to_bind_network_port */
const ERROR_unable_to_bind_network_port = 0;
/** @var int ERROR_no_network_port_available */
const ERROR_no_network_port_available = 0;
/** @var int ERROR_port_already_in_use */
const ERROR_port_already_in_use = 0;
/** @var int ERROR_client_invalid_id */
const ERROR_client_invalid_id = 0;
/** @var int ERROR_client_nickname_inuse */
const ERROR_client_nickname_inuse = 0;
/** @var int ERROR_client_protocol_limit_reached */
const ERROR_client_protocol_limit_reached = 0;
/** @var int ERROR_client_invalid_type */
const ERROR_client_invalid_type = 0;
/** @var int ERROR_client_already_subscribed */
const ERROR_client_already_subscribed = 0;
/** @var int ERROR_client_not_logged_in */
const ERROR_client_not_logged_in = 0;
/** @var int ERROR_client_could_not_validate_identity */
const ERROR_client_could_not_validate_identity = 0;
/** @var int ERROR_client_version_outdated */
const ERROR_client_version_outdated = 0;
/** @var int ERROR_client_is_flooding */
const ERROR_client_is_flooding = 0;
/** @var int ERROR_client_hacked */
const ERROR_client_hacked = 0;
/** @var int ERROR_client_cannot_verify_now */
const ERROR_client_cannot_verify_now = 0;
/** @var int ERROR_client_login_not_permitted */
const ERROR_client_login_not_permitted = 0;
/** @var int ERROR_client_not_subscribed */
const ERROR_client_not_subscribed = 0;
/** @var int ERROR_channel_invalid_id */
const ERROR_channel_invalid_id = 0;
/** @var int ERROR_channel_protocol_limit_reached */
const ERROR_channel_protocol_limit_reached = 0;
/** @var int ERROR_channel_already_in */
const ERROR_channel_already_in = 0;
/** @var int ERROR_channel_name_inuse */
const ERROR_channel_name_inuse = 0;
/** @var int ERROR_channel_not_empty */
const ERROR_channel_not_empty = 0;
/** @var int ERROR_channel_can_not_delete_default */
const ERROR_channel_can_not_delete_default = 0;
/** @var int ERROR_channel_default_require_permanent */
const ERROR_channel_default_require_permanent = 0;
/** @var int ERROR_channel_invalid_flags */
const ERROR_channel_invalid_flags = 0;
/** @var int ERROR_channel_parent_not_permanent */
const ERROR_channel_parent_not_permanent = 0;
/** @var int ERROR_channel_maxclients_reached */
const ERROR_channel_maxclients_reached = 0;
/** @var int ERROR_channel_maxfamily_reached */
const ERROR_channel_maxfamily_reached = 0;
/** @var int ERROR_channel_invalid_order */
const ERROR_channel_invalid_order = 0;
/** @var int ERROR_channel_no_filetransfer_supported */
const ERROR_channel_no_filetransfer_supported = 0;
/** @var int ERROR_channel_invalid_password */
const ERROR_channel_invalid_password = 0;
/** @var int ERROR_channel_invalid_security_hash */
const ERROR_channel_invalid_security_hash = 0;
/** @var int ERROR_server_invalid_id */
const ERROR_server_invalid_id = 0;
/** @var int ERROR_server_running */
const ERROR_server_running = 0;
/** @var int ERROR_server_is_shutting_down */
const ERROR_server_is_shutting_down = 0;
/** @var int ERROR_server_maxclients_reached */
const ERROR_server_maxclients_reached = 0;
/** @var int ERROR_server_invalid_password */
const ERROR_server_invalid_password = 0;
/** @var int ERROR_server_is_virtual */
const ERROR_server_is_virtual = 0;
/** @var int ERROR_server_is_not_running */
const ERROR_server_is_not_running = 0;
/** @var int ERROR_server_is_booting */
const ERROR_server_is_booting = 0;
/** @var int ERROR_server_status_invalid */
const ERROR_server_status_invalid = 0;
/** @var int ERROR_server_version_outdated */
const ERROR_server_version_outdated = 0;
/** @var int ERROR_server_duplicate_running */
const ERROR_server_duplicate_running = 0;
/** @var int ERROR_parameter_quote */
const ERROR_parameter_quote = 0;
/** @var int ERROR_parameter_invalid_count */
const ERROR_parameter_invalid_count = 0;
/** @var int ERROR_parameter_invalid */
const ERROR_parameter_invalid = 0;
/** @var int ERROR_parameter_not_found */
const ERROR_parameter_not_found = 0;
/** @var int ERROR_parameter_convert */
const ERROR_parameter_convert = 0;
/** @var int ERROR_parameter_invalid_size */
const ERROR_parameter_invalid_size = 0;
/** @var int ERROR_parameter_missing */
const ERROR_parameter_missing = 0;
/** @var int ERROR_parameter_checksum */
const ERROR_parameter_checksum = 0;
/** @var int ERROR_vs_critical */
const ERROR_vs_critical = 0;
/** @var int ERROR_connection_lost */
const ERROR_connection_lost = 0;
/** @var int ERROR_not_connected */
const ERROR_not_connected = 0;
/** @var int ERROR_no_cached_connection_info */
const ERROR_no_cached_connection_info = 0;
/** @var int ERROR_currently_not_possible */
const ERROR_currently_not_possible = 0;
/** @var int ERROR_failed_connection_initialisation */
const ERROR_failed_connection_initialisation = 0;
/** @var int ERROR_could_not_resolve_hostname */
const ERROR_could_not_resolve_hostname = 0;
/** @var int ERROR_invalid_server_connection_handler_id */
const ERROR_invalid_server_connection_handler_id = 0;
/** @var int ERROR_could_not_initialise_input_manager */
const ERROR_could_not_initialise_input_manager = 0;
/** @var int ERROR_clientlibrary_not_initialised */
const ERROR_clientlibrary_not_initialised = 0;
/** @var int ERROR_serverlibrary_not_initialised */
const ERROR_serverlibrary_not_initialised = 0;
/** @var int ERROR_whisper_too_many_targets */
const ERROR_whisper_too_many_targets = 0;
/** @var int ERROR_whisper_no_targets */
const ERROR_whisper_no_targets = 0;
/** @var int ERROR_connection_ip_protocol_missing */
const ERROR_connection_ip_protocol_missing = 0;
/** @var int ERROR_file_invalid_name */
const ERROR_file_invalid_name = 0;
/** @var int ERROR_file_invalid_permissions */
const ERROR_file_invalid_permissions = 0;
/** @var int ERROR_file_already_exists */
const ERROR_file_already_exists = 0;
/** @var int ERROR_file_not_found */
const ERROR_file_not_found = 0;
/** @var int ERROR_file_io_error */
const ERROR_file_io_error = 0;
/** @var int ERROR_file_invalid_transfer_id */
const ERROR_file_invalid_transfer_id = 0;
/** @var int ERROR_file_invalid_path */
const ERROR_file_invalid_path = 0;
/** @var int ERROR_file_no_files_available */
const ERROR_file_no_files_available = 0;
/** @var int ERROR_file_overwrite_excludes_resume */
const ERROR_file_overwrite_excludes_resume = 0;
/** @var int ERROR_file_invalid_size */
const ERROR_file_invalid_size = 0;
/** @var int ERROR_file_already_in_use */
const ERROR_file_already_in_use = 0;
/** @var int ERROR_file_could_not_open_connection */
const ERROR_file_could_not_open_connection = 0;
/** @var int ERROR_file_no_space_left_on_device */
const ERROR_file_no_space_left_on_device = 0;
/** @var int ERROR_file_exceeds_file_system_maximum_size */
const ERROR_file_exceeds_file_system_maximum_size = 0;
/** @var int ERROR_file_transfer_connection_timeout */
const ERROR_file_transfer_connection_timeout = 0;
/** @var int ERROR_file_connection_lost */
const ERROR_file_connection_lost = 0;
/** @var int ERROR_file_exceeds_supplied_size */
const ERROR_file_exceeds_supplied_size = 0;
/** @var int ERROR_file_transfer_complete */
const ERROR_file_transfer_complete = 0;
/** @var int ERROR_file_transfer_canceled */
const ERROR_file_transfer_canceled = 0;
/** @var int ERROR_file_transfer_interrupted */
const ERROR_file_transfer_interrupted = 0;
/** @var int ERROR_file_transfer_server_quota_exceeded */
const ERROR_file_transfer_server_quota_exceeded = 0;
/** @var int ERROR_file_transfer_client_quota_exceeded */
const ERROR_file_transfer_client_quota_exceeded = 0;
/** @var int ERROR_file_transfer_reset */
const ERROR_file_transfer_reset = 0;
/** @var int ERROR_file_transfer_limit_reached */
const ERROR_file_transfer_limit_reached = 0;
/** @var int ERROR_sound_preprocessor_disabled */
const ERROR_sound_preprocessor_disabled = 0;
/** @var int ERROR_sound_internal_preprocessor */
const ERROR_sound_internal_preprocessor = 0;
/** @var int ERROR_sound_internal_encoder */
const ERROR_sound_internal_encoder = 0;
/** @var int ERROR_sound_internal_playback */
const ERROR_sound_internal_playback = 0;
/** @var int ERROR_sound_no_capture_device_available */
const ERROR_sound_no_capture_device_available = 0;
/** @var int ERROR_sound_no_playback_device_available */
const ERROR_sound_no_playback_device_available = 0;
/** @var int ERROR_sound_could_not_open_capture_device */
const ERROR_sound_could_not_open_capture_device = 0;
/** @var int ERROR_sound_could_not_open_playback_device */
const ERROR_sound_could_not_open_playback_device = 0;
/** @var int ERROR_sound_handler_has_device */
const ERROR_sound_handler_has_device = 0;
/** @var int ERROR_sound_invalid_capture_device */
const ERROR_sound_invalid_capture_device = 0;
/** @var int ERROR_sound_invalid_playback_device */
const ERROR_sound_invalid_playback_device = 0;
/** @var int ERROR_sound_invalid_wave */
const ERROR_sound_invalid_wave = 0;
/** @var int ERROR_sound_unsupported_wave */
const ERROR_sound_unsupported_wave = 0;
/** @var int ERROR_sound_open_wave */
const ERROR_sound_open_wave = 0;
/** @var int ERROR_sound_internal_capture */
const ERROR_sound_internal_capture = 0;
/** @var int ERROR_sound_device_in_use */
const ERROR_sound_device_in_use = 0;
/** @var int ERROR_sound_device_already_registerred */
const ERROR_sound_device_already_registerred = 0;
/** @var int ERROR_sound_unknown_device */
const ERROR_sound_unknown_device = 0;
/** @var int ERROR_sound_unsupported_frequency */
const ERROR_sound_unsupported_frequency = 0;
/** @var int ERROR_sound_invalid_channel_count */
const ERROR_sound_invalid_channel_count = 0;
/** @var int ERROR_sound_read_wave */
const ERROR_sound_read_wave = 0;
/** @var int ERROR_sound_need_more_data */
const ERROR_sound_need_more_data = 0;
/** @var int ERROR_sound_device_busy */
const ERROR_sound_device_busy = 0;
/** @var int ERROR_sound_no_data */
const ERROR_sound_no_data = 0;
/** @var int ERROR_sound_channel_mask_mismatch */
const ERROR_sound_channel_mask_mismatch = 0;
/** @var int ERROR_permissions_client_insufficient */
const ERROR_permissions_client_insufficient = 0;
/** @var int ERROR_permissions */
const ERROR_permissions = 0;
/** @var int ERROR_accounting_virtualserver_limit_reached */
const ERROR_accounting_virtualserver_limit_reached = 0;
/** @var int ERROR_accounting_slot_limit_reached */
const ERROR_accounting_slot_limit_reached = 0;
/** @var int ERROR_accounting_license_file_not_found */
const ERROR_accounting_license_file_not_found = 0;
/** @var int ERROR_accounting_license_date_not_ok */
const ERROR_accounting_license_date_not_ok = 0;
/** @var int ERROR_accounting_unable_to_connect_to_server */
const ERROR_accounting_unable_to_connect_to_server = 0;
/** @var int ERROR_accounting_unknown_error */
const ERROR_accounting_unknown_error = 0;
/** @var int ERROR_accounting_server_error */
const ERROR_accounting_server_error = 0;
/** @var int ERROR_accounting_instance_limit_reached */
const ERROR_accounting_instance_limit_reached = 0;
/** @var int ERROR_accounting_instance_check_error */
const ERROR_accounting_instance_check_error = 0;
/** @var int ERROR_accounting_license_file_invalid */
const ERROR_accounting_license_file_invalid = 0;
/** @var int ERROR_accounting_running_elsewhere */
const ERROR_accounting_running_elsewhere = 0;
/** @var int ERROR_accounting_instance_duplicated */
const ERROR_accounting_instance_duplicated = 0;
/** @var int ERROR_accounting_already_started */
const ERROR_accounting_already_started = 0;
/** @var int ERROR_accounting_not_started */
const ERROR_accounting_not_started = 0;
/** @var int ERROR_accounting_to_many_starts */
const ERROR_accounting_to_many_starts = 0;
/** @var int ERROR_provisioning_invalid_password */
const ERROR_provisioning_invalid_password = 0;
/** @var int ERROR_provisioning_invalid_request */
const ERROR_provisioning_invalid_request = 0;
/** @var int ERROR_provisioning_no_slots_available */
const ERROR_provisioning_no_slots_available = 0;
/** @var int ERROR_provisioning_pool_missing */
const ERROR_provisioning_pool_missing = 0;
/** @var int ERROR_provisioning_pool_unknown */
const ERROR_provisioning_pool_unknown = 0;
/** @var int ERROR_provisioning_unknown_ip_location */
const ERROR_provisioning_unknown_ip_location = 0;
/** @var int ERROR_provisioning_internal_tries_exceeded */
const ERROR_provisioning_internal_tries_exceeded = 0;
/** @var int ERROR_provisioning_too_many_slots_requested */
const ERROR_provisioning_too_many_slots_requested = 0;
/** @var int ERROR_provisioning_too_many_reserved */
const ERROR_provisioning_too_many_reserved = 0;
/** @var int ERROR_provisioning_could_not_connect */
const ERROR_provisioning_could_not_connect = 0;
/** @var int ERROR_provisioning_auth_server_not_connected */
const ERROR_provisioning_auth_server_not_connected = 0;
/** @var int ERROR_provisioning_auth_data_too_large */
const ERROR_provisioning_auth_data_too_large = 0;
/** @var int ERROR_provisioning_already_initialized */
const ERROR_provisioning_already_initialized = 0;
/** @var int ERROR_provisioning_not_initialized */
const ERROR_provisioning_not_initialized = 0;
/** @var int ERROR_provisioning_connecting */
const ERROR_provisioning_connecting = 0;
/** @var int ERROR_provisioning_already_connected */
const ERROR_provisioning_already_connected = 0;
/** @var int ERROR_provisioning_not_connected */
const ERROR_provisioning_not_connected = 0;
/** @var int ERROR_provisioning_io_error */
const ERROR_provisioning_io_error = 0;
/** @var int ERROR_provisioning_invalid_timeout */
const ERROR_provisioning_invalid_timeout = 0;
/** @var int ERROR_provisioning_ts3server_not_found */
const ERROR_provisioning_ts3server_not_found = 0;
/** @var int ERROR_provisioning_no_permission */
const ERROR_provisioning_no_permission = 0;
/** @var int STATUS_NOT_TALKING */
const STATUS_NOT_TALKING = 0;
/** @var int STATUS_TALKING */
const STATUS_TALKING = 0;
/** @var int STATUS_TALKING_WHILE_DISABLED */
const STATUS_TALKING_WHILE_DISABLED = 0;
/** @var int CODEC_SPEEX_NARROWBAND */
const CODEC_SPEEX_NARROWBAND = 0;
/** @var int CODEC_SPEEX_WIDEBAND */
const CODEC_SPEEX_WIDEBAND = 0;
/** @var int CODEC_SPEEX_ULTRAWIDEBAND */
const CODEC_SPEEX_ULTRAWIDEBAND = 0;
/** @var int CODEC_CELT_MONO */
const CODEC_CELT_MONO = 0;
/** @var int CODEC_OPUS_VOICE */
const CODEC_OPUS_VOICE = 0;
/** @var int CODEC_OPUS_MUSIC */
const CODEC_OPUS_MUSIC = 0;
/** @var int CODEC_ENCRYPTION_PER_CHANNEL */
const CODEC_ENCRYPTION_PER_CHANNEL = 0;
/** @var int CODEC_ENCRYPTION_FORCED_OFF */
const CODEC_ENCRYPTION_FORCED_OFF = 0;
/** @var int CODEC_ENCRYPTION_FORCED_ON */
const CODEC_ENCRYPTION_FORCED_ON = 0;
/** @var int CHANNEL_NAME */
const CHANNEL_NAME = 0;
/** @var int CHANNEL_TOPIC */
const CHANNEL_TOPIC = 0;
/** @var int CHANNEL_DESCRIPTION */
const CHANNEL_DESCRIPTION = 0;
/** @var int CHANNEL_PASSWORD */
const CHANNEL_PASSWORD = 0;
/** @var int CHANNEL_CODEC */
const CHANNEL_CODEC = 0;
/** @var int CHANNEL_CODEC_QUALITY */
const CHANNEL_CODEC_QUALITY = 0;
/** @var int CHANNEL_MAXCLIENTS */
const CHANNEL_MAXCLIENTS = 0;
/** @var int CHANNEL_MAXFAMILYCLIENTS */
const CHANNEL_MAXFAMILYCLIENTS = 0;
/** @var int CHANNEL_ORDER */
const CHANNEL_ORDER = 0;
/** @var int CHANNEL_FLAG_PERMANENT */
const CHANNEL_FLAG_PERMANENT = 0;
/** @var int CHANNEL_FLAG_SEMI_PERMANENT */
const CHANNEL_FLAG_SEMI_PERMANENT = 0;
/** @var int CHANNEL_FLAG_DEFAULT */
const CHANNEL_FLAG_DEFAULT = 0;
/** @var int CHANNEL_FLAG_PASSWORD */
const CHANNEL_FLAG_PASSWORD = 0;
/** @var int CHANNEL_CODEC_LATENCY_FACTOR */
const CHANNEL_CODEC_LATENCY_FACTOR = 0;
/** @var int CHANNEL_CODEC_IS_UNENCRYPTED */
const CHANNEL_CODEC_IS_UNENCRYPTED = 0;
/** @var int CHANNEL_SECURITY_SALT */
const CHANNEL_SECURITY_SALT = 0;
/** @var int CHANNEL_DELETE_DELAY */
const CHANNEL_DELETE_DELAY = 0;
/** @var int CLIENT_UNIQUE_IDENTIFIER */
const CLIENT_UNIQUE_IDENTIFIER = 0;
/** @var int CLIENT_NICKNAME */
const CLIENT_NICKNAME = 0;
/** @var int CLIENT_VERSION */
const CLIENT_VERSION = 0;
/** @var int CLIENT_PLATFORM */
const CLIENT_PLATFORM = 0;
/** @var int CLIENT_FLAG_TALKING */
const CLIENT_FLAG_TALKING = 0;
/** @var int CLIENT_INPUT_MUTED */
const CLIENT_INPUT_MUTED = 0;
/** @var int CLIENT_OUTPUT_MUTED */
const CLIENT_OUTPUT_MUTED = 0;
/** @var int CLIENT_OUTPUTONLY_MUTED */
const CLIENT_OUTPUTONLY_MUTED = 0;
/** @var int CLIENT_INPUT_HARDWARE */
const CLIENT_INPUT_HARDWARE = 0;
/** @var int CLIENT_OUTPUT_HARDWARE */
const CLIENT_OUTPUT_HARDWARE = 0;
/** @var int CLIENT_INPUT_DEACTIVATED */
const CLIENT_INPUT_DEACTIVATED = 0;
/** @var int CLIENT_IDLE_TIME */
const CLIENT_IDLE_TIME = 0;
/** @var int CLIENT_DEFAULT_CHANNEL */
const CLIENT_DEFAULT_CHANNEL = 0;
/** @var int CLIENT_DEFAULT_CHANNEL_PASSWORD */
const CLIENT_DEFAULT_CHANNEL_PASSWORD = 0;
/** @var int CLIENT_SERVER_PASSWORD */
const CLIENT_SERVER_PASSWORD = 0;
/** @var int CLIENT_META_DATA */
const CLIENT_META_DATA = 0;
/** @var int CLIENT_IS_MUTED */
const CLIENT_IS_MUTED = 0;
/** @var int CLIENT_IS_RECORDING */
const CLIENT_IS_RECORDING = 0;
/** @var int CLIENT_VOLUME_MODIFICATOR */
const CLIENT_VOLUME_MODIFICATOR = 0;
/** @var int CLIENT_VERSION_SIGN */
const CLIENT_VERSION_SIGN = 0;
/** @var int CLIENT_SECURITY_HASH */
const CLIENT_SECURITY_HASH = 0;
/** @var int VIRTUALSERVER_UNIQUE_IDENTIFIER */
const VIRTUALSERVER_UNIQUE_IDENTIFIER = 0;
/** @var int VIRTUALSERVER_NAME */
const VIRTUALSERVER_NAME = 0;
/** @var int VIRTUALSERVER_WELCOMEMESSAGE */
const VIRTUALSERVER_WELCOMEMESSAGE = 0;
/** @var int VIRTUALSERVER_PLATFORM */
const VIRTUALSERVER_PLATFORM = 0;
/** @var int VIRTUALSERVER_VERSION */
const VIRTUALSERVER_VERSION = 0;
/** @var int VIRTUALSERVER_MAXCLIENTS */
const VIRTUALSERVER_MAXCLIENTS = 0;
/** @var int VIRTUALSERVER_PASSWORD */
const VIRTUALSERVER_PASSWORD = 0;
/** @var int VIRTUALSERVER_CLIENTS_ONLINE */
const VIRTUALSERVER_CLIENTS_ONLINE = 0;
/** @var int VIRTUALSERVER_CHANNELS_ONLINE */
const VIRTUALSERVER_CHANNELS_ONLINE = 0;
/** @var int VIRTUALSERVER_CREATED */
const VIRTUALSERVER_CREATED = 0;
/** @var int VIRTUALSERVER_UPTIME */
const VIRTUALSERVER_UPTIME = 0;
/** @var int VIRTUALSERVER_CODEC_ENCRYPTION_MODE */
const VIRTUALSERVER_CODEC_ENCRYPTION_MODE = 0;
/** @var int STATUS_DISCONNECTED */
const STATUS_DISCONNECTED = 0;
/** @var int STATUS_CONNECTING */
const STATUS_CONNECTING = 0;
/** @var int STATUS_CONNECTED */
const STATUS_CONNECTED = 0;
/** @var int STATUS_CONNECTION_ESTABLISHING */
const STATUS_CONNECTION_ESTABLISHING = 0;
/** @var int STATUS_CONNECTION_ESTABLISHED */
const STATUS_CONNECTION_ESTABLISHED = 0;
/** @var int TEST_MODE_OFF */
const TEST_MODE_OFF = 0;
/** @var int TEST_MODE_VOICE_LOCAL_ONLY */
const TEST_MODE_VOICE_LOCAL_ONLY = 0;
/** @var int TEST_MODE_VOICE_LOCAL_AND_REMOTE */
const TEST_MODE_VOICE_LOCAL_AND_REMOTE = 0;
/** @var int CONNECTION_PING */
const CONNECTION_PING = 0;
/** @var int CONNECTION_PING_DEVIATION */
const CONNECTION_PING_DEVIATION = 0;
/** @var int CONNECTION_CONNECTED_TIME */
const CONNECTION_CONNECTED_TIME = 0;
/** @var int CONNECTION_IDLE_TIME */
const CONNECTION_IDLE_TIME = 0;
/** @var int CONNECTION_CLIENT_IP */
const CONNECTION_CLIENT_IP = 0;
/** @var int CONNECTION_CLIENT_PORT */
const CONNECTION_CLIENT_PORT = 0;
/** @var int CONNECTION_SERVER_IP */
const CONNECTION_SERVER_IP = 0;
/** @var int CONNECTION_SERVER_PORT */
const CONNECTION_SERVER_PORT = 0;
/** @var int CONNECTION_PACKETS_SENT_SPEECH */
const CONNECTION_PACKETS_SENT_SPEECH = 0;
/** @var int CONNECTION_PACKETS_SENT_KEEPALIVE */
const CONNECTION_PACKETS_SENT_KEEPALIVE = 0;
/** @var int CONNECTION_PACKETS_SENT_CONTROL */
const CONNECTION_PACKETS_SENT_CONTROL = 0;
/** @var int CONNECTION_PACKETS_SENT_TOTAL */
const CONNECTION_PACKETS_SENT_TOTAL = 0;
/** @var int CONNECTION_BYTES_SENT_SPEECH */
const CONNECTION_BYTES_SENT_SPEECH = 0;
/** @var int CONNECTION_BYTES_SENT_KEEPALIVE */
const CONNECTION_BYTES_SENT_KEEPALIVE = 0;
/** @var int CONNECTION_BYTES_SENT_CONTROL */
const CONNECTION_BYTES_SENT_CONTROL = 0;
/** @var int CONNECTION_BYTES_SENT_TOTAL */
const CONNECTION_BYTES_SENT_TOTAL = 0;
/** @var int CONNECTION_PACKETS_RECEIVED_SPEECH */
const CONNECTION_PACKETS_RECEIVED_SPEECH = 0;
/** @var int CONNECTION_PACKETS_RECEIVED_KEEPALIVE */
const CONNECTION_PACKETS_RECEIVED_KEEPALIVE = 0;
/** @var int CONNECTION_PACKETS_RECEIVED_CONTROL */
const CONNECTION_PACKETS_RECEIVED_CONTROL = 0;
/** @var int CONNECTION_PACKETS_RECEIVED_TOTAL */
const CONNECTION_PACKETS_RECEIVED_TOTAL = 0;
/** @var int CONNECTION_BYTES_RECEIVED_SPEECH */
const CONNECTION_BYTES_RECEIVED_SPEECH = 0;
/** @var int CONNECTION_BYTES_RECEIVED_KEEPALIVE */
const CONNECTION_BYTES_RECEIVED_KEEPALIVE = 0;
/** @var int CONNECTION_BYTES_RECEIVED_CONTROL */
const CONNECTION_BYTES_RECEIVED_CONTROL = 0;
/** @var int CONNECTION_BYTES_RECEIVED_TOTAL */
const CONNECTION_BYTES_RECEIVED_TOTAL = 0;
/** @var int CONNECTION_PACKETLOSS_SPEECH */
const CONNECTION_PACKETLOSS_SPEECH = 0;
/** @var int CONNECTION_PACKETLOSS_KEEPALIVE */
const CONNECTION_PACKETLOSS_KEEPALIVE = 0;
/** @var int CONNECTION_PACKETLOSS_CONTROL */
const CONNECTION_PACKETLOSS_CONTROL = 0;
/** @var int CONNECTION_PACKETLOSS_TOTAL */
const CONNECTION_PACKETLOSS_TOTAL = 0;
/** @var int CONNECTION_SERVER2CLIENT_PACKETLOSS_SPEECH */
const CONNECTION_SERVER2CLIENT_PACKETLOSS_SPEECH = 0;
/** @var int CONNECTION_SERVER2CLIENT_PACKETLOSS_KEEPALIVE */
const CONNECTION_SERVER2CLIENT_PACKETLOSS_KEEPALIVE = 0;
/** @var int CONNECTION_SERVER2CLIENT_PACKETLOSS_CONTROL */
const CONNECTION_SERVER2CLIENT_PACKETLOSS_CONTROL = 0;
/** @var int CONNECTION_SERVER2CLIENT_PACKETLOSS_TOTAL */
const CONNECTION_SERVER2CLIENT_PACKETLOSS_TOTAL = 0;
/** @var int CONNECTION_CLIENT2SERVER_PACKETLOSS_SPEECH */
const CONNECTION_CLIENT2SERVER_PACKETLOSS_SPEECH = 0;
/** @var int CONNECTION_CLIENT2SERVER_PACKETLOSS_KEEPALIVE */
const CONNECTION_CLIENT2SERVER_PACKETLOSS_KEEPALIVE = 0;
/** @var int CONNECTION_CLIENT2SERVER_PACKETLOSS_CONTROL */
const CONNECTION_CLIENT2SERVER_PACKETLOSS_CONTROL = 0;
/** @var int CONNECTION_CLIENT2SERVER_PACKETLOSS_TOTAL */
const CONNECTION_CLIENT2SERVER_PACKETLOSS_TOTAL = 0;
/** @var int CONNECTION_BANDWIDTH_SENT_LAST_SECOND_SPEECH */
const CONNECTION_BANDWIDTH_SENT_LAST_SECOND_SPEECH = 0;
/** @var int CONNECTION_BANDWIDTH_SENT_LAST_SECOND_KEEPALIVE */
const CONNECTION_BANDWIDTH_SENT_LAST_SECOND_KEEPALIVE = 0;
/** @var int CONNECTION_BANDWIDTH_SENT_LAST_SECOND_CONTROL */
const CONNECTION_BANDWIDTH_SENT_LAST_SECOND_CONTROL = 0;
/** @var int CONNECTION_BANDWIDTH_SENT_LAST_SECOND_TOTAL */
const CONNECTION_BANDWIDTH_SENT_LAST_SECOND_TOTAL = 0;
/** @var int CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_SPEECH */
const CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_SPEECH = 0;
/** @var int CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_KEEPALIVE */
const CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_KEEPALIVE = 0;
/** @var int CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_CONTROL */
const CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_CONTROL = 0;
/** @var int CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_TOTAL */
const CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_TOTAL = 0;
/** @var int CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_SPEECH */
const CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_SPEECH = 0;
/** @var int CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_KEEPALIVE */
const CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_KEEPALIVE = 0;
/** @var int CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_CONTROL */
const CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_CONTROL = 0;
/** @var int CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_TOTAL */
const CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_TOTAL = 0;
/** @var int CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_SPEECH */
const CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_SPEECH = 0;
/** @var int CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_KEEPALIVE */
const CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_KEEPALIVE = 0;
/** @var int CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_CONTROL */
const CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_CONTROL = 0;
/** @var int CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_TOTAL */
const CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_TOTAL = 0;

?>
