#define TIMEOUT 5

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "php_ts3client.h"
#include "zend_exceptions.h"
#include "ext/standard/info.h"
#include "stdatomic.h"
#include "stdbool.h"
#include "pthread.h"
#include "teamspeak/clientlib.h"
#include "teamspeak/public_errors.h"

struct WaitItem
{
	struct WaitItem *next;
	unsigned int return_code;
	char return_code_text[20];
	unsigned int result;
	bool returned;
	pthread_cond_t cond;
};

enum ConnectState
{
	CONNECT_STATE_NONE,
	CONNECT_STATE_CONNECTING,
	CONNECT_STATE_DISCONNECTING,
};

struct ConnectionItem
{
	struct ConnectionItem *next;
	uint64_t serverConnectionHandlerID;
	_Atomic enum ConnectState expected_state;
	struct WaitItem state_changed;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static struct WaitItem *wait_items = NULL;
static struct ConnectionItem *connection_items = NULL;

void to_asciiz(char** pointer, size_t length)
{
	char* result;
	if (*pointer)
	{
		result  = strndup(*pointer, length);
	}
	else
	{
		result = NULL;
	}
	*pointer = result;
}

struct WaitItem *create_return_code_item()
{
	static atomic_uint next = ATOMIC_VAR_INIT(1);
	struct WaitItem *result = malloc(sizeof(struct WaitItem));
	result->return_code = next++;
	snprintf(result->return_code_text, sizeof(result->return_code_text), "%u", result->return_code);
	result->returned = false;
	pthread_cond_init(&result->cond, NULL);

	pthread_mutex_lock(&mutex);
	result->next = wait_items;
	wait_items = result;
	pthread_mutex_unlock(&mutex);

	return result;
}

struct WaitItem *remove_return_code_item(unsigned int return_code)
{
	pthread_mutex_lock(&mutex);
	struct WaitItem **parent = &wait_items, *item;
	while (true)
	{
		item = *parent;
		if (item == NULL)
		{
			break;
		}
		if (item->return_code == return_code)
		{
			*parent = item->next;
			break;
		}
		parent = &item->next;
	}
	pthread_mutex_unlock(&mutex);
	return item;
}

void free_return_code_item(struct WaitItem* item)
{
	pthread_cond_destroy(&item->cond);
	free(item);
}

struct ConnectionItem* get_connection_item(uint64_t serverConnectionHandlerID)
{
	pthread_mutex_lock(&mutex);
	struct ConnectionItem *item = connection_items;
	while (item != NULL && item->serverConnectionHandlerID != serverConnectionHandlerID)
		item = item->next;

	if (item == NULL)
	{
		item = malloc(sizeof(struct ConnectionItem));
		item->serverConnectionHandlerID = serverConnectionHandlerID;
		item->expected_state = CONNECT_STATE_NONE;
		item->state_changed.returned = false;
		pthread_cond_init(&item->state_changed.cond, NULL);

		item->next = connection_items;
		connection_items = item;
	}
	pthread_mutex_unlock(&mutex);
	return item;
}

void free_connection_item(struct ConnectionItem* item)
{
	pthread_cond_destroy(&item->state_changed.cond);
	free(item);
}

void delete_connection_item(uint64_t serverConnectionHandlerID)
{
	pthread_mutex_lock(&mutex);
	struct ConnectionItem **parent = &connection_items, *item;
	while (true)
	{
		item = *parent;
		if (item == NULL)
		{
			break;
		}
		if (item->serverConnectionHandlerID == serverConnectionHandlerID)
		{
			*parent = item->next;
			free_connection_item(item);
			break;
		}
		parent = &item->next;
	}
	pthread_mutex_unlock(&mutex);
}

void set_result(struct WaitItem *item, unsigned int return_code)
{
	pthread_mutex_lock(&mutex);
	if (item->returned == false)
	{
		item->result = return_code;
		item->returned = true;
		pthread_cond_signal(&item->cond);
	}
	pthread_mutex_unlock(&mutex);
}

void wait_for(struct WaitItem *item)
{
	pthread_mutex_lock(&mutex);
	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += TIMEOUT;

	while (item->returned == false && pthread_cond_timedwait(&item->cond, &mutex, &timeout) == 0);
	if (item->returned)
	{
		item->returned = false;
	}
	else
	{
		item->result = ERROR_connection_lost;
	}

	pthread_mutex_unlock(&mutex);
}

unsigned int handle_return_code(struct WaitItem *item, unsigned int error)
{
	if (error == ERROR_ok)
	{
		wait_for(item);
		error = item->result;
	}
	else
	{
		remove_return_code_item(item->return_code);
	}
	free_return_code_item(item);
	return error;
}

void onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage)
{
	(void)serverConnectionHandlerID;
	(void)errorMessage;
	(void)extraMessage;
	if (returnCode)
	{
		char* endptr;
		long int return_code = strtol(returnCode, &endptr, 10);
		if (return_code > 0)
		{
			struct WaitItem *item = remove_return_code_item(return_code);
			set_result(item, error);
		}
	}
	else
	{
		struct ConnectionItem *item = get_connection_item(serverConnectionHandlerID);
		enum ConnectState expected = atomic_load(&item->expected_state);
		if (expected == CONNECT_STATE_CONNECTING)
			set_result(&item->state_changed, error);
	}
}

void onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber)
{
	struct ConnectionItem *item = get_connection_item(serverConnectionHandlerID);
	const bool connected = newStatus == STATUS_CONNECTION_ESTABLISHED;
	const bool disconnected = newStatus == STATUS_DISCONNECTED;
	if (!connected && !disconnected)
		return;

	switch (atomic_load(&item->expected_state))
	{
		case CONNECT_STATE_NONE: return;
		case CONNECT_STATE_CONNECTING:
			if (disconnected && errorNumber == ERROR_ok)
				errorNumber = ERROR_undefined;
			break;
		case CONNECT_STATE_DISCONNECTING:
			if (connected && errorNumber == ERROR_ok)
				errorNumber = ERROR_undefined;
			break;
	}
	set_result(&item->state_changed, errorNumber);
}

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getClientLibVersion, 0)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getClientLibVersionNumber, 0)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_spawnNewServerConnectionHandler, 0)
	ZEND_ARG_INFO(0, port)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_destroyServerConnectionHandler, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_createIdentity, 0)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_identityStringToUniqueIdentifier, 0)
	ZEND_ARG_INFO(0, identityString)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getErrorMessage, 0)
	ZEND_ARG_INFO(0, errorCode)
	ZEND_ARG_INFO(1, error)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_startConnection, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, identity)
	ZEND_ARG_INFO(0, ip)
	ZEND_ARG_INFO(0, port)
	ZEND_ARG_INFO(0, nickname)
	ZEND_ARG_INFO(0, defaultChannelID)
	ZEND_ARG_INFO(0, defaultChannelPassword)
	ZEND_ARG_INFO(0, serverPassword)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_stopConnection, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, quitMessage)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestClientMove, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
	ZEND_ARG_INFO(0, newChannelID)
	ZEND_ARG_INFO(0, password)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestClientVariables, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestClientKickFromChannel, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
	ZEND_ARG_INFO(0, kickReason)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestClientKickFromServer, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
	ZEND_ARG_INFO(0, kickReason)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestChannelDelete, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelID)
	ZEND_ARG_INFO(0, force)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestChannelMove, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelID)
	ZEND_ARG_INFO(0, newChannelParentID)
	ZEND_ARG_INFO(0, newChannelOrder)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestSendPrivateTextMsg, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, message)
	ZEND_ARG_INFO(0, targetClientID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestSendChannelTextMsg, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, message)
	ZEND_ARG_INFO(0, targetChannelID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestSendServerTextMsg, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestConnectionInfo, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestChannelSubscribeAll, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestChannelUnsubscribeAll, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getClientID, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getConnectionStatus, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
    ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getConnectionVariableAsUInt64, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
	ZEND_ARG_INFO(0, flag)
    ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getConnectionVariableAsDouble, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
	ZEND_ARG_INFO(0, flag)
    ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getConnectionVariableAsString, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
	ZEND_ARG_INFO(0, flag)
    ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(arginfo_ts3client_cleanUpConnectionInfo, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestServerConnectionInfo, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getServerConnectionVariableAsUInt64, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
    ZEND_ARG_INFO(0, flag)
    ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getServerConnectionVariableAsFloat, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
    ZEND_ARG_INFO(0, flag)
    ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getClientSelfVariableAsInt, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
    ZEND_ARG_INFO(0, flag)
    ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getClientSelfVariableAsString, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
    ZEND_ARG_INFO(0, flag)
    ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(arginfo_ts3client_setClientSelfVariableAsInt, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
    ZEND_ARG_INFO(0, flag)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_setClientSelfVariableAsString, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
    ZEND_ARG_INFO(0, flag)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_flushClientSelfUpdates, 0)
    ZEND_ARG_INFO(0, serverConnectionHandlerID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getClientVariableAsInt, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getClientVariableAsUInt64, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getClientVariableAsString, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getClientList, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getChannelOfClient, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, clientID)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getChannelVariableAsInt, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelID)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getChannelVariableAsUInt64, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelID)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getChannelVariableAsString, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelID)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_setChannelVariableAsInt, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelID)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_setChannelVariableAsUInt64, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelID)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_setChannelVariableAsString, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelID)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_flushChannelUpdates, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_flushChannelCreation, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelParentID)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getChannelList, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getChannelClientList, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelID)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getParentChannelOfChannel, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelID)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getChannelEmptySecs, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, channelID)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getServerVariableAsInt, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getServerVariableAsUInt64, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_getServerVariableAsString, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
	ZEND_ARG_INFO(0, flag)
	ZEND_ARG_INFO(1, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_ts3client_requestServerVariables, 0)
	ZEND_ARG_INFO(0, serverConnectionHandlerID)
ZEND_END_ARG_INFO()

PHP_FUNCTION(ts3client_getClientLibVersion)
{
	char *result;
	unsigned int error = ts3client_getClientLibVersion(&result);
	if (error == ERROR_ok)
	{
		zval *zresult;
		if (zend_parse_parameters(ZEND_NUM_ARGS(), "z/", &zresult) == FAILURE)
			return;
		zval_dtor(zresult);
		ZVAL_STRING(zresult, result);
		ts3client_freeMemory(result);
	}
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getClientLibVersionNumber)
{
	uint64_t result;
	unsigned int error = ts3client_getClientLibVersionNumber(&result);
	if (error == ERROR_ok)
	{
		zval *zresult;
		if (zend_parse_parameters(ZEND_NUM_ARGS(), "z/", &zresult) == FAILURE)
			return;
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_spawnNewServerConnectionHandler)
{
	zend_long port;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz/", &port, &zresult) == FAILURE)
		return;
	uint64_t result;
	unsigned int error = ts3client_spawnNewServerConnectionHandler(port, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_destroyServerConnectionHandler)
{
	zend_long serverConnectionHandlerID;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &serverConnectionHandlerID) == FAILURE)
		return;
	unsigned int error = ts3client_destroyServerConnectionHandler(serverConnectionHandlerID);
	if (error == ERROR_ok)
		delete_connection_item(serverConnectionHandlerID);
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_createIdentity)
{
	char *identity;
	unsigned int error = ts3client_createIdentity(&identity);
	if (error== ERROR_ok)
	{
		zval *zidentity;
		if (zend_parse_parameters(ZEND_NUM_ARGS(), "z/", &zidentity) == FAILURE)
			return;
		zval_dtor(zidentity);
		ZVAL_STRING(zidentity, identity);
		ts3client_freeMemory(identity);
	}
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_identityStringToUniqueIdentifier)
{
	char *identityString, *result;
	size_t identityString_len;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz/", &identityString, &identityString_len, &zresult) == FAILURE)
		return;
	to_asciiz(&identityString, identityString_len);
	unsigned int error = ts3client_identityStringToUniqueIdentifier(identityString, &result);
	free(identityString);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_STRING(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getErrorMessage)
{
	zend_long errorCode;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz/", &errorCode, &zresult) == FAILURE)
		return;
	char *result;
	unsigned int error = ts3client_getErrorMessage(errorCode, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_STRING(zresult, result);
		ts3client_freeMemory(result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_startConnection)
{
	zend_long serverConnectionHandlerID;
	char* identity;               size_t identity_len;
	char* ip;                     size_t ip_len;
	zend_long port;
	char* nickname;               size_t nickname_len;
	zend_long defaultChannelID;
	char* defaultChannelPassword; size_t defaultChannelPassword_len;
	char* serverPassword;         size_t serverPassword_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lsslslss",
				&serverConnectionHandlerID,
				&identity, &identity_len,
			   	&ip, &ip_len,
				&port,
				&nickname, &nickname_len,
				&defaultChannelID,
				&defaultChannelPassword, &defaultChannelPassword_len,
				&serverPassword, &serverPassword_len)
			== FAILURE)
		return;
	to_asciiz(&identity, identity_len);
	to_asciiz(&ip, ip_len);
	to_asciiz(&nickname, nickname_len);
	to_asciiz(&defaultChannelPassword, defaultChannelPassword_len);
	to_asciiz(&serverPassword, serverPassword_len);

	struct ConnectionItem* connection_item = get_connection_item(serverConnectionHandlerID);
	enum ConnectState expected = CONNECT_STATE_NONE;
	if (atomic_compare_exchange_strong(&connection_item->expected_state, &expected, CONNECT_STATE_CONNECTING) == false)
		RETURN_LONG(ERROR_currently_not_possible);

	unsigned int error = ts3client_startConnectionWithChannelID(
			serverConnectionHandlerID,
			identity,
			ip,
			port,
			nickname,
			defaultChannelID,
			defaultChannelPassword,
			serverPassword);

	free(identity);
	free(ip);
	free(nickname);
	free(defaultChannelPassword);
	free(serverPassword);

	if (error == ERROR_ok)
	{
		wait_for(&connection_item->state_changed);
		RETVAL_LONG(connection_item->state_changed.result);
	}
	else
	{
		RETVAL_LONG(error)
	}

	atomic_exchange(&connection_item->expected_state, CONNECT_STATE_NONE);
}

PHP_FUNCTION(ts3client_stopConnection)
{
	zend_long serverConnectionHandlerID;
	char* reason; size_t reason_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ls", &serverConnectionHandlerID, &reason, &reason_len) == FAILURE)
		return;
	to_asciiz(&reason, reason_len);

	struct ConnectionItem* connection_item = get_connection_item(serverConnectionHandlerID);
	enum ConnectState expected = CONNECT_STATE_NONE;
	if (atomic_compare_exchange_strong(&connection_item->expected_state, &expected, CONNECT_STATE_DISCONNECTING) == false)
		RETURN_LONG(ERROR_currently_not_possible);

	unsigned int error = ts3client_stopConnection(serverConnectionHandlerID, reason);

	free(reason);

	if (error == ERROR_ok)
	{
		wait_for(&connection_item->state_changed);
		RETVAL_LONG(connection_item->state_changed.result);
	}
	else
	{
		RETVAL_LONG(error)
	}

	atomic_exchange(&connection_item->expected_state, CONNECT_STATE_NONE);
}

PHP_FUNCTION(ts3client_requestClientMove)
{
	zend_long serverConnectionHandlerID;
	zend_long clientID;
	zend_long newChannelID;
	char* password; size_t password_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llls", &serverConnectionHandlerID, &clientID, &newChannelID, &password, &password_len) == FAILURE)
		return;
	to_asciiz(&password, password_len);
	struct WaitItem* item = create_return_code_item();
	unsigned int error = ts3client_requestClientMove(serverConnectionHandlerID, clientID, newChannelID, password, item->return_code_text);
	free(password);
	RETURN_LONG(handle_return_code(item, error));
}

PHP_FUNCTION(ts3client_requestClientVariables)
{
	zend_long serverConnectionHandlerID;
	zend_long clientID;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &serverConnectionHandlerID, &clientID) == FAILURE)
		return;
	struct WaitItem* item = create_return_code_item();
	unsigned int error = ts3client_requestClientVariables(serverConnectionHandlerID, clientID, item->return_code_text);
	RETURN_LONG(handle_return_code(item, error));
}

PHP_FUNCTION(ts3client_requestClientKickFromChannel)
{
	zend_long serverConnectionHandlerID;
	zend_long clientID;
	char* kickReason; size_t kickReason_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lls", &serverConnectionHandlerID, &clientID, &kickReason, &kickReason_len) == FAILURE)
		return;
	to_asciiz(&kickReason, kickReason_len);
	struct WaitItem* item = create_return_code_item();
	unsigned int error = ts3client_requestClientKickFromChannel(serverConnectionHandlerID, clientID, kickReason, item->return_code_text);
	free(kickReason);
	RETURN_LONG(handle_return_code(item, error));
}

PHP_FUNCTION(ts3client_requestClientKickFromServer)
{
	zend_long serverConnectionHandlerID;
	zend_long clientID;
	char* kickReason; size_t kickReason_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lls", &serverConnectionHandlerID, &clientID, &kickReason, &kickReason_len) == FAILURE)
		return;
	to_asciiz(&kickReason, kickReason_len);
	struct WaitItem* item = create_return_code_item();
	unsigned int error = ts3client_requestClientKickFromServer(serverConnectionHandlerID, clientID, kickReason, item->return_code_text);
	free(kickReason);
	RETURN_LONG(handle_return_code(item, error));
}

PHP_FUNCTION(ts3client_requestChannelDelete)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	zend_bool force;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llb", &serverConnectionHandlerID, &channelID, &force) == FAILURE)
		return;
	struct WaitItem* item = create_return_code_item();
	unsigned int error = ts3client_requestChannelDelete(serverConnectionHandlerID, channelID, force, item->return_code_text);
	RETURN_LONG(handle_return_code(item, error));
}

PHP_FUNCTION(ts3client_requestChannelMove)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	zend_long newChannelParentID;
	zend_long newChannelOrder;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llll", &serverConnectionHandlerID, &channelID, &newChannelParentID, &newChannelOrder) == FAILURE)
		return;
	struct WaitItem* item = create_return_code_item();
	unsigned int error = ts3client_requestChannelMove(serverConnectionHandlerID, channelID, newChannelParentID, newChannelOrder, item->return_code_text);
	RETURN_LONG(handle_return_code(item, error))
}

PHP_FUNCTION(ts3client_requestSendPrivateTextMsg)
{
	zend_long serverConnectionHandlerID;
	char* message; size_t message_len;
	zend_long targetClientID;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lsl", &serverConnectionHandlerID, &message, &message_len, &targetClientID) == FAILURE)
		return;
	to_asciiz(&message, message_len);
	unsigned int error = ts3client_requestSendPrivateTextMsg(serverConnectionHandlerID, message, targetClientID, NULL);
	free(message);
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_requestSendChannelTextMsg)
{
	zend_long serverConnectionHandlerID;
	char* message; size_t message_len;
	zend_long targetChannelID;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lsl", &serverConnectionHandlerID, &message, &message_len, &targetChannelID) == FAILURE)
		return;
	to_asciiz(&message, message_len);
	unsigned int error = ts3client_requestSendChannelTextMsg(serverConnectionHandlerID, message, targetChannelID, NULL);
	free(message);
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_requestSendServerTextMsg)
{
	zend_long serverConnectionHandlerID;
	char* message; size_t message_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ls", &serverConnectionHandlerID, &message, &message_len) == FAILURE)
		return;
	to_asciiz(&message, message_len);
	unsigned int error = ts3client_requestSendServerTextMsg(serverConnectionHandlerID, message, NULL);
	free(message);
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_requestConnectionInfo)
{
	zend_long serverConnectionHandlerID;
	zend_long clientID;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &serverConnectionHandlerID, &clientID) == FAILURE)
		return;
	struct WaitItem* item = create_return_code_item();
	unsigned int error = ts3client_requestConnectionInfo(serverConnectionHandlerID, clientID, item->return_code_text);
	RETURN_LONG(handle_return_code(item, error))
}

PHP_FUNCTION(ts3client_getConnectionStatus)
{
	zend_long serverConnectionHandlerID;
	zval* zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz/", &serverConnectionHandlerID, &zresult) == FAILURE)
		return;
	int result;
	unsigned int error = ts3client_getConnectionStatus(serverConnectionHandlerID, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_requestChannelSubscribeAll)
{
	zend_long serverConnectionHandlerID;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &serverConnectionHandlerID) == FAILURE)
		return;
	struct WaitItem* item = create_return_code_item();
	unsigned int error = ts3client_requestChannelSubscribeAll(serverConnectionHandlerID, item->return_code_text);
	RETURN_LONG(handle_return_code(item, error))
}

PHP_FUNCTION(ts3client_requestChannelUnsubscribeAll)
{
	zend_long serverConnectionHandlerID;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &serverConnectionHandlerID) == FAILURE)
		return;
	struct WaitItem* item = create_return_code_item();
	unsigned int error = ts3client_requestChannelUnsubscribeAll(serverConnectionHandlerID, item->return_code_text);
	RETURN_LONG(handle_return_code(item, error))
}

PHP_FUNCTION(ts3client_getClientID)
{
	zend_long serverConnectionHandlerID;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz/", &serverConnectionHandlerID, &zresult) == FAILURE)
		return;
	anyID result;
	unsigned int error = ts3client_getClientID(serverConnectionHandlerID, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getConnectionVariableAsUInt64)
{
    zend_long serverConnectionHandlerID;
	zend_long clientID;
	zend_long flag;
	zval *zresult;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "lllz/", &serverConnectionHandlerID, &clientID, &flag, &zresult) == FAILURE)
        return;
	uint64_t result;
    unsigned int error = ts3client_getConnectionVariableAsUInt64(serverConnectionHandlerID, clientID, flag, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getConnectionVariableAsDouble)
{
    zend_long serverConnectionHandlerID;
	zend_long clientID;
	zend_long flag;
	zval *zresult;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "lllz/", &serverConnectionHandlerID, &clientID, &flag, &zresult) == FAILURE)
        return;
	double result;
    unsigned int error = ts3client_getConnectionVariableAsDouble(serverConnectionHandlerID, clientID, flag, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_DOUBLE(zresult, result);
	}
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getConnectionVariableAsString)
{
    zend_long serverConnectionHandlerID;
	zend_long clientID;
	zend_long flag;
	zval *zresult;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "lllz/", &serverConnectionHandlerID, &clientID, &flag, &zresult) == FAILURE)
        return;
	char* result;
    unsigned int error = ts3client_getConnectionVariableAsString(serverConnectionHandlerID, clientID, flag, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_STRING(zresult, result);
		ts3client_freeMemory(result);
	}
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_cleanUpConnectionInfo)
{
    zend_long serverConnectionHandlerID;
	zend_long clientID;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &serverConnectionHandlerID, &clientID) == FAILURE)
        return;
    unsigned int error = ts3client_cleanUpConnectionInfo(serverConnectionHandlerID, clientID);
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_requestServerConnectionInfo)
{
    zend_long serverConnectionHandlerID;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &serverConnectionHandlerID) == FAILURE)
        return;
	struct WaitItem* item = create_return_code_item();
    unsigned int error = ts3client_requestServerConnectionInfo(serverConnectionHandlerID, item->return_code_text);
	RETURN_LONG(handle_return_code(item, error))
}

PHP_FUNCTION(ts3client_getServerConnectionVariableAsUInt64)
{
    zend_long serverConnectionHandlerID;
	zend_long flag;
	zval *zresult;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "llz/", &serverConnectionHandlerID, &flag, &zresult) == FAILURE)
        return;
	uint64_t result;
    unsigned int error = ts3client_getServerConnectionVariableAsUInt64(serverConnectionHandlerID, flag, &result);
	if (error != ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getServerConnectionVariableAsFloat)
{
    zend_long serverConnectionHandlerID;
	zend_long flag;
	zval *zresult;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "llz/", &serverConnectionHandlerID, &flag, &zresult) == FAILURE)
        return;
	float result;
    unsigned int error = ts3client_getServerConnectionVariableAsFloat(serverConnectionHandlerID, flag, &result);
	if (error != ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_DOUBLE(zresult, result);
	}
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getClientSelfVariableAsInt)
{
    zend_long serverConnectionHandlerID;
	zend_long flag;
	zval *zresult;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "llz/", &serverConnectionHandlerID, &flag, &zresult) == FAILURE)
        return;
	int result;
    unsigned int error = ts3client_getClientSelfVariableAsInt(serverConnectionHandlerID, flag, &result);
	if (error != ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getClientSelfVariableAsString)
{
    zend_long serverConnectionHandlerID;
	zend_long flag;
	zval *zresult;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "llz/", &serverConnectionHandlerID, &flag, &zresult) == FAILURE)
        return;
	char *result;
    unsigned int error = ts3client_getClientSelfVariableAsString(serverConnectionHandlerID, flag, &result);
	if (error != ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_STRING(zresult, result);
		ts3client_freeMemory(result);
	}
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_setClientSelfVariableAsInt)
{
    zend_long serverConnectionHandlerID;
	zend_long flag;
	zend_long value;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "lll", &serverConnectionHandlerID, flag, value) == FAILURE)
        return;
    unsigned int error = ts3client_setClientSelfVariableAsInt(serverConnectionHandlerID, flag, value);
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_setClientSelfVariableAsString)
{
    zend_long serverConnectionHandlerID;
	zend_long flag;
	char* value; size_t value_len;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "lls", &serverConnectionHandlerID, &flag, &value, &value_len) == FAILURE)
        return;
	to_asciiz(&value, value_len);
    unsigned int error = ts3client_setClientSelfVariableAsString(serverConnectionHandlerID, flag, value);
	free(value);
    RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_flushClientSelfUpdates)
{
    zend_long serverConnectionHandlerID;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &serverConnectionHandlerID) == FAILURE)
        return;
	struct WaitItem* item = create_return_code_item();
    unsigned int error = ts3client_flushClientSelfUpdates(serverConnectionHandlerID, item->return_code_text);
	RETURN_LONG(handle_return_code(item, error))
}

PHP_FUNCTION(ts3client_getClientVariableAsInt)
{
	zend_long serverConnectionHandlerID;
	zend_long clientID;
	zend_long flag;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lllz/", &serverConnectionHandlerID, &clientID, &flag, &zresult) == FAILURE)
		return;
	int result;
	unsigned int error = ts3client_getClientVariableAsInt(serverConnectionHandlerID, clientID, flag, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getClientVariableAsUInt64)
{
	zend_long serverConnectionHandlerID;
	zend_long clientID;
	zend_long flag;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lllz/", &serverConnectionHandlerID, &clientID, &flag, &zresult) == FAILURE)
		return;
	uint64_t result;
	unsigned int error = ts3client_getClientVariableAsUInt64(serverConnectionHandlerID, clientID, flag, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getClientVariableAsString)
{
	zend_long serverConnectionHandlerID;
	zend_long clientID;
	zend_long flag;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lllz/", &serverConnectionHandlerID, &clientID, &flag, &zresult) == FAILURE)
		return;
	char *result;
	unsigned int error = ts3client_getClientVariableAsString(serverConnectionHandlerID, clientID, flag, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_STRING(zresult, result);
		ts3client_freeMemory(result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getClientList)
{
	zend_long serverConnectionHandlerID;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz/", &serverConnectionHandlerID, &zresult) == FAILURE)
		return;
	anyID* result;
	unsigned int error = ts3client_getClientList(serverConnectionHandlerID, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		array_init(zresult);
		for  (anyID* p = result; *p; ++p)
		{
			add_next_index_long(zresult, *p);
		}
		ts3client_freeMemory(result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getChannelOfClient)
{
	zend_long serverConnectionHandlerID = 25;
	zend_long clientID;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llz/", &serverConnectionHandlerID, &clientID, &zresult) == FAILURE)
		return;
	uint64_t result;
	unsigned int error = ts3client_getChannelOfClient(serverConnectionHandlerID, clientID, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getChannelVariableAsInt)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	zend_long flag;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lllz/", &serverConnectionHandlerID, &channelID, &flag, &zresult) == FAILURE)
		return;
	int result;
	unsigned int error = ts3client_getChannelVariableAsInt(serverConnectionHandlerID, channelID, flag, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getChannelVariableAsUInt64)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	zend_long flag;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lllz/", &serverConnectionHandlerID, &channelID, &flag, &zresult) == FAILURE)
		return;
	uint64_t result;
	unsigned int error = ts3client_getChannelVariableAsUInt64(serverConnectionHandlerID, channelID, flag, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getChannelVariableAsString)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	zend_long flag;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lllz/", &serverConnectionHandlerID, &channelID, &flag, &zresult) == FAILURE)
		return;
	char *result;
	unsigned int error = ts3client_getChannelVariableAsString(serverConnectionHandlerID, channelID, flag, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_STRING(zresult, result);
		ts3client_freeMemory(result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_setChannelVariableAsInt)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	zend_long flag;
	zend_long value;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llll", &serverConnectionHandlerID, &channelID, &flag, &value) == FAILURE)
		return;
	unsigned int error = ts3client_setChannelVariableAsInt(serverConnectionHandlerID, channelID, flag, value);
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_setChannelVariableAsUInt64)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	zend_long flag;
	zend_long value;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llll", &serverConnectionHandlerID, &channelID, &flag, &value) == FAILURE)
		return;
	unsigned int error = ts3client_setChannelVariableAsUInt64(serverConnectionHandlerID, channelID, flag, value);
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_setChannelVariableAsString)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	zend_long flag;
	char *value; size_t value_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llls", &serverConnectionHandlerID, &channelID, &flag, &value, &value_len) == FAILURE)
		return;
	to_asciiz(&value, value_len);
	unsigned int error = ts3client_setChannelVariableAsString(serverConnectionHandlerID, channelID, flag, value);
	free(value);
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_flushChannelUpdates)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &serverConnectionHandlerID, &channelID) == FAILURE)
		return;
	struct WaitItem* item = create_return_code_item();
	unsigned int error = ts3client_flushChannelUpdates(serverConnectionHandlerID, channelID, item->return_code_text);
	RETURN_LONG(handle_return_code(item, error));
}

PHP_FUNCTION(ts3client_flushChannelCreation)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &serverConnectionHandlerID, &channelID) == FAILURE)
		return;
	struct WaitItem* item = create_return_code_item();
	unsigned int error = ts3client_flushChannelCreation(serverConnectionHandlerID, channelID, item->return_code_text);
	RETURN_LONG(handle_return_code(item, error));
}

PHP_FUNCTION(ts3client_getChannelList)
{
	zend_long serverConnectionHandlerID;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz/", &serverConnectionHandlerID, &zresult) == FAILURE)
		return;
	uint64_t* result;
	unsigned int error = ts3client_getChannelList(serverConnectionHandlerID, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		array_init(zresult);
		for  (uint64_t* p = result; *p; ++p)
		{
			add_next_index_long(zresult, *p);
		}
		ts3client_freeMemory(result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getChannelClientList)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llz/", &serverConnectionHandlerID, &channelID, &zresult) == FAILURE)
		return;
	anyID* result;
	unsigned int error = ts3client_getChannelClientList(serverConnectionHandlerID, channelID, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		array_init(zresult);
		for  (anyID* p = result; *p; ++p)
		{
			add_next_index_long(zresult, *p);
		}
		ts3client_freeMemory(result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getParentChannelOfChannel)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llz/", &serverConnectionHandlerID, &channelID, &zresult) == FAILURE)
		return;
	uint64_t result;
	unsigned int error = ts3client_getParentChannelOfChannel(serverConnectionHandlerID, channelID, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getChannelEmptySecs)
{
	zend_long serverConnectionHandlerID;
	zend_long channelID;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llz/", &serverConnectionHandlerID, &channelID, &zresult) == FAILURE)
		return;
	int result;
	unsigned int error = ts3client_getChannelEmptySecs(serverConnectionHandlerID, channelID, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getServerVariableAsInt)
{
	zend_long serverConnectionHandlerID;
	zend_long flag;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llz/", &serverConnectionHandlerID, &flag, &zresult) == FAILURE)
		return;
	int result;
	unsigned int error = ts3client_getServerVariableAsInt(serverConnectionHandlerID, flag, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getServerVariableAsUInt64)
{
	zend_long serverConnectionHandlerID;
	zend_long flag;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llz/", &serverConnectionHandlerID, &flag, &zresult) == FAILURE)
		return;
	uint64_t result;
	unsigned int error = ts3client_getServerVariableAsUInt64(serverConnectionHandlerID, flag, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_LONG(zresult, result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_getServerVariableAsString)
{
	zend_long serverConnectionHandlerID;
	zend_long flag;
	zval *zresult;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llz/", &serverConnectionHandlerID, &flag, &zresult) == FAILURE)
		return;
	char *result;
	unsigned int error = ts3client_getServerVariableAsString(serverConnectionHandlerID, flag, &result);
	if (error == ERROR_ok)
	{
		zval_dtor(zresult);
		ZVAL_STRING(zresult, result);
		ts3client_freeMemory(result);
	}
	RETURN_LONG(error);
}

PHP_FUNCTION(ts3client_requestServerVariables)
{
	zend_long serverConnectionHandlerID;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &serverConnectionHandlerID) == FAILURE)
		return;
	unsigned int error = ts3client_requestServerVariables(serverConnectionHandlerID);
	RETURN_LONG(error);
}

zend_function_entry ts3client_functions[] =
{
	PHP_FE(ts3client_getClientLibVersion, arginfo_ts3client_getClientLibVersion)
	PHP_FE(ts3client_getClientLibVersionNumber, arginfo_ts3client_getClientLibVersionNumber)
	PHP_FE(ts3client_spawnNewServerConnectionHandler, arginfo_ts3client_spawnNewServerConnectionHandler)
	PHP_FE(ts3client_destroyServerConnectionHandler, arginfo_ts3client_destroyServerConnectionHandler)
	PHP_FE(ts3client_createIdentity, arginfo_ts3client_createIdentity)
	PHP_FE(ts3client_identityStringToUniqueIdentifier, arginfo_ts3client_identityStringToUniqueIdentifier)
	PHP_FE(ts3client_getErrorMessage, arginfo_ts3client_getErrorMessage)
	PHP_FE(ts3client_startConnection, arginfo_ts3client_startConnection)
	PHP_FE(ts3client_stopConnection, arginfo_ts3client_stopConnection)
	PHP_FE(ts3client_requestClientMove, arginfo_ts3client_requestClientMove)
	PHP_FE(ts3client_requestClientVariables, arginfo_ts3client_requestClientVariables)
	PHP_FE(ts3client_requestClientKickFromChannel, arginfo_ts3client_requestClientKickFromChannel)
	PHP_FE(ts3client_requestClientKickFromServer, arginfo_ts3client_requestClientKickFromServer)
	PHP_FE(ts3client_requestChannelDelete, arginfo_ts3client_requestChannelDelete)
	PHP_FE(ts3client_requestChannelMove, arginfo_ts3client_requestChannelMove)
	PHP_FE(ts3client_requestSendPrivateTextMsg, arginfo_ts3client_requestSendPrivateTextMsg)
	PHP_FE(ts3client_requestSendChannelTextMsg, arginfo_ts3client_requestSendChannelTextMsg)
	PHP_FE(ts3client_requestSendServerTextMsg, arginfo_ts3client_requestSendServerTextMsg)
	PHP_FE(ts3client_requestConnectionInfo, arginfo_ts3client_requestConnectionInfo)
	PHP_FE(ts3client_requestChannelSubscribeAll, arginfo_ts3client_requestChannelSubscribeAll)
	PHP_FE(ts3client_requestChannelUnsubscribeAll, arginfo_ts3client_requestChannelUnsubscribeAll)
	PHP_FE(ts3client_getClientID, arginfo_ts3client_getClientID)
	PHP_FE(ts3client_getConnectionStatus, arginfo_ts3client_getConnectionStatus)
	PHP_FE(ts3client_getConnectionVariableAsUInt64, arginfo_ts3client_getConnectionVariableAsUInt64)
	PHP_FE(ts3client_getConnectionVariableAsDouble, arginfo_ts3client_getConnectionVariableAsDouble)
	PHP_FE(ts3client_getConnectionVariableAsString, arginfo_ts3client_getConnectionVariableAsString)
	PHP_FE(ts3client_cleanUpConnectionInfo, arginfo_ts3client_cleanUpConnectionInfo)
	PHP_FE(ts3client_requestServerConnectionInfo, arginfo_ts3client_requestServerConnectionInfo)
	PHP_FE(ts3client_getServerConnectionVariableAsUInt64, arginfo_ts3client_getServerConnectionVariableAsUInt64)
	PHP_FE(ts3client_getServerConnectionVariableAsFloat, arginfo_ts3client_getServerConnectionVariableAsFloat)
	PHP_FE(ts3client_getClientSelfVariableAsInt, arginfo_ts3client_getClientSelfVariableAsInt)
	PHP_FE(ts3client_getClientSelfVariableAsString, arginfo_ts3client_getClientSelfVariableAsString)
	PHP_FE(ts3client_setClientSelfVariableAsInt, arginfo_ts3client_setClientSelfVariableAsInt)
	PHP_FE(ts3client_setClientSelfVariableAsString, arginfo_ts3client_setClientSelfVariableAsString)
	PHP_FE(ts3client_flushClientSelfUpdates, arginfo_ts3client_flushClientSelfUpdates)
	PHP_FE(ts3client_getClientVariableAsInt, arginfo_ts3client_getClientVariableAsInt)
	PHP_FE(ts3client_getClientVariableAsUInt64, arginfo_ts3client_getClientVariableAsUInt64)
	PHP_FE(ts3client_getClientVariableAsString, arginfo_ts3client_getClientVariableAsString)
	PHP_FE(ts3client_getClientList, arginfo_ts3client_getClientList)
	PHP_FE(ts3client_getChannelOfClient, arginfo_ts3client_getChannelOfClient)
	PHP_FE(ts3client_getChannelVariableAsInt, arginfo_ts3client_getChannelVariableAsInt)
	PHP_FE(ts3client_getChannelVariableAsUInt64, arginfo_ts3client_getChannelVariableAsUInt64)
	PHP_FE(ts3client_getChannelVariableAsString, arginfo_ts3client_getChannelVariableAsString)
	PHP_FE(ts3client_setChannelVariableAsInt, arginfo_ts3client_setChannelVariableAsInt)
	PHP_FE(ts3client_setChannelVariableAsUInt64, arginfo_ts3client_setChannelVariableAsUInt64)
	PHP_FE(ts3client_setChannelVariableAsString, arginfo_ts3client_setChannelVariableAsString)
	PHP_FE(ts3client_flushChannelUpdates, arginfo_ts3client_flushChannelUpdates)
	PHP_FE(ts3client_flushChannelCreation, arginfo_ts3client_flushChannelCreation)
	PHP_FE(ts3client_getChannelList, arginfo_ts3client_getChannelList)
	PHP_FE(ts3client_getChannelClientList, arginfo_ts3client_getChannelClientList)
	PHP_FE(ts3client_getParentChannelOfChannel, arginfo_ts3client_getParentChannelOfChannel)
	PHP_FE(ts3client_getChannelEmptySecs, arginfo_ts3client_getChannelEmptySecs)
	PHP_FE(ts3client_getServerVariableAsInt, arginfo_ts3client_getServerVariableAsInt)
	PHP_FE(ts3client_getServerVariableAsUInt64, arginfo_ts3client_getServerVariableAsUInt64)
	PHP_FE(ts3client_getServerVariableAsString, arginfo_ts3client_getServerVariableAsString)
	PHP_FE(ts3client_requestServerVariables, arginfo_ts3client_requestServerVariables)
	PHP_FE_END
};

atomic_uint instance_count = ATOMIC_VAR_INIT(0);


PHP_MINIT_FUNCTION(ts3client)
{
	if (instance_count++ == 0)
	{
		struct ClientUIFunctions funcs;
		memset(&funcs, 0, sizeof(funcs));
        funcs.onConnectStatusChangeEvent    = onConnectStatusChangeEvent;
        funcs.onServerErrorEvent            = onServerErrorEvent;
		unsigned int error  = ts3client_initClientLib(&funcs, NULL, LogType_NONE, NULL, NULL);
		if (error != ERROR_ok) return FAILURE;
	}

	REGISTER_LONG_CONSTANT("ERROR_ok", ERROR_ok, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_undefined", ERROR_undefined, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_not_implemented", ERROR_not_implemented, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_ok_no_update", ERROR_ok_no_update, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_dont_notify", ERROR_dont_notify, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_lib_time_limit_reached", ERROR_lib_time_limit_reached, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_command_not_found", ERROR_command_not_found, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_unable_to_bind_network_port", ERROR_unable_to_bind_network_port, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_no_network_port_available", ERROR_no_network_port_available, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_port_already_in_use", ERROR_port_already_in_use, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_invalid_id", ERROR_client_invalid_id, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_nickname_inuse", ERROR_client_nickname_inuse, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_protocol_limit_reached", ERROR_client_protocol_limit_reached, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_invalid_type", ERROR_client_invalid_type, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_already_subscribed", ERROR_client_already_subscribed, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_not_logged_in", ERROR_client_not_logged_in, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_could_not_validate_identity", ERROR_client_could_not_validate_identity, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_version_outdated", ERROR_client_version_outdated, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_is_flooding", ERROR_client_is_flooding, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_hacked", ERROR_client_hacked, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_cannot_verify_now", ERROR_client_cannot_verify_now, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_login_not_permitted", ERROR_client_login_not_permitted, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_client_not_subscribed", ERROR_client_not_subscribed, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_invalid_id", ERROR_channel_invalid_id, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_protocol_limit_reached", ERROR_channel_protocol_limit_reached, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_already_in", ERROR_channel_already_in, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_name_inuse", ERROR_channel_name_inuse, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_not_empty", ERROR_channel_not_empty, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_can_not_delete_default", ERROR_channel_can_not_delete_default, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_default_require_permanent", ERROR_channel_default_require_permanent, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_invalid_flags", ERROR_channel_invalid_flags, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_parent_not_permanent", ERROR_channel_parent_not_permanent, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_maxclients_reached", ERROR_channel_maxclients_reached, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_maxfamily_reached", ERROR_channel_maxfamily_reached, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_invalid_order", ERROR_channel_invalid_order, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_no_filetransfer_supported", ERROR_channel_no_filetransfer_supported, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_invalid_password", ERROR_channel_invalid_password, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_channel_invalid_security_hash", ERROR_channel_invalid_security_hash, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_server_invalid_id", ERROR_server_invalid_id, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_server_running", ERROR_server_running, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_server_is_shutting_down", ERROR_server_is_shutting_down, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_server_maxclients_reached", ERROR_server_maxclients_reached, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_server_invalid_password", ERROR_server_invalid_password, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_server_is_virtual", ERROR_server_is_virtual, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_server_is_not_running", ERROR_server_is_not_running, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_server_is_booting", ERROR_server_is_booting, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_server_status_invalid", ERROR_server_status_invalid, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_server_version_outdated", ERROR_server_version_outdated, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_server_duplicate_running", ERROR_server_duplicate_running, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_parameter_quote", ERROR_parameter_quote, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_parameter_invalid_count", ERROR_parameter_invalid_count, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_parameter_invalid", ERROR_parameter_invalid, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_parameter_not_found", ERROR_parameter_not_found, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_parameter_convert", ERROR_parameter_convert, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_parameter_invalid_size", ERROR_parameter_invalid_size, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_parameter_missing", ERROR_parameter_missing, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_parameter_checksum", ERROR_parameter_checksum, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_vs_critical", ERROR_vs_critical, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_connection_lost", ERROR_connection_lost, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_not_connected", ERROR_not_connected, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_no_cached_connection_info", ERROR_no_cached_connection_info, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_currently_not_possible", ERROR_currently_not_possible, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_failed_connection_initialisation", ERROR_failed_connection_initialisation, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_could_not_resolve_hostname", ERROR_could_not_resolve_hostname, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_invalid_server_connection_handler_id", ERROR_invalid_server_connection_handler_id, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_could_not_initialise_input_manager", ERROR_could_not_initialise_input_manager, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_clientlibrary_not_initialised", ERROR_clientlibrary_not_initialised, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_serverlibrary_not_initialised", ERROR_serverlibrary_not_initialised, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_whisper_too_many_targets", ERROR_whisper_too_many_targets, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_whisper_no_targets", ERROR_whisper_no_targets, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_connection_ip_protocol_missing", ERROR_connection_ip_protocol_missing, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_invalid_name", ERROR_file_invalid_name, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_invalid_permissions", ERROR_file_invalid_permissions, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_already_exists", ERROR_file_already_exists, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_not_found", ERROR_file_not_found, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_io_error", ERROR_file_io_error, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_invalid_transfer_id", ERROR_file_invalid_transfer_id, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_invalid_path", ERROR_file_invalid_path, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_no_files_available", ERROR_file_no_files_available, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_overwrite_excludes_resume", ERROR_file_overwrite_excludes_resume, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_invalid_size", ERROR_file_invalid_size, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_already_in_use", ERROR_file_already_in_use, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_could_not_open_connection", ERROR_file_could_not_open_connection, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_no_space_left_on_device", ERROR_file_no_space_left_on_device, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_exceeds_file_system_maximum_size", ERROR_file_exceeds_file_system_maximum_size, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_transfer_connection_timeout", ERROR_file_transfer_connection_timeout, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_connection_lost", ERROR_file_connection_lost, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_exceeds_supplied_size", ERROR_file_exceeds_supplied_size, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_transfer_complete", ERROR_file_transfer_complete, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_transfer_canceled", ERROR_file_transfer_canceled, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_transfer_interrupted", ERROR_file_transfer_interrupted, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_transfer_server_quota_exceeded", ERROR_file_transfer_server_quota_exceeded, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_transfer_client_quota_exceeded", ERROR_file_transfer_client_quota_exceeded, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_transfer_reset", ERROR_file_transfer_reset, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_file_transfer_limit_reached", ERROR_file_transfer_limit_reached, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_preprocessor_disabled", ERROR_sound_preprocessor_disabled, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_internal_preprocessor", ERROR_sound_internal_preprocessor, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_internal_encoder", ERROR_sound_internal_encoder, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_internal_playback", ERROR_sound_internal_playback, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_no_capture_device_available", ERROR_sound_no_capture_device_available, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_no_playback_device_available", ERROR_sound_no_playback_device_available, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_could_not_open_capture_device", ERROR_sound_could_not_open_capture_device, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_could_not_open_playback_device", ERROR_sound_could_not_open_playback_device, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_handler_has_device", ERROR_sound_handler_has_device, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_invalid_capture_device", ERROR_sound_invalid_capture_device, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_invalid_playback_device", ERROR_sound_invalid_playback_device, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_invalid_wave", ERROR_sound_invalid_wave, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_unsupported_wave", ERROR_sound_unsupported_wave, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_open_wave", ERROR_sound_open_wave, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_internal_capture", ERROR_sound_internal_capture, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_device_in_use", ERROR_sound_device_in_use, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_device_already_registerred", ERROR_sound_device_already_registerred, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_unknown_device", ERROR_sound_unknown_device, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_unsupported_frequency", ERROR_sound_unsupported_frequency, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_invalid_channel_count", ERROR_sound_invalid_channel_count, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_read_wave", ERROR_sound_read_wave, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_need_more_data", ERROR_sound_need_more_data, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_device_busy", ERROR_sound_device_busy, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_no_data", ERROR_sound_no_data, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_sound_channel_mask_mismatch", ERROR_sound_channel_mask_mismatch, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_permissions_client_insufficient", ERROR_permissions_client_insufficient, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_permissions", ERROR_permissions, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_virtualserver_limit_reached", ERROR_accounting_virtualserver_limit_reached, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_slot_limit_reached", ERROR_accounting_slot_limit_reached, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_license_file_not_found", ERROR_accounting_license_file_not_found, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_license_date_not_ok", ERROR_accounting_license_date_not_ok, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_unable_to_connect_to_server", ERROR_accounting_unable_to_connect_to_server, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_unknown_error", ERROR_accounting_unknown_error, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_server_error", ERROR_accounting_server_error, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_instance_limit_reached", ERROR_accounting_instance_limit_reached, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_instance_check_error", ERROR_accounting_instance_check_error, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_license_file_invalid", ERROR_accounting_license_file_invalid, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_running_elsewhere", ERROR_accounting_running_elsewhere, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_instance_duplicated", ERROR_accounting_instance_duplicated, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_already_started", ERROR_accounting_already_started, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_not_started", ERROR_accounting_not_started, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_accounting_to_many_starts", ERROR_accounting_to_many_starts, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_invalid_password", ERROR_provisioning_invalid_password, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_invalid_request", ERROR_provisioning_invalid_request, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_no_slots_available", ERROR_provisioning_no_slots_available, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_pool_missing", ERROR_provisioning_pool_missing, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_pool_unknown", ERROR_provisioning_pool_unknown, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_unknown_ip_location", ERROR_provisioning_unknown_ip_location, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_internal_tries_exceeded", ERROR_provisioning_internal_tries_exceeded, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_too_many_slots_requested", ERROR_provisioning_too_many_slots_requested, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_too_many_reserved", ERROR_provisioning_too_many_reserved, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_could_not_connect", ERROR_provisioning_could_not_connect, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_auth_server_not_connected", ERROR_provisioning_auth_server_not_connected, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_auth_data_too_large", ERROR_provisioning_auth_data_too_large, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_already_initialized", ERROR_provisioning_already_initialized, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_not_initialized", ERROR_provisioning_not_initialized, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_connecting", ERROR_provisioning_connecting, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_already_connected", ERROR_provisioning_already_connected, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_not_connected", ERROR_provisioning_not_connected, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_io_error", ERROR_provisioning_io_error, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_invalid_timeout", ERROR_provisioning_invalid_timeout, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_ts3server_not_found", ERROR_provisioning_ts3server_not_found, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("ERROR_provisioning_no_permission", ERROR_provisioning_no_permission, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("STATUS_NOT_TALKING", STATUS_NOT_TALKING, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("STATUS_TALKING", STATUS_TALKING, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("STATUS_TALKING_WHILE_DISABLED", STATUS_TALKING_WHILE_DISABLED, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CODEC_SPEEX_NARROWBAND", CODEC_SPEEX_NARROWBAND, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CODEC_SPEEX_WIDEBAND", CODEC_SPEEX_WIDEBAND, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CODEC_SPEEX_ULTRAWIDEBAND", CODEC_SPEEX_ULTRAWIDEBAND, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CODEC_CELT_MONO", CODEC_CELT_MONO, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CODEC_OPUS_VOICE", CODEC_OPUS_VOICE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CODEC_OPUS_MUSIC", CODEC_OPUS_MUSIC, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CODEC_ENCRYPTION_PER_CHANNEL", CODEC_ENCRYPTION_PER_CHANNEL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CODEC_ENCRYPTION_FORCED_OFF", CODEC_ENCRYPTION_FORCED_OFF, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CODEC_ENCRYPTION_FORCED_ON", CODEC_ENCRYPTION_FORCED_ON, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_NAME", CHANNEL_NAME, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_TOPIC", CHANNEL_TOPIC, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_DESCRIPTION", CHANNEL_DESCRIPTION, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_PASSWORD", CHANNEL_PASSWORD, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_CODEC", CHANNEL_CODEC, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_CODEC_QUALITY", CHANNEL_CODEC_QUALITY, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_MAXCLIENTS", CHANNEL_MAXCLIENTS, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_MAXFAMILYCLIENTS", CHANNEL_MAXFAMILYCLIENTS, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_ORDER", CHANNEL_ORDER, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_FLAG_PERMANENT", CHANNEL_FLAG_PERMANENT, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_FLAG_SEMI_PERMANENT", CHANNEL_FLAG_SEMI_PERMANENT, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_FLAG_DEFAULT", CHANNEL_FLAG_DEFAULT, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_FLAG_PASSWORD", CHANNEL_FLAG_PASSWORD, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_CODEC_LATENCY_FACTOR", CHANNEL_CODEC_LATENCY_FACTOR, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_CODEC_IS_UNENCRYPTED", CHANNEL_CODEC_IS_UNENCRYPTED, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_SECURITY_SALT", CHANNEL_SECURITY_SALT, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CHANNEL_DELETE_DELAY", CHANNEL_DELETE_DELAY, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_UNIQUE_IDENTIFIER", CLIENT_UNIQUE_IDENTIFIER, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_NICKNAME", CLIENT_NICKNAME, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_VERSION", CLIENT_VERSION, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_PLATFORM", CLIENT_PLATFORM, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_FLAG_TALKING", CLIENT_FLAG_TALKING, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_INPUT_MUTED", CLIENT_INPUT_MUTED, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_OUTPUT_MUTED", CLIENT_OUTPUT_MUTED, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_OUTPUTONLY_MUTED", CLIENT_OUTPUTONLY_MUTED, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_INPUT_HARDWARE", CLIENT_INPUT_HARDWARE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_OUTPUT_HARDWARE", CLIENT_OUTPUT_HARDWARE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_INPUT_DEACTIVATED", CLIENT_INPUT_DEACTIVATED, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_IDLE_TIME", CLIENT_IDLE_TIME, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_DEFAULT_CHANNEL", CLIENT_DEFAULT_CHANNEL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_DEFAULT_CHANNEL_PASSWORD", CLIENT_DEFAULT_CHANNEL_PASSWORD, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_SERVER_PASSWORD", CLIENT_SERVER_PASSWORD, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_META_DATA", CLIENT_META_DATA, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_IS_MUTED", CLIENT_IS_MUTED, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_IS_RECORDING", CLIENT_IS_RECORDING, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_VOLUME_MODIFICATOR", CLIENT_VOLUME_MODIFICATOR, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_VERSION_SIGN", CLIENT_VERSION_SIGN, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CLIENT_SECURITY_HASH", CLIENT_SECURITY_HASH, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("VIRTUALSERVER_UNIQUE_IDENTIFIER", VIRTUALSERVER_UNIQUE_IDENTIFIER, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("VIRTUALSERVER_NAME", VIRTUALSERVER_NAME, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("VIRTUALSERVER_WELCOMEMESSAGE", VIRTUALSERVER_WELCOMEMESSAGE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("VIRTUALSERVER_PLATFORM", VIRTUALSERVER_PLATFORM, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("VIRTUALSERVER_VERSION", VIRTUALSERVER_VERSION, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("VIRTUALSERVER_MAXCLIENTS", VIRTUALSERVER_MAXCLIENTS, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("VIRTUALSERVER_PASSWORD", VIRTUALSERVER_PASSWORD, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("VIRTUALSERVER_CLIENTS_ONLINE", VIRTUALSERVER_CLIENTS_ONLINE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("VIRTUALSERVER_CHANNELS_ONLINE", VIRTUALSERVER_CHANNELS_ONLINE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("VIRTUALSERVER_CREATED", VIRTUALSERVER_CREATED, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("VIRTUALSERVER_UPTIME", VIRTUALSERVER_UPTIME, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("VIRTUALSERVER_CODEC_ENCRYPTION_MODE", VIRTUALSERVER_CODEC_ENCRYPTION_MODE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("STATUS_DISCONNECTED", STATUS_DISCONNECTED, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("STATUS_CONNECTING", STATUS_CONNECTING, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("STATUS_CONNECTED", STATUS_CONNECTED, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("STATUS_CONNECTION_ESTABLISHING", STATUS_CONNECTION_ESTABLISHING, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("STATUS_CONNECTION_ESTABLISHED", STATUS_CONNECTION_ESTABLISHED, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("TEST_MODE_OFF", TEST_MODE_OFF, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("TEST_MODE_VOICE_LOCAL_ONLY", TEST_MODE_VOICE_LOCAL_ONLY, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("TEST_MODE_VOICE_LOCAL_AND_REMOTE", TEST_MODE_VOICE_LOCAL_AND_REMOTE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PING", CONNECTION_PING, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PING_DEVIATION", CONNECTION_PING_DEVIATION, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_CONNECTED_TIME", CONNECTION_CONNECTED_TIME, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_IDLE_TIME", CONNECTION_IDLE_TIME, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_CLIENT_IP", CONNECTION_CLIENT_IP, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_CLIENT_PORT", CONNECTION_CLIENT_PORT, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_SERVER_IP", CONNECTION_SERVER_IP, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_SERVER_PORT", CONNECTION_SERVER_PORT, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PACKETS_SENT_SPEECH", CONNECTION_PACKETS_SENT_SPEECH, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PACKETS_SENT_KEEPALIVE", CONNECTION_PACKETS_SENT_KEEPALIVE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PACKETS_SENT_CONTROL", CONNECTION_PACKETS_SENT_CONTROL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PACKETS_SENT_TOTAL", CONNECTION_PACKETS_SENT_TOTAL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BYTES_SENT_SPEECH", CONNECTION_BYTES_SENT_SPEECH, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BYTES_SENT_KEEPALIVE", CONNECTION_BYTES_SENT_KEEPALIVE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BYTES_SENT_CONTROL", CONNECTION_BYTES_SENT_CONTROL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BYTES_SENT_TOTAL", CONNECTION_BYTES_SENT_TOTAL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PACKETS_RECEIVED_SPEECH", CONNECTION_PACKETS_RECEIVED_SPEECH, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PACKETS_RECEIVED_KEEPALIVE", CONNECTION_PACKETS_RECEIVED_KEEPALIVE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PACKETS_RECEIVED_CONTROL", CONNECTION_PACKETS_RECEIVED_CONTROL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PACKETS_RECEIVED_TOTAL", CONNECTION_PACKETS_RECEIVED_TOTAL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BYTES_RECEIVED_SPEECH", CONNECTION_BYTES_RECEIVED_SPEECH, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BYTES_RECEIVED_KEEPALIVE", CONNECTION_BYTES_RECEIVED_KEEPALIVE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BYTES_RECEIVED_CONTROL", CONNECTION_BYTES_RECEIVED_CONTROL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BYTES_RECEIVED_TOTAL", CONNECTION_BYTES_RECEIVED_TOTAL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PACKETLOSS_SPEECH", CONNECTION_PACKETLOSS_SPEECH, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PACKETLOSS_KEEPALIVE", CONNECTION_PACKETLOSS_KEEPALIVE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PACKETLOSS_CONTROL", CONNECTION_PACKETLOSS_CONTROL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_PACKETLOSS_TOTAL", CONNECTION_PACKETLOSS_TOTAL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_SERVER2CLIENT_PACKETLOSS_SPEECH", CONNECTION_SERVER2CLIENT_PACKETLOSS_SPEECH, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_SERVER2CLIENT_PACKETLOSS_KEEPALIVE", CONNECTION_SERVER2CLIENT_PACKETLOSS_KEEPALIVE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_SERVER2CLIENT_PACKETLOSS_CONTROL", CONNECTION_SERVER2CLIENT_PACKETLOSS_CONTROL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_SERVER2CLIENT_PACKETLOSS_TOTAL", CONNECTION_SERVER2CLIENT_PACKETLOSS_TOTAL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_CLIENT2SERVER_PACKETLOSS_SPEECH", CONNECTION_CLIENT2SERVER_PACKETLOSS_SPEECH, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_CLIENT2SERVER_PACKETLOSS_KEEPALIVE", CONNECTION_CLIENT2SERVER_PACKETLOSS_KEEPALIVE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_CLIENT2SERVER_PACKETLOSS_CONTROL", CONNECTION_CLIENT2SERVER_PACKETLOSS_CONTROL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_CLIENT2SERVER_PACKETLOSS_TOTAL", CONNECTION_CLIENT2SERVER_PACKETLOSS_TOTAL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_SENT_LAST_SECOND_SPEECH", CONNECTION_BANDWIDTH_SENT_LAST_SECOND_SPEECH, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_SENT_LAST_SECOND_KEEPALIVE", CONNECTION_BANDWIDTH_SENT_LAST_SECOND_KEEPALIVE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_SENT_LAST_SECOND_CONTROL", CONNECTION_BANDWIDTH_SENT_LAST_SECOND_CONTROL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_SENT_LAST_SECOND_TOTAL", CONNECTION_BANDWIDTH_SENT_LAST_SECOND_TOTAL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_SPEECH", CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_SPEECH, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_KEEPALIVE", CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_KEEPALIVE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_CONTROL", CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_CONTROL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_TOTAL", CONNECTION_BANDWIDTH_SENT_LAST_MINUTE_TOTAL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_SPEECH", CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_SPEECH, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_KEEPALIVE", CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_KEEPALIVE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_CONTROL", CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_CONTROL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_TOTAL", CONNECTION_BANDWIDTH_RECEIVED_LAST_SECOND_TOTAL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_SPEECH", CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_SPEECH, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_KEEPALIVE", CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_KEEPALIVE, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_CONTROL", CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_CONTROL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);
	REGISTER_LONG_CONSTANT("CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_TOTAL", CONNECTION_BANDWIDTH_RECEIVED_LAST_MINUTE_TOTAL, CONST_CS|CONST_PERSISTENT|CONST_CT_SUBST);

	return SUCCESS;
}

void shutdown_return_codes(struct WaitItem* item)
{
	if (item != NULL)
	{
		shutdown_return_codes(item->next);
		free_return_code_item(item);
	}
}

void shutdown_connection_items(struct ConnectionItem* item)
{
	if (item != NULL)
	{
		shutdown_connection_items(item->next);
		free_connection_item(item);
	}
}

PHP_MSHUTDOWN_FUNCTION(ts3client)
{
	if (--instance_count == 0)
	{
		pthread_mutex_lock(&mutex);
		if (instance_count  == 0)
		{
			ts3client_destroyClientLib();
			shutdown_return_codes(wait_items);
			wait_items = NULL;
			shutdown_connection_items(connection_items);
			connection_items = NULL;
		}
		pthread_mutex_unlock(&mutex);
	}
	return SUCCESS;
}

PHP_MINFO_FUNCTION(ts3client)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Teamspeak Client SDK support", "enabled");
	php_info_print_table_row(2, "Version", PHP_TS3CLIENT_VERSION);
	php_info_print_table_end();
}

zend_module_entry ts3client_module_entry = {
	STANDARD_MODULE_HEADER,
	"ts3client",
	ts3client_functions,
	PHP_MINIT(ts3client),
	PHP_MSHUTDOWN(ts3client),
	NULL,
	NULL,
	PHP_MINFO(ts3client),
	PHP_TS3CLIENT_VERSION,
	NO_MODULE_GLOBALS,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_TS3CLIENT
ZEND_GET_MODULE(ts3client)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
