/* Engineer: Brandon Lu
 * Description: This is the BISON-Transfer server version 1.
 * What dose it do?  Tarring and insanity.
 * This is indeed a multi-threaded server.
 */

/* Includes */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

/* Defines */
const char __DEFAULT_SYSTEM_PATH__[] = "/var/run/UCR-UAS/BISON-Transfer/";

/* System critical errors */
void crit_error(const char *message)
{
	perror(message);
	exit(2);
}

void error(const char *message)
{
	perror(message);
	exit(1);
}

/* a main function */
int main (int argc, char *argv[])
{
	printf("Server Starting Up...\n");		/* TODO: Color support ;) */
	char* args[] = {"tar", "-cz", "./test/nyan_cat", 0};
	execvp("tar", args);
	return 0;
}
