/* Copyright (c) 2014 UCR-UAS
 * You should have recieved a copy of the MIT license with this file.
 * Engineer: Brandon Lu
 * Description: This is the BISON-Transfer server version.
 * What dose it do?  Tarring and insanity.
 * This is indeed a child-processed server.
 */

// ================= TODO =================
// Read the Trello cards.  (You can be added if you ask!)

// =============== Includes ===============
#include <arpa/inet.h>
#include "BISON-Defaults.h"
#include <boost/circular_buffer.hpp>
#include <boost/filesystem.hpp>
#include "config-check.h"
#include <dirent.h>
#include <errno.h>
#include <exception>
#include <fstream>
#include <list>
#include <map>
#include <netinet/in.h>
#include <openssl/md5.h>
#include "parse-command.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "filetable.h"
#include <vector>
#include <yaml-cpp/yaml.h>

// ===== Terrible global variables  ========
int sfd;
int MAX_BACKLOG;
bool BISON_TRANSMIT_REALTIME;
std::string BISON_TRANSFER_ADDRESS;
int BISON_TRANSFER_PORT;
std::string BISON_TRANSFER_DIR;
int ERROR_WAIT;
int argC;
char **argV;
std::map<std::string, std::vector<unsigned char>> filetable;
YAML::Node config;

// ============ Configuration ==============
void configure_server(YAML::Node &config)
{
	try {
		config = YAML::LoadFile(__DEF_SERVER_CONFIG_PATH__
				+ std::string(__DEF_SERVER_CONFIG_FILE__));
#if DEBUG
		std::cout << "Successfully opened configuration file." << std::endl;
#endif // DEBUG
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "Attempting to configure automatically." << std::endl;
	}

	// configure with the default settings if the values do not exist
	YAML::Node xferConfig = config["BISON-Transfer"]["Server"];
	if (!xferConfig["PID-File"])
		xferConfig["PID-File"] = DEF_TRANSMIT_PID_FILE;
	if (!xferConfig["Transmit-Mode"])
		xferConfig["Transmit-Mode"] = DEF_BISON_TRANSMIT_MODE;
	if (!xferConfig["Bind-Address"])
		xferConfig["Bind-Address"] = DEF_BISON_TRANSFER_BIND;
	if (!xferConfig["Port"])
		xferConfig["Port"] = DEF_BISON_TRANSFER_PORT;
	if (!xferConfig["Max-Backlog"])
		xferConfig["Max-Backlog"] = DEF_MAX_BACKLOG;
	if (!xferConfig["Transmit-Dir"])
		xferConfig["Transmit-Dir"] = DEF_TRANSMIT_DIR;
	if (!xferConfig["Error-Wait"])
		xferConfig["Error-Wait"] = DEF_ERROR_WAIT;

	// set file variables to the yaml configuration variables
	std::string BISON_TRANSMIT_MODE
		= xferConfig["Transmit-Mode"].as<std::string>();
	BISON_TRANSFER_ADDRESS
		= xferConfig["Bind-Address"].as<std::string>();
	BISON_TRANSFER_PORT = xferConfig["Port"].as<int>();
	MAX_BACKLOG = xferConfig["Max-Backlog"].as<int>();
	BISON_TRANSFER_DIR 
		= xferConfig["Transmit-Dir"].as<std::string>();
	ERROR_WAIT = xferConfig["Error-Wait"].as<int>();

	// if we are debugging, output the debug information
	if (DEBUG) {
		std::cout << "Transmit mode:" << BISON_TRANSMIT_MODE << std::endl;
		std::cout << "Transfer Address (for binding): "
			<< BISON_TRANSFER_ADDRESS << std::endl;
		std::cout << "Transfer port: " << BISON_TRANSFER_PORT << std::endl;
		std::cout << "Max Backlog: " << MAX_BACKLOG << std::endl;
		std::cout << "Transmitting: " << BISON_TRANSFER_DIR << std::endl;
	}

	if (strcmp(BISON_TRANSMIT_MODE.c_str(), "REALTIME") == 0) {
		BISON_TRANSMIT_REALTIME = true;
		std::cout << "Transmitting realtime." << std::endl;
	} else if (strcmp(BISON_TRANSMIT_MODE.c_str(), "QUEUE") == 0) {
		
	}

#if DEBUG
	printf("Writing configuration file...\n");
#endif // if DEBUG

	dirChkCreate(__DEF_SERVER_CONFIG_PATH__, "configuration");

	writeOutYaml(config, __DEF_SERVER_CONFIG_PATH__
			+ std::string(__DEF_SERVER_CONFIG_FILE__));
}

// ========= Connection Management =========
int prepare_connection()
{
	struct sockaddr_in my_addr;

	// socket it
#if DEBUG
	printf("Socketing...\n");
#endif // if DEBUG
	sfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sfd == -1) {
		std::cerr << "Could not create socket." << std::endl;
		errno = 0;
		return 1;
	}

	// set things for bind
	memset (&my_addr, 0, sizeof(struct sockaddr));
	my_addr.sin_family = AF_INET;
	if (inet_aton(BISON_TRANSFER_ADDRESS.c_str(), &my_addr.sin_addr) == 0) {
		std::cerr << "Could not parse bind address." << std::endl;
		exit(1);
	}
	my_addr.sin_port = htons(BISON_TRANSFER_PORT);
	if (my_addr.sin_port == -1) {
		std::cerr << "Could not convert input port." << std::endl;
		exit(1);
	}

	// bind
#if DEBUG
	printf("Binding...\n");
#endif // if DEBUG
	if (bind(sfd, (struct sockaddr*) &my_addr, sizeof(struct sockaddr_in))
			== -1) {
		std::cerr << "Could not bind to socket." << std::endl;
		errno = 0;
		return 2;
	}

	// listen for connections
	if (listen(sfd, MAX_BACKLOG) == -1) {
		std::cerr << "Could not listen to socket." << std::endl;
		errno = 0;
		return 3;
	}

	return 0;
}

// ============ Send Filetable =============
void send_filetable(int cfd)
{
	for (std::map<std::string, std::vector<unsigned char>>::iterator 
			it=filetable.begin(); it != filetable.end(); it++) {
		for (std::vector<unsigned char>::iterator iter
				= it->second.begin(); iter != it->second.end(); iter++) {
			dprintf(cfd, "%02x", *iter);	// print MD5 sum
		}
		dprintf(cfd, "  %s\n", it->first.c_str());
		// print filename
	}
	// newline termination
	dprintf(cfd, "\n");
}

// ============ Child Function =============
void child_function(int cfd)
{
	action_t action;
	std::string filename;
	parse_command(cfd, action, filename);
	switch (action) {
		case NONE:
			std::cerr << "Defunct client found." << std::endl;
			break;
		case SEND:
		{
		/* Consider this: what if the server does not have the file? TODO
*/
			FILE *send_me = fopen((BISON_TRANSFER_DIR + filename).c_str(), "r");
			if (!send_me)
			   error("Could not open file for reading.");
			int len;
			char buf[MAXBUFLEN + 1];
			while ((len = fread(buf, sizeof(char), MAXBUFLEN, send_me)) > 0)
			   write(cfd, buf, len);
		}	break;
		case FILETABLE_REQ:
			send_filetable(cfd);
			break;
		case RECALCULATE_MD5:
			recalculate_MD5(BISON_TRANSFER_DIR, filename, filetable);
			break;
	}
}

// ========== Connection Handling ==========
void handle_connection()
{
	pid_t pid;
	int cfd;

	// Wait to accept connection
	cfd = accept(sfd, NULL, NULL);
	if (cfd == -1) {
		crit_error("Could not accept connections");
	}

	pid = fork();
	if (pid == -1)
		error("Fork");
	if (pid == 0) {							// we are the child.
		child_function(cfd);
		printf("Spawned child exiting.\n");
		shutdown(cfd, SHUT_RDWR);
		close(cfd);
		exit(0);
	} else {								// we are the parent
		close(cfd);							// the child handles this
	}
}

// =========== Child Handling =============
void handle_children()
{
	// I accidentally took all the pipes out, so I'll have to reimplement -
	// them for logging... or better yet, I can just have the server listen -
	// on a socket!
}

// ======= Nice Termination handler =======
// terminate nicely!
void terminate_nicely(int sig)
{
	printf("Caught signal %i, terminating nicely.\n", sig);
	close(sfd);
	exit(0);
}

// ===== Not Nice Termination handler =====
void error_terminate(const int status)
{
	close(sfd);
	exit(status);
}

// ======= Configuration Reloading ========
void configuration_reload(int sig)
{
	configure_server(config);
}

// =========== Signal Handling ============
void signalHandling()
{
	signal(SIGHUP, configuration_reload);
	signal(SIGTERM, terminate_nicely);
	signal(SIGINT, terminate_nicely);
	signal(SIGCHLD, SIG_IGN);				// IGNORE YOUR CHILDREN.
}

// ========== The Main Function ============
int main (int argc, char *argv[])
{
	configure_server(config);

	signalHandling();

#if DEBUG
	printf("Checking if transmission directory exists...\n");
#endif // if DEBUG
	dirChkCreate(BISON_TRANSFER_DIR.c_str(), "transfer");

	printf("Server Starting Up...\n");

	// Insert PID file management stuff here

	// keep on preparing the connection until it is prepared.
	while (1) {
		if (!prepare_connection())
			break;
		for (int i = 0; i > ERROR_WAIT; i++)
			usleep(1000);
	}

#if DEBUG
	printf("Server ready, waiting for connection.\n");
#endif // if DEBUG

	// Loop to keep accepting connections
	while (1) {
		const char* ret = update_filetable(BISON_TRANSFER_DIR, filetable);
		if (ret) {
			std::cerr << "Error on filetable update: " << ret << std::endl;
		}
		handle_connection();
		usleep(5000);					// wait a bit so that we won't -
		// overload the system
	}

	return 0;
}
