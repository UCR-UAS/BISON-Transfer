/* Engineer: Brandon Lu
 * Description: This is the BISON-Transfer server version 1.
 * What dose it do?  Tarring and insanity.
 * This is indeed a multi-threaded server.
 */

// ================= TODO =================
// Actually read configuration files
// File tracking
// Actually keep track of clients
// Actually log and -HUP support

// =============== Includes ===============
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <exception>
#include <string.h>
#include <boost/filesystem.hpp>
#include <ucontext.h>
#include "BISON-Defaults.h"

/* Terrible global variables */
int sfd;
int MAX_BACKLOG;
std::string BISON_TRANSFER_ADDRESS;
int BISON_TRANSFER_PORT;
int argC;
char **argV;

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

	BISON_TRANSFER_ADDRESS
		= config["BISON-Transfer"]["Server"]["Bind-address"].as<std::string>();
	BISON_TRANSFER_PORT = config["BISON-Transfer"]["Server"]["Port"].as<int>();
	MAX_BACKLOG = config["BISON-Transfer"]["Server"]["Max-Backlog"].as<int>();

	if (DEBUG) {
		std::cout << "Transfer Address (for binding): "
			<< BISON_TRANSFER_ADDRESS << std::endl;
		std::cout << "Transfer port: " << BISON_TRANSFER_PORT << std::endl;
		std::cout << "Max Backlog: " << MAX_BACKLOG << std::endl;
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
	if (pid == 0) {							// we are the child.
		// Tell the client what they're getting
		const char *file_name = "nyan.cat";
		dprintf(cfd, "SENDING: %s\n\n", file_name);

		dprintf(cfd, "Nyans and cats.\n");

		close(cfd);
		exit(0);
	} else {								// we are the parent
		close(cfd);
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
	signal(SIGTERM, terminate_nicely);
	signal(SIGINT, terminate_nicely);
	signal(SIGCHLD, SIG_IGN);				// IGNORE YOUR CHILDREN.
											// TODO: pay attention to children
	YAML::Node config;
	configure_server(config);

	printf("Server Starting Up...\n");

	// Insert PID file management stuff here

	// Write out the configuration before starting 
	boost::filesystem::path dir(__DEF_SERVER_CONFIG_PATH__);
	if (!boost::filesystem::exists(dir)) {
		std::cerr << "Configuration directory does not exist.  Creating..."
			<< std::flush;
		if (boost::filesystem::create_directories(dir))
			std::cout << "Created." << std::endl;
	}
	std::ofstream fout(__DEF_SERVER_CONFIG_PATH__ 
		+ std::string(__DEF_SERVER_CONFIG_FILE__));
	fout << "%YAML 1.2\n" << "---\n";		// version string
	fout << config;
	fout.close();

	prepare_connection();

	// Loop to keep accepting connections
	while (1) {
		handle_connection();
	}

	return 0;
}
