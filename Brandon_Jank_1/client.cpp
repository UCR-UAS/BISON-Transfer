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
#include <string>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <boost/filesystem.hpp>
#include "BISON-Defaults.h"

// =========== Global Variables ===========
int sfd;
std::string BISON_TRANSFER_SERVER;
int BISON_TRANSFER_PORT;

// ============ Configuration ==============
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

	BISON_TRANSFER_SERVER
		= config["BISON-Transfer"]["Client"]["Server-address"]
		.as<std::string>();
	BISON_TRANSFER_PORT = config["BISON-Transfer"]["Client"]["Port"].as<int>();

	if (DEBUG) {
		std::cout << "Transfer Address (for binding): "
			<< BISON_TRANSFER_SERVER << std::endl;
		std::cout << "Transfer port: " << BISON_TRANSFER_PORT << std::endl;
	}
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

int main (int argc, char *argv[])
{
	YAML::Node config;
	configure_client(config);
	printf("Client Starting Up...\n");

	// Insert PID file management stuff here

	signal(SIGCHLD, SIG_IGN);				// ignore children for now
	// Handle different ways of program termination
	signal(SIGTERM, terminate_nicely);
	signal(SIGINT, terminate_nicely);

	struct sockaddr_in srv_addr;

#if DEBUG
	printf("Socketing... \n");
#endif // if DEBUG
	sfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sfd == -1)
		crit_error("Socket");

	// set things for sockaddr_in before binding
	memset(&srv_addr, 0, sizeof(struct sockaddr_in));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(BISON_TRANSFER_PORT);
	// TODO: Preliminary testing removal, insertion of actual stuff
	if (!inet_aton(BISON_TRANSFER_SERVER.c_str(),
		(struct in_addr*) &srv_addr.sin_addr.s_addr))
		error("Server Address Invalid");

	// bind
#if DEBUG
	printf("Binding... \n");
#endif // if DEBUG
	if (connect(sfd, (struct sockaddr*) &srv_addr, sizeof(struct sockaddr_in))
			== -1)
		crit_error("Connect Failed");

#if DEBUG
	std::cout << "Writing config file" << std::endl;
#endif // if DEBUG

	// Write out the configuration before starting 
	boost::filesystem::path dir(__DEF_CLIENT_CONFIG_PATH__);
	if (!boost::filesystem::exists(dir)) {
		std::cerr << "Configuration directory does not exist.  Creating..."
			<< std::flush;
		if (boost::filesystem::create_directories(dir))
			std::cout << "Created." << std::endl;
	}
	std::ofstream fout(__DEF_CLIENT_CONFIG_PATH__
		+ std::string(__DEF_CLIENT_CONFIG_FILE__));
	fout << "%YAML 1.2\n" << "---\n";
	fout << config;

	int since_last_newline = -1;
	while(1) {
		char c;
		if(!read(sfd, &c, 1)) {
			std::cerr << "Could not read from file descriptor.Is this the end?" 
				<< std::endl;
			exit(1);
		}
		if (c == '\n') {
			printf("%d\n", since_last_newline);
			putchar(c);
			if (since_last_newline == 0) {
				break;
			} else {
				++since_last_newline;
				continue;
			}
		}
		since_last_newline = -1;
		putchar(c);
	}

	dup2(sfd, STDIN_FILENO);				// prepare input for tar

	char* args[] = {"tar", "-xz", 0};
	execvp(args[0], args);
	// included for posterity, not functionality.  exec() will not bring us -
	// back
	close(sfd);
	return 0;
}
