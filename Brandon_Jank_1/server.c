/* Engineer: Brandon Lu
 * Description: This is the BISON-Transfer server version 1.
 * What dose it do?  Tarring and insanity.
 * This is indeed a multi-threaded server.
 */

/* Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

/* Defines */
const char __DEFAULT_SYSTEM_PATH__[] = "/var/run/UCR-UAS/BISON-Transfer/";
#define BRANDON_DEBUG (1)
#define MAX_BACKLOG (5)

/* Terrible global variables */
int sfd;

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

/* Connection management */
void prepare_connection()
{
    struct sockaddr_in my_addr;

    // socket it
#if BRANDON_DEBUG
    printf("Socketing...\n");
#endif // if BRANDON_DEBUG
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        crit_error("socket");

    // set things for bind
    memset (&my_addr, 0, sizeof(struct sockaddr));
                                            // clear structure
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(6673);			// ASCII to decimal BI

    // bind
#if BRANDON_DEBUG
    printf("Binding...\n");
#endif // if BRANDON_DEBUG
    if (bind(sfd, (struct sockaddr*) &my_addr, sizeof(struct sockaddr_in))
			== -1)
        crit_error("bind");

	// listen for connections
	if (listen(sfd, MAX_BACKLOG) == -1)
		crit_error("listen");
}

/* Listen for and Handle Connections */
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

		char* args[] = {"tar", "-cz", "./test/nyan_cat", 0};
		execvp("tar", args);
		close(cfd);
		exit(0);
	}
}

/* Signal Handler for SIGINT or SIGTERM */
// Die nicely!
void die_nicely(int sig)
{
	close(sfd);
	exit(0);
}

/* A Main Function */
int main (int argc, char *argv[])
{
	printf("Server Starting Up...\n");		/* TODO: Color support ;) */

	signal(SIGCHLD, SIG_IGN);				// IGNORE YOUR CHILDREN.
											// TODO: pay attention to children
	// Different, pleasant ways to terminate the program
	signal(SIGTERM, die_nicely);
	signal(SIGINT, die_nicely);

	prepare_connection();

	// Loop to keep accepting connections
	while (1) {
		handle_connection();
	}
	return 0;
}
