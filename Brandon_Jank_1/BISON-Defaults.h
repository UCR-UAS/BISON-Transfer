#ifndef __BISON_STUFF__
#define __BISON_STUFF__
#include <string>

/*
 * This file definitely needs to be changed once the distributed version is
 * ready.
 */

// ================ Defines ================
const char __DEF_SERVER_CONFIG_PATH__[] = "/home/brandon/.BISON/";
const char __DEF_SERVER_CONFIG_FILE__[] = "transfer_server_config.yaml";
const char __DEF_CLIENT_CONFIG_PATH__[] = "/home/brandon/.BISON/";
const char __DEF_CLIENT_CONFIG_FILE__[] = "transfer_client_config.yaml";
#define DEBUG (1)
#define DEF_MAX_BACKLOG (30)
#define DEF_BISON_TRANSFER_PORT (6673)		// ascii "BI"
#define DEF_BISON_TRANSFER_SERVER ("127.0.0.1")
#define DEF_BISON_TRANSFER_BIND ("0.0.0.0")

// ============ Error Messages ============
extern void error_terminate(const int);
void crit_error(const char *message)
{
	perror(message);
	error_terminate(2);
}

void error(const char *message)
{
	perror(message);
	error_terminate(1);
}

#endif // ifndef __BISON_STUFF__
