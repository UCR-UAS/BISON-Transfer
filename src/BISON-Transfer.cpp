/* Copyright (c) 2014 UCR-UAS
 * You should have recieved a copy of the MIT licence with this file.
 * Engineer: Brandon Lu
 * Description: This is the BISON-Transfer client version 1.
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
#include "config-check.h"
#include <dirent.h>
#include <errno.h>
#include <exception>
#include <fstream>
#include <map>
#include <netinet/in.h>
#include <openssl/md5.h>
#include "filetable.h"
#include <queue>
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

typedef enum {FILETABLE, TRANSFER, REALTIME, RECALCULATE_MD5} action_t;

// =========== Global Variables ===========
int sfd;
std::string BISON_TRANSFER_SERVER;
int BISON_TRANSFER_PORT;
std::string BISON_RECIEVE_DIR;
int ERROR_WAIT;
std::queue<std::string> filequeue;
std::queue<std::string> recalc_queue;
std::map<std::string, std::vector<unsigned char>> filetable;
YAML::Node config;

// ============ Configuration ==============
/*
 * The configuration functions for the server and client get very repetetive.
 * But they're darn easy to program in!!! -- Brandon
 *
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
	if (!config["BISON-Transfer"]["Client"]["Error-Wait"])
		config["BISON-Transfer"]["Client"]["Error-Wait"] = DEF_ERROR_WAIT;

	BISON_TRANSFER_SERVER
		= config["BISON-Transfer"]["Client"]["Server-address"]
		.as<std::string>();
	BISON_TRANSFER_PORT = config["BISON-Transfer"]["Client"]["Port"].as<int>();
	BISON_RECIEVE_DIR = config["BISON-Transfer"]["Client"]["Recieve-Dir"]
		.as<std::string>();
	ERROR_WAIT = config["BISON-Transfer"]["Client"]["Error-Wait"].as<int>();

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
	// printf("Socketing... \n");
#endif // if DEBUG
	sfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sfd == -1) {
		close(sfd);
	}

	// set things for sockaddr_in before connection
	struct sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(struct sockaddr_in));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(BISON_TRANSFER_PORT);
	if (!inet_aton(BISON_TRANSFER_SERVER.c_str(),
		(struct in_addr*) &srv_addr.sin_addr.s_addr)) {
		std::cerr << "Server Address Invalid" << std::endl;
		close(sfd);
		return;
	}

#if DEBUG
	// printf("Connecting... \n");
#endif // if DEBUG
	if (connect(sfd, (struct sockaddr*) &srv_addr,
		sizeof(struct sockaddr_in)) == -1) {
		errno = 0;
		close(sfd);
		return;
	}

	if (filequeue.empty()) {
		std::cout << "Filequeue is empty.  Checking for new files."
			<< std::endl;
		action = FILETABLE;
	}

	if (!recalc_queue.empty()) {
		std::cout << "Recalculation queue is not empty. Relaying filenames."
			<< std::endl;
		action = RECALCULATE_MD5;
	}

	switch (action) {
		case TRANSFER:
		{
			// In this case, we transfer over the file that is on top of the--
			// file queue.

			// Man, I really need to implement loglevels -- Brandon
			std::cout << "Front is: " << filequeue.front() << std::endl;

			// Get the filename
			std::string filename(filequeue.front());
			// Remove the very top file
			filequeue.pop();				// remove front member
			// Tell the server exactly what we want.
			dprintf(sfd, "REQ: %s\n\n", filename.c_str());
			// Open the file that we want to put it in.
			FILE *output_file = fopen((BISON_RECIEVE_DIR + filename).c_str(),
				"w");
			// If we couldn't open it for some reason, let the server continue
			if (output_file == 0) {
				std::cerr << "Could not open new file for writing."
					<< std::endl;
				close(sfd);
				return;
			}

			// Write the data into the output file which was opened earlier.
			bool delete_flag = false;
			char buf[MAXBUFLEN + 1];
			int len = 0;
            // file descriptor set for writing to file
            fd_set set;
            FD_SET(sfd, &set);
            struct timeval timeout;
            timeout.tv_sec = 30;
            timeout.tv_usec = 0;
			while(select(sfd+1, &set, NULL, NULL, &timeout) != 0) {
                len = read(sfd, buf, MAXBUFLEN);
                if (len == 0)					// reached end of file
                    break;
				if (len == -1) {				// error condition
					std::cerr << "Error on socket read from client."
						<< strerror(errno) << std::endl;
					errno = 0;
					delete_flag = true;
					break;
				}
				fwrite(buf, sizeof(char), len, output_file);
                timeout.tv_sec = 30;
                timeout.tv_usec = 0;
			}

			// Close the file, of course.
			fclose(output_file);

			// Delete the file if necessary
			if (delete_flag == true) {
				std::cerr << "Deleting error'd file." << std::endl;
			}
		}	break;

		case FILETABLE:
		{
			// Have the server send over a fresh copy of its file queue so --
			// that we know what's going on.

			// Start off by asking ourselves what we have.
            const char *ret;
			ret = update_filetable(BISON_RECIEVE_DIR, filetable);
			// Oh yeah, and if we failed at that... try again.
			if (ret) {
				std::cerr << "Filetable did not update:" << ret << std::endl;
				std::cerr << "Well, the file recieve directory is being made."
					<< std::endl;
				dirChkCreate(BISON_RECIEVE_DIR.c_str(), "recieve");
				close(sfd);
				return;
			}
			// Ask the server for its file table.
			// (What is this?  A date? -- Brandon)
			dprintf(sfd, "FTREQ\n\n");
			std::map<std::string, std::vector<unsigned char>> tmp_filetable;

			// Parse the filetable information.
			// (Hang in there buddy, it's going to take a little work to --
			// make it all work out. -- Brandon)
			md5_parse(sfd, tmp_filetable);

			// If our current filetable is empty, dump it.
			if (!filequeue.empty())
				std::cerr << "Filetable is not empty. Emptying." << std::endl;
			while(!filequeue.empty())
				filequeue.pop();

			// Check for files here that do not exist on our end.
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
					recalc_queue.push(it->first);
					filequeue.push(it->first);
					continue;
				}
			}

			action = TRANSFER;
		}	break;
		case REALTIME:
			// Not implemented yet, I blame the rest of the requirements.
			break;
		case RECALCULATE_MD5:
		{
			// Oh, we're in trouble.  We need to recalculate a faulty MD5 sum.
			std::cout << "Recalculating for "
				<< recalc_queue.front() << std::endl;
			// Get the file that we need to recalculate
			std::string filenam(recalc_queue.front());
			recalc_queue.pop();

			// Tell the server to recalculate it.
			dprintf(sfd, "RECALC: %s\n\n", filenam.c_str());

			// And recalculate it ourselves.
			recalculate_MD5(BISON_RECIEVE_DIR, filenam, filetable);
		}	break;
	}
	close(sfd);
}

// ======= Configuration Reloading ========
void configuration_reload(int sig)
{
	configure_client(config);
}

// =========== Signal Handling =============
void signalHandling()
{
	signal (SIGHUP, configuration_reload);
	signal(SIGCHLD, SIG_IGN);				// ignore children for now
	// Handle different ways of program termination
	signal(SIGTERM, terminate_nicely);
	signal(SIGINT, terminate_nicely);

}

// ========== The Main Function ============
int main (int argc, char *argv[])
{
	configure_client(config);

	// PID management is offloaded to initialization daemon

	signalHandling();

	dirChkCreate(BISON_RECIEVE_DIR.c_str(), "recieve");

	printf("Client Starting Up...\n");
	action_t action = FILETABLE;

	while(1) {
		handle_connection(action);
		// Actually time things out after return
		// this runs for one second.
		for (int i = 0; i < ERROR_WAIT; i++)
			usleep(1000);
	}
	return 0;
}
