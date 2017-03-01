/* $Id$ */
#pragma once
#include <php.h>

#define PHP_TS3CLIENT_VERSION "0.0.0"

#ifdef PHP_WIN32
#define TS3CLIENT_API __declspec(dllexport)
#else
#define TS3CLIENT_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

extern zend_module_entry ts3client_module_entry;
#define phpext_ts3client_ptr &ts3client_module_entry

PHP_MINFO_FUNCTION(ts3client);
PHP_MINIT_FUNCTION(ts3client);
PHP_MSHUTDOWN_FUNCTION(ts3client);

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
