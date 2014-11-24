/* Engineer: Brandon Lu
 * Description: This is the BISON-Transfer server version 1.
 * What dose it do?  Tarring and insanity.
 * This is indeed a multi-threaded server.
 */

// ================= TODO =================
// File tracking
// Actually keep track of clients
// Actually read from a directory
// Actually read environmental variables
// Actually keep logs
// Logrotate support?

// =============== Includes ===============
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include "BISON-Defaults.h"

/* Terrible global variables */
int sfd;

// ========= Connection Management =========
void prepare_connection()
{
    struct sockaddr_in my_addr;

    // socket it
#if BRANDON_DEBUG
    printf("Socketing...\n");
#endif // if BRANDON_DEBUG
    sfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        crit_error("Socket");

    // set things for bind
    memset (&my_addr, 0, sizeof(struct sockaddr));
                                            // clear structure
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(BISON_TRANSFER_PORT);

    // bind
#if BRANDON_DEBUG
    printf("Binding...\n");
#endif // if BRANDON_DEBUG
    if (bind(sfd, (struct sockaddr*) &my_addr, sizeof(struct sockaddr_in))
			== -1)
        crit_error("Bind Failed");

	// listen for connections
	if (listen(sfd, MAX_BACKLOG) == -1)
		crit_error("listen");
}

// ========== Connection Handling ==========
void handle_connection()
{
	int pid;
    int cfd;

	// Wait to accept connection
#if BRANDON_DEBUG
	printf("Server waiting for connection.\n");
#endif // if BRANDON_DEBUG
	cfd = accept(sfd, NULL, NULL);
	if (cfd == -1)
		crit_error("Could not accept connections");

	pid = fork();
	if (pid == -1)
		error("Fork");
	if (pid == 0) {
		// Dup back to the client
		dup2(cfd, STDOUT_FILENO);

		char* args[] = {"tar", "-hcz", "./test_link/", 0};
		execvp("tar", args);
		// included for posterity, not functionality.  exec() will not bring -
		// us back
		close(cfd);
		exit(0);
	} else {
		close(cfd);
	}
}

// ======= Nice Termination handler =======
// terminate nicely!
void terminate_nicely(int sig)
{
	printf("Caught signal %i, terminating nicely.", sig);
	close(sfd);
	exit(0);
}

// ===== Not Nice Termination handler =====
void error_terminate(const int status)
{
	close(sfd);
	exit(status);
}

int main (int argc, char *argv[])
{
	printf("Server Starting Up...\n");		/* TODO: Color support ;) */

	// Insert PID file management stuff here

	signal(SIGCHLD, SIG_IGN);				// IGNORE YOUR CHILDREN.
											// TODO: pay attention to children
	// Different, pleasant ways to terminate the program
	signal(SIGTERM, terminate_nicely);
	signal(SIGINT, terminate_nicely);

	prepare_connection();

	// Loop to keep accepting connections
	while (1) {
		handle_connection();
	}
	return 0;
}
