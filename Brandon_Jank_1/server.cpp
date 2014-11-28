/* Engineer: Brandon Lu
 * Description: This is the BISON-Transfer server version 1.
 * What dose it do?  Tarring and insanity.
 * This is indeed a multi-threaded server.
 */

// ================= TODO =================
// File tracking
// Actually keep track of clients
// Actually log and -HUP support
// Debugging times
// Debugging levels

// =============== Includes ===============
#include "BISON-Defaults.h"
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
#include <vector>
#include <yaml-cpp/yaml.h>

enum action_t {FILETABLE, TRANSFER, REALTIME};

// =============== Structs =================
struct child_struct_t {
    int pipe;
    enum action_t status;
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
#if BRANDON_DEBUG
    printf("Socketing...\n");
#endif // if BRANDON_DEBUG
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
				printf("Recieving filetable\n");
				dup2(cfd, pipefd[1]);
                printf("Duped file descriptors.\n");
			}
			break;
			case TRANSFER:					// send next queued file
            {
				const char *file_name = "nyan.cat";
				dprintf(cfd, "SENDING: %s\n\n", file_name);
				dprintf(cfd, "Nyans and cats.\n");
            }
			break;
            case REALTIME:

            break;
		}
        printf("Closing client file descriptor.\n");
		close(pipefd[1]);
        printf("Exiting.\n");
		exit(0);
	} else {								// we are the parent
        printf("Parent handling child process stuff.\n");
		close(pipefd[1]);					// parent reads from pipe, close w
		close(cfd);
        struct child_struct_t *child_struct = new struct child_struct_t;
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
        printf("Exiting function.\n");
	}
}

// =========== Child Handling =============
void handle_children()
{
    char c;
    for (std::list<child_struct_t*>::iterator it = pipe_back.begin();
            it != pipe_back.end() && (*it); ++it) {
                                            // using a (*it) to find out when -
                                            // iterator dereferences to zero
        printf("%d\n", (*it)->pipe);
        while (read((*it)->pipe, &c, 1) != 0) {
            putchar(c);
        }
        close((*it)->pipe);
        delete (*it);
        pipe_back.erase(it);
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
	boost::filesystem::path dir(__DEF_SERVER_CONFIG_PATH__);
	if (!boost::filesystem::exists(dir)) {
#if DEBUG
		std::cerr << "Configuration directory does not exist.  Creating..."
			<< std::flush;
#endif // if DEBUG
		if (boost::filesystem::create_directories(dir)) {
#if DEBUG
			std::cout << "Created." << std::endl;
#endif // if DEBUG
		} else {
#if DEBUG
			std::cout << "Not created?" << std::endl;
#endif // if DEBUG
		}
	}
#if DEBUG
	printf("Writing configuration file...\n");
#endif // if DEBUG
	std::ofstream fout(__DEF_SERVER_CONFIG_PATH__ 
		+ std::string(__DEF_SERVER_CONFIG_FILE__));
	fout << "%YAML 1.2\n" << "---\n";		// version string
	fout << config;
	fout.close();

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
