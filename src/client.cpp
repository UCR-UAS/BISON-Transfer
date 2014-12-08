/* Copyright (c) 2014 UCR-UAS
 * You should have recieved a copy of the MIT licence with this file.
 * Engineer: Brandon Lu
 * Description: This is the BISON-Transfer client version 1.
 * What dose it do? Un-tarring and insanity.
 * This is not a multi-threaded client.
 */

/* ================ Notices ===============
 * You should probably read the server code comments for the most updated
 * information.  The server kind of came first.
 */

// =============== Includes ===============
#include <arpa/inet.h>
#include "BISON-Defaults.h"
#include <boost/filesystem.hpp>
#include "directory-check.h"
#include <dirent.h>
#include <errno.h>
#include <exception>
#include <fstream>
#include <map>
#include <netinet/in.h>
#include <openssl/md5.h>
#include "parse-filetable.h"
#include <queue>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "update-filetable.h"
#include <vector>
#include <yaml-cpp/yaml.h>

typedef enum {FILETABLE, TRANSFER, REALTIME} action_t;

// =========== Global Variables ===========
int sfd;
std::string BISON_TRANSFER_SERVER;
int BISON_TRANSFER_PORT;
std::string BISON_RECIEVE_DIR;
std::queue<std::string> filequeue;
std::map<std::string, std::vector<unsigned char>> filetable;

// ============ Configuration ==============
/*
 * The configuration functions for the server and client get very repetetive.
 */
void configure_client(YAML::Node &config)
{
	try {
		config = YAML::LoadFile(__DEF_CLIENT_CONFIG_PATH__ 
			+ std::string(__DEF_CLIENT_CONFIG_FILE__));
#if DEBUG
		std::cout << "Successfully opened configuration file." << std::endl;
#endif // DEBUG
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "Attempting to configure automatically." << std::endl;
	}

	if (!config["BISON-Transfer"]["Client"]["Server-address"])
		config["BISON-Transfer"]["Client"]["Server-address"] 
			= DEF_BISON_TRANSFER_SERVER;
	if (!config["BISON-Transfer"]["Client"]["Port"])
		config["BISON-Transfer"]["Client"]["Port"] = DEF_BISON_TRANSFER_PORT;
	if (!config["BISON-Transfer"]["Client"]["Recieve-Dir"])
		config["BISON-Transfer"]["Client"]["Recieve-Dir"] = DEF_RECIEVE_DIR;

	BISON_TRANSFER_SERVER
		= config["BISON-Transfer"]["Client"]["Server-address"]
		.as<std::string>();
	BISON_TRANSFER_PORT = config["BISON-Transfer"]["Client"]["Port"].as<int>();
	BISON_RECIEVE_DIR
		= config["BISON-Transfer"]["Client"]["Recieve-Dir"].as<std::string>();

	if (DEBUG) {
		std::cout << "Transfer Address (for binding): "
			<< BISON_TRANSFER_SERVER << std::endl;
		std::cout << "Transfer port: " << BISON_TRANSFER_PORT << std::endl;
	}

#if DEBUG
	std::cout << "Writing config file" << std::endl;
#endif // if DEBUG

	// Write out the configuration before starting 
	dirChkCreate(__DEF_CLIENT_CONFIG_PATH__, "configuration");

	writeOutYaml(config, __DEF_CLIENT_CONFIG_PATH__ 
			+ std::string(__DEF_CLIENT_CONFIG_FILE__));
}

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

// =========== Handle Connection ===========
void handle_connection(action_t &action)
{
#if DEBUG
	printf("Socketing... \n");
#endif // if DEBUG
	sfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sfd == -1)
		crit_error("Socket");

	// set things for sockaddr_in before connection
	struct sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(struct sockaddr_in));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(BISON_TRANSFER_PORT);
	if (!inet_aton(BISON_TRANSFER_SERVER.c_str(),
		(struct in_addr*) &srv_addr.sin_addr.s_addr))
		error("Server Address Invalid");

#if DEBUG
	printf("Connecting... \n");
#endif // if DEBUG
	while (connect(sfd, (struct sockaddr*) &srv_addr,
		sizeof(struct sockaddr_in)) == -1) {
		// brute force and try to connect anyway!
		std:: cout << '.' << std::flush;
		for (int i = 0; i < 1000; i++)
			usleep(1000);
		errno = 0;
	}

	if (filequeue.empty()) {
		std::cout << "Filequeue is empty.  Checking for new files."
			<< std::endl;
		action = FILETABLE;
	}

	switch (action) {
		case TRANSFER:
		{
			std::cout << "Front is: " << filequeue.front() << std::endl;
			std::string filename(filequeue.front());
			filequeue.pop();				// remove front member
			dprintf(sfd, "REQ: %s\n\n", filename.c_str());
			FILE *output_file = fopen((BISON_RECIEVE_DIR + filename).c_str(),
				"w");
			if (output_file == 0)
				error("Could not open new file for writing.");

			char buf[MAXBUFLEN + 1];
			int len = 0;
			while((len = read(sfd, buf, MAXBUFLEN)) > 0) {
											// Now works with binary!
				fwrite(buf, sizeof(char), len, output_file);
			}

			fclose(output_file);
		}	break;
		case FILETABLE:
		{
            const char *ret;
			ret = update_filetable(BISON_RECIEVE_DIR, filetable);
			if (ret) {
				std::cerr << "Filetable did not update:" << ret << std::endl;
				exit(1);
			}
			dprintf(sfd, "FTREQ\n\n");
			std::map<std::string, std::vector<unsigned char>> tmp_filetable;
			md5_parse(sfd, tmp_filetable);

			if (!filequeue.empty())
				std::cerr << "Filetable is not empty. Emptying." << std::endl;
			while(!filequeue.empty())
				filequeue.pop();

			// check for files here that do not exist on our end
			for (std::map<std::string, std::vector<unsigned char>>::iterator
				it = tmp_filetable.begin(); it != tmp_filetable.end(); it++) {
				std::map<std::string, std::vector<unsigned char>>::iterator
					it2 = filetable.find(it->first);
				// nonexistent file
				if (it2 == filetable.end()) {
					std::cout << "Missing: " << it->first << std::endl;
					filequeue.push(it->first);
					continue;
				}
				// handle incorrect / incomplete files
				if (it2->second != it->second) {
					std::cout << "Corrupt file: " << it->first << std::endl;
					filequeue.push(it->first);
					continue;
				}
			}
			action = TRANSFER;
		}	break;
		case REALTIME:
			break;
	}
	close(sfd);
}

// =========== Signal Handling =============
void signalHandling()
{
	signal(SIGCHLD, SIG_IGN);				// ignore children for now
	// Handle different ways of program termination
	signal(SIGTERM, terminate_nicely);
	signal(SIGINT, terminate_nicely);

}

// ========== The Main Function ============
int main (int argc, char *argv[])
{
	YAML::Node config;
	configure_client(config);

	// Insert PID file management stuff here

	signalHandling();

	dirChkCreate(BISON_RECIEVE_DIR.c_str(), "recieve");

	printf("Client Starting Up...\n");
	action_t action = FILETABLE;

	while(1) {
		handle_connection(action);
	}
	return 0;
}
