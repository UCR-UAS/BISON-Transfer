#ifndef __BISON_STUFF__
#define __BISON_STUFF__

// ================ Defines ================
const char __CONFIG_PATH__[] = "~/.BISON/config.yaml";
#define BRANDON_DEBUG (1)
#define MAX_BACKLOG (30)
#define BISON_TRANSFER_PORT (6673)			// ascii "BI"

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
