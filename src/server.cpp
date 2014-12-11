/* Copyright (c) 2014 UCR-UAS
 * You should have recieved a copy of the MIT license with this file.
 * Engineer: Brandon Lu
 * Description: This is the BISON-Transfer server version 1.
 * What dose it do?  Tarring and insanity.
 * This is indeed a child-processed server.
 */

// ================= TODO =================
// Read the Trello cards.  (You can be added if you ask!)

// =============== Includes ===============
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
std::string BISON_TRANSFER_ADDRESS;
int BISON_TRANSFER_PORT;
std::string BISON_TRANSFER_DIR;
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
	if (!config["BISON-Transfer"]["Server"]["Bind-address"])
		config["BISON-Transfer"]["Server"]["Bind-address"] 
			= DEF_BISON_TRANSFER_BIND;
	if (!config["BISON-Transfer"]["Server"]["Port"])
		config["BISON-Transfer"]["Server"]["Port"] = DEF_BISON_TRANSFER_PORT;
	if (!config["BISON-Transfer"]["Server"]["Max-Backlog"])
		config["BISON-Transfer"]["Server"]["Max-Backlog"] = DEF_MAX_BACKLOG;
	if (!config["BISON-Transfer"]["Server"]["Transmit-Dir"])
		config["BISON-Transfer"]["Server"]["Transmit-Dir"] = DEF_TRANSMIT_DIR;

	// set file variables to the yaml configuration variables
	BISON_TRANSFER_ADDRESS
		= config["BISON-Transfer"]["Server"]["Bind-address"].as<std::string>();
	BISON_TRANSFER_PORT = config["BISON-Transfer"]["Server"]["Port"].as<int>();
	MAX_BACKLOG = config["BISON-Transfer"]["Server"]["Max-Backlog"].as<int>();
	BISON_TRANSFER_DIR 
		= config["BISON-Transfer"]["Server"]["Transmit-Dir"].as<std::string>();

	// if we are debugging, output the debug information
	if (DEBUG) {
		std::cout << "Transfer Address (for binding): "
			<< BISON_TRANSFER_ADDRESS << std::endl;
		std::cout << "Transfer port: " << BISON_TRANSFER_PORT << std::endl;
		std::cout << "Max Backlog: " << MAX_BACKLOG << std::endl;
		std::cout << "Transmitting: " << BISON_TRANSFER_DIR << std::endl;
	}

#if DEBUG
	printf("Writing configuration file...\n");
#endif // if DEBUG

	dirChkCreate(__DEF_SERVER_CONFIG_PATH__, "configuration");

	writeOutYaml(config, __DEF_SERVER_CONFIG_PATH__
			+ std::string(__DEF_SERVER_CONFIG_FILE__));
}

// ========= Connection Management =========
void prepare_connection()
{
	struct sockaddr_in my_addr;

	// socket it
#if DEBUG
	printf("Socketing...\n");
#endif // if DEBUG
	sfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sfd == -1)
		crit_error("Socket");

	// set things for bind
	memset (&my_addr, 0, sizeof(struct sockaddr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(BISON_TRANSFER_PORT);

	// bind
#if DEBUG
	printf("Binding...\n");
#endif // if DEBUG
	if (bind(sfd, (struct sockaddr*) &my_addr, sizeof(struct sockaddr_in))
			== -1)
		crit_error("Bind Failed");

	// listen for connections
	if (listen(sfd, MAX_BACKLOG) == -1)
		crit_error("listen");
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
		// this is a nonblocking function, so it will give us an error when it-
		// executes (and we simply catch the error and ignore it)
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			errno = 0;						// There is no error...
			usleep(5000);					// wait a bit so that we won't -
			// overload the system
			return;
		}
		else
			crit_error("Could not accept connections");
	}

	pid = fork();
	if (pid == -1)
		error("Fork");
	if (pid == 0) {							// we are the child.
		child_function(cfd);
		printf("Spawned child exiting.\n");
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

	prepare_connection();

#if DEBUG
	printf("Server ready, waiting for connection.\n");
#endif // if DEBUG

	// Loop to keep accepting connections
	while (1) {
		const char* ret = update_filetable(BISON_TRANSFER_DIR, filetable);
		if (ret) {
			std::cerr << "Error on filetable update: " << ret << std::endl;
			exit(1);
		}
		handle_connection();
	}

	return 0;
}
