/* Engineer: Brandon lu
 * Description: This is the BISON-Transfer client version 1.
 * What dose it do? Un-tarring and insanity.
 * This is probably not a multi-threaded client.
 */

// ================ Notices ===============
/* You should probably read the server code comments for the most updated
 * information.  The server kind of came first.
 */

// =============== Includes ===============
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "BISON-Defaults.h"

// =========== Global Variables ===========
int sfd;

// ======= Nice Termination handler =======
// terminate nicely!
void terminate_nicely(int sig)
{
	close(sfd);
	exit(0);
}

// ===== Not Nice Termination handler =====
void error_terminate(const int status) {
	close(sfd);
	exit(status);
}

int main (int argc, char *argv[])
{
	printf("Client Starting Up...\n");

	// Insert PID file management stuff here

	signal(SIGCHLD, SIG_IGN);				// ignore children for now
	// Handle different ways of program termination
	signal(SIGTERM, terminate_nicely);
	signal(SIGINT, terminate_nicely);

	struct sockaddr_in srv_addr;

#if BRANDON_DEBUG
	printf("Socketing... \n");
#endif // if BRANDON_DEBUG
	sfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sfd == -1)
		crit_error("Socket");

	// set things for sockaddr_in before binding
	memset(&srv_addr, 0, sizeof(struct sockaddr_in));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(BISON_TRANSFER_PORT);
	// TODO: Preliminary testing removal, insertion of actual stuff
	if (!inet_aton("192.168.1.40", (struct in_addr*) &srv_addr.sin_addr.s_addr))
		error("Server Address Invalid");

	// bind
#if BRANDON_DEBUG
	printf("Binding... \n");
#endif // if BRANDON_DEBUG
	if (connect(sfd, (struct sockaddr*) &srv_addr, sizeof(struct sockaddr_in))
			== -1)
		crit_error("Connect Failed");

	dup2(sfd, STDIN_FILENO);				// prepare input for tar

	char* args[] = {"tar", "-x", 0};
	execvp(args[0], args);
	// included for posterity, not functionality.  exec() will not bring us -
	// back
	close(sfd);
	exit(0);
	return 0;
}
