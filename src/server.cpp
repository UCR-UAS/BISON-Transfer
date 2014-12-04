/* Copyright (c) 2014 UCR-UAS
 * You should have recieved a copy of the MIT licence with this file.
 * Engineer: Brandon Lu
 * Description: This is the BISON-Transfer server version 1.
 * What dose it do?  Tarring and insanity.
 * This is indeed a child-processed server.
 */

// ================= TODO =================
// Multithreading instead of forking?
// File tracking implementation improvement
// Actually keep track of clients
// Actually log and -HUP support
// Debugging times
// Debugging levels

// =============== Includes ===============
#include "BISON-Defaults.h"
#include <boost/circular_buffer.hpp>
#include <boost/filesystem.hpp>
#include <dirent.h>
#include <errno.h>
#include <exception>
#include <fstream>
#include <list>
#include <map>
#include <netinet/in.h>
#include <openssl/md5.h>
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

enum action_t {FILETABLE, TRANSFER, REALTIME};

// =============== Structs =================
struct child_struct_t {
    int pipe;
    enum action_t status;
	boost::circular_buffer_space_optimized<char> buffer;
};

// ===== Terrible global variables  ========
int sfd;
int MAX_BACKLOG;
std::string BISON_TRANSFER_ADDRESS;
int BISON_TRANSFER_PORT;
std::string BISON_TRANSFER_DIR;
enum action_t transmitMode;
int argC;
char **argV;
std::list<child_struct_t*> pipe_back;
std::map<std::string, std::vector<unsigned char>> filetable;
std::queue<std::string> filequeue;

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

	if (!config["BISON-Transfer"]["Server"]["Bind-address"])
		config["BISON-Transfer"]["Server"]["Bind-address"] 
			= DEF_BISON_TRANSFER_BIND;
	if (!config["BISON-Transfer"]["Server"]["Port"])
		config["BISON-Transfer"]["Server"]["Port"] = DEF_BISON_TRANSFER_PORT;
	if (!config["BISON-Transfer"]["Server"]["Max-Backlog"])
		config["BISON-Transfer"]["Server"]["Max-Backlog"] = DEF_MAX_BACKLOG;
	if (!config["BISON-Transfer"]["Server"]["Transmit-Dir"])
		config["BISON-Transfer"]["Server"]["Transmit-Dir"] = DEF_TRANSMIT_DIR;

	BISON_TRANSFER_ADDRESS
		= config["BISON-Transfer"]["Server"]["Bind-address"].as<std::string>();
	BISON_TRANSFER_PORT = config["BISON-Transfer"]["Server"]["Port"].as<int>();
	MAX_BACKLOG = config["BISON-Transfer"]["Server"]["Max-Backlog"].as<int>();
	BISON_TRANSFER_DIR 
		= config["BISON-Transfer"]["Server"]["Transmit-Dir"].as<std::string>();

	if (DEBUG) {
		std::cout << "Transfer Address (for binding): "
			<< BISON_TRANSFER_ADDRESS << std::endl;
		std::cout << "Transfer port: " << BISON_TRANSFER_PORT << std::endl;
		std::cout << "Max Backlog: " << MAX_BACKLOG << std::endl;
		std::cout << "Transmitting: " << BISON_TRANSFER_DIR << std::endl;
	}
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
                                            // clear structure
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

// ========== Connection Handling ==========
void handle_connection()
{
	pid_t pid;
    int cfd;

	// Wait to accept connection
	cfd = accept(sfd, NULL, NULL);
	if (cfd == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			errno = 0;						// There is no error...
			usleep(5000);					// wait a bit so that we won't -
											// overload the system
			return;
		}
		else
			crit_error("Could not accept connections");
	}

	// switch transmitMode to filetable if necessary.
	if (filequeue.empty())
		transmitMode = FILETABLE;

	// piping stuff here
    printf("Piping!\n");
	int pipefd[2];
	if (pipe(pipefd) == -1)
		crit_error("Could not pipe");

	pid = fork();
	if (pid == -1)
		error("Fork");
	if (pid == 0) {							// we are the child.
		close(pipefd[0]);					// child writes to pipe, close read
		// Tell the client what they're getting
		switch (transmitMode) {
			case FILETABLE:					// ask for filetable
			{
				dprintf(cfd, "FTREQ\n\n");
				printf("Recieving filetable and calculating.\n");
				char buf[MAXBUFLEN + 1];
				int len;
				while ((len =read(cfd, buf, 255)) > 0) {
					write(pipefd[1], buf, len);
				}
			}
			break;
			case TRANSFER:					// send next queued file
            {
				std::cout << "Front is: " << filequeue.front() << std::endl;
				std::string file_name(filequeue.front());
				dprintf(cfd, "SENDING: %s\n\n", file_name.c_str());
				FILE *send_me = fopen((BISON_TRANSFER_DIR + file_name).c_str(), "r");
				if (!send_me)
					error("Could not open file for reading.");

				int len;
				char buf[MAXBUFLEN + 1];
				while ((len = fread(buf, sizeof(char), MAXBUFLEN, send_me))> 0) {
					write(cfd, buf, len);
				}

            }
			break;
            case REALTIME:

            break;
		}
		close(pipefd[1]);
		printf("Spawned child exiting.\n");
		exit(0);
	} else {								// we are the parent
        printf("Parent handling child process stuff.\n");
		close(pipefd[1]);					// parent reads from pipe, close w
		close(cfd);
        struct child_struct_t *child_struct = new struct child_struct_t;
		child_struct->buffer.set_capacity(1024*1024);
        std::cout << "Struct created: " <<(void*) child_struct << std::endl;
        child_struct->pipe = pipefd[0];
        printf("Pipe: %d\n", pipefd[0]);
        child_struct->status = transmitMode;
        // temporarily set our file transfer on... this is to be removed later.
        if (transmitMode == FILETABLE)
            transmitMode = TRANSFER;
        /* 
        remove next file.
        if there are no more files to be transferred, switch to filetable.
         */
        pipe_back.push_back(child_struct);
	}
}

// =========== Child Handling =============
/*
 * This is a warning message of some sort.
 *
 * The code that is currently here is all blocking, and it may soon be -
 * replaced with nonblocking code (once Brandon gets how to do so).
 * Bear with it for now...  but the actual filetable processing might still be
 * blocking regardless.
 */
void handle_children()
{
    for (std::list<child_struct_t*>::iterator it = pipe_back.begin();
        it != pipe_back.end(); ) {
                                            // using a (*it) to find out when -
                                            // iterator dereferences to zero
		try {
			switch ((*it)->status) {
				case TRANSFER:
					filequeue.pop();
					// transfer status tracking not implemented yet...
					// cool gui's recommended.
					break;
				case FILETABLE: {
					const char* ret;
					if ((ret = update_filetable(BISON_TRANSFER_DIR, filetable)))
						error(ret);

					char buf[MAXBUFLEN];	
					std::vector<unsigned char> sum(16);
					std::string filename;
					std::map<std::string, std::vector<unsigned char>>
						tmp_filetable;
					int i = 0;
					int len;
					// read all of filetable into a buffer to process later
					std::cout << "Reading file contents into buffer."
						<< std::endl;
					while ((len = read((*it)->pipe, buf, MAXBUFLEN + 1)) > 0) {
						for (int i = 0; i < len; i++) {
							(*it)->buffer.push_back(buf[i]);
						}
					}

					unsigned char tmp;
					std::cout << "Processing buffer..." << std::endl;
					// each entry should be in the form of 
					// (32 char md5 sum) (2 spaces) (a filename) (newline)
					int termState = 0;
					while (!((*it)->buffer.empty())) {
						char c = (*it)->buffer[0];
						(*it)->buffer.pop_front();
						if (c == '\n') {
							// okay, time to end the parsing
							if (i == 0) {
								termState = 1;
								continue;
							}

							// check for premature newlines (filename should -
							// be at least one character long)
							if (i != 34) {
								throw(5);
							}

							// push elements into the filetable
							tmp_filetable.emplace(filename, sum);

							// clear filename and leave sum alone.
							filename.clear();
							i = 0;
							continue;
						}
						
						// throw an exception if this was supposed to -
						// mark a termination but did not
						if (termState)
							throw(7);

						// first 32 characters are md5 sum.
						if (i < 32) {
							if (i % 2 == 0) {
								tmp = 0;
								if (c >= 'a' && c <= 'f') {
									tmp += (c - 'a' + 10) << 4;
								} else if (c >= 'A' && c <= 'F') {
									tmp += (c - 'A' + 10) << 4;
								} else if (c >= '0' && c <= '9') {
									tmp += (c - '0') << 4;
								} else {
									throw(2);
								}
								// increment i for next character
								++i;
								continue;
							} else {
								if (c >= 'a' && c <= 'f') {
									tmp += (c - 'a' + 10);
								} else if (c >= 'A' && c <= 'F') {
									tmp += (c - 'A' + 10);
								} else if (c >= '0' && c <= '9') {
									tmp += (c - '0');
								} else {
									throw(2);
								}
								// increment i and take care of sum
								sum[(i++ - 1) / 2] = tmp;
								continue;
							}
						// next two characters are spaces.
						} else if (i >= 32 && i <= 33) {
							// check if spaces
							if (c != ' ')
								throw(4);
							++i;
							continue;
						// everything else is part of the filename
						} else {
							filename.push_back(c);
							continue;
						}
						throw(6);
					}
					
					// check if we are allowed to pass
					if (!termState) {
						std::cerr << "Parsing did not complete successfully." 
							<< std::endl;
						exit(1);
					}

					// the file queue must be empty -
					// if it is not, empty it.
					if (!filequeue.empty())
						std::cerr << "Filequeue is not empty! "
							<< "Prematurely emptying it." << std::endl;
					while (!filequeue.empty())
						filequeue.pop();

					// check for files that exist here that don't exist on -
					// the client
					for (std::map<std::string, std::vector<unsigned char>>
						::iterator it = filetable.begin();
						it != filetable.end(); it++) {
						std::map<std::string, std::vector<unsigned char>>
							::iterator it2 = tmp_filetable.find(it->first);
						// handle nonexistent files
						if (it2 == tmp_filetable.end()) {
							std::cout << "Client missing: " << it->first
								<< std::endl;
							filequeue.push(it->first);
							continue;
						}
						// handle inconnect / incomplete files
						if (it2->second != it->second) {
							std::cout << " Corrupt file: " << it->first 
								<< std::endl;
							filequeue.push(it->first);
							continue;
						}
					}
					
				} break;
				case REALTIME:
				break;
			}
		} catch (int e) {
			switch (e) {
				default:
					std::cerr << 
						"Back-parsing ended prematurely for unknown reason: "
						<< e << std::endl;
				break;
				case 1:
					std::cerr << "Parsing ended because of client disconnect"
						<< "in md5 sum section." << std::endl;
				break;
				case 2:
					std::cerr << "Invalid character detected in md5 sum."
						<< std::endl;
				break;
				case 3:
					std::cerr << "Parsing ended because of client disconnect"
						<< "in space section." << std::endl;
				break;
				case 4:
					std::cerr << "Invalid character detected in space section"
						<< std::endl;
				break;

			}
		}
        close((*it)->pipe);
        delete (*it);
        pipe_back.erase(it++);
    }
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

int main (int argc, char *argv[])
{
	transmitMode = FILETABLE;				// ask for filetable from client

	signal(SIGTERM, terminate_nicely);
	signal(SIGINT, terminate_nicely);
	signal(SIGCHLD, SIG_IGN);				// IGNORE YOUR CHILDREN.
											// TODO: pay attention to children
	YAML::Node config;
	configure_server(config);

	// Write out the configuration before starting 
	boost::filesystem::path conf_dir(__DEF_SERVER_CONFIG_PATH__);
	if (!boost::filesystem::exists(conf_dir)) {
#if DEBUG
		std::cerr << "Configuration directory does not exist.  Creating..."
			<< std::flush;
		if (boost::filesystem::create_directories(conf_dir))
			std::cout << "Created." << std::endl;
		else
			std::cout << "Not created?" << std::endl;
#else 
		boost::filesystem::create_directories(conf_dir);
#endif // if DEBUG
	}
#if DEBUG
	printf("Writing configuration file...\n");
#endif // if DEBUG
	std::ofstream fout(__DEF_SERVER_CONFIG_PATH__ 
		+ std::string(__DEF_SERVER_CONFIG_FILE__));
	fout << "%YAML 1.2\n" << "---\n";		// version string
	fout << config;
	fout.close();

#if DEBUG
	printf("Checking if transmission directory exists...\n");
#endif // if DEBUG
	boost::filesystem::path xfer_dir(BISON_TRANSFER_DIR);
	if (!boost::filesystem::exists(xfer_dir)) {
#if DEBUG
		std::cerr << "Transfer directory does not exist.  Creating..."
			<< std::flush;
		if (boost::filesystem::create_directories(xfer_dir))
			std::cout << "Created." << std::endl;
		else
			std::cout << "Not created?" << std::endl;
#else 
		boost::filesystem::create_directories(xfer_dir);
#endif // if DEBUG
	}

	printf("Server Starting Up...\n");

	// Insert PID file management stuff here

	prepare_connection();

	// Server ready.
#if DEBUG
	printf("Server ready, waiting for connection.\n");
#endif // if DEBUG

	// Loop to keep accepting connections
	while (1) {
		handle_connection();
		handle_children();
	}

	return 0;
}
