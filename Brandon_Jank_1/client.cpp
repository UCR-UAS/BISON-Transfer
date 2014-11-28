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
#include <arpa/inet.h>
#include "BISON-Defaults.h"
#include <boost/filesystem.hpp>
#include <dirent.h>
#include <errno.h>
#include <exception>
#include <fstream>
#include <map>
#include <netinet/in.h>
#include <openssl/md5.h>
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

// =========== Global Variables ===========
int sfd;
std::string BISON_TRANSFER_SERVER;
int BISON_TRANSFER_PORT;
std::string BISON_RECIEVE_DIR;

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

// ========= Filetable Generation =========
const char *update_filetable (DIR *directory, std::map<std::string,
std::vector<unsigned char>> &filetable)
{
	struct dirent *dir_ent;
    while ((dir_ent = readdir(directory))) {
        if (dir_ent->d_type != DT_REG) {	// if it is not a file
#if DEBUG
			printf("Skipped irregular file, %s\n", dir_ent->d_name);
#endif // DEBUG
            continue;
		}
        if (*dir_ent->d_name == '.') {		// if it has a dot
#if DEBUG
			printf("Skipped hidden file: %s\n", dir_ent->d_name);
#endif // DEBUG
            continue;
		}

		std::vector<unsigned char> sum(16);	// for the MD5 sum storage
        char c[4096];						// sorry I use the same temporary -
											// for everything.
		std::string name(dir_ent->d_name);	// generate a string from the name

		strcpy(c, BISON_RECIEVE_DIR.c_str());
		strcat(c, name.c_str());	// concatenate to full system path
#if DEBUG
		printf("Processing File: %s\n", c);
#endif // DEBUG

		FILE *fp = fopen(c, "r");
		if (!fp)
			return "Could not open file in directory!";

		char buf[MAXBUFLEN + 1];

		MD5_CTX md5_structure;
		if (!MD5_Init(&md5_structure))
			return "Could not initialize MD5";

		while (!feof(fp)) {
			size_t newLen = fread(buf, sizeof(char), MAXBUFLEN, fp);
			if (!newLen && errno) {
				return "Could not read from file";
			}
			buf[newLen + 1] = '\0';
			if (!MD5_Update(&md5_structure, buf, newLen))
				return "Could not update MD5 checksum";
		}
		fclose(fp);

		if(!MD5_Final(sum.data(), &md5_structure))
			return "Could not generate final MD5 checksum";

		filetable.insert(std::pair<std::string, std::vector<unsigned char>>
			(name, sum));
		printf("Inserted: %s into filetable\n", name.c_str());
    }
	return 0;
}

int main (int argc, char *argv[])
{
	YAML::Node config;
	configure_client(config);

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

	printf("Client Starting Up...\n");

	// Insert PID file management stuff here

	signal(SIGCHLD, SIG_IGN);				// ignore children for now
	// Handle different ways of program termination
	signal(SIGTERM, terminate_nicely);
	signal(SIGINT, terminate_nicely);

#if DEBUG
	printf("Checking if BISON-Recieve directory exists.\n");
#endif // if DEBUG
	boost::filesystem::path dir_chk(BISON_RECIEVE_DIR);
	if (!boost::filesystem::exists(dir_chk)) {
#if DEBUG
	printf("Recieve directory does not exist.  Creating...");
			if (boost::filesystem::create_directories(dir_chk)) {
				std::cout << "Created." << std::endl;
			} else {
				std::cout << "Not created? continuing." << std::endl;
			}
#else
			boost::filesystem::create_directories(dir_chk);
#endif
	}
	DIR *directory = opendir(BISON_RECIEVE_DIR.c_str());
	std::map<std::string, std::vector<unsigned char>> filetable;

	const char *ret;
	ret = update_filetable(directory, filetable);
	if (ret) {
		std::cerr << "Filetable did not update:" << ret << std::endl;
		exit(1);
	}

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

	int since_last_newline = -1;
    int count = 0;
    std::string filename;
	enum action_t {NONE, RECIEVE, FILETABLE_SEND} action = NONE;
	char c;
	// Excuse the ugly, difficult parser.
	while(1) {
		if(read(sfd, &c, 1) < 1) {
			std::cerr << "Could not read from file descriptor.Is this the end?" 
				<< std::endl;
			exit(1);
		}
		if (c == '\n') {
			if (since_last_newline == 0) {
				if (action != NONE)
					break;
				else {
					std::cerr << "Fake server detected?" << std::endl;
					exit(1);
				}
			} else {
				if (action == NONE) {
					std::cerr << "Recieved newline without correct action."
						<< std::endl;
					exit(1);
				}
				++since_last_newline;
				continue;
			}
		} else if (count == 0 && c == 'F') {
			count = 9;
			continue;
		} else if (count == 9 && c == 'T') {
			++count;
			continue;
		} else if (count == 10 && c == 'R') {
			++count;
			continue;
		} else if (count == 11 && c == 'E') {
			++count;
			continue;
		} else if (count == 12 && c == 'Q') {
			++count;
			action = FILETABLE_SEND;
			continue;
		} else if (count == 0 && c == 'S') {
            ++count;
            continue;
        } else if (count == 1 && c == 'E') {
            ++count;
            continue;
        } else if (count == 2 && c == 'N') {
            ++count;
            continue;
        } else if (count == 3 && c == 'D') {
            ++count;
            continue;
        } else if (count == 4 && c == 'I') {
            ++count;
            continue;
        } else if (count == 5 && c == 'N') {
            ++count;
            continue;
        } else if (count == 6 && c == 'G') {
            ++count;
            continue;
        } else if (count == 7 && c == ':') {
            ++count;
            continue;
        } else if (count == 8 && c == ' ') {
            ++count;
			action = RECIEVE;
            continue;
        } else if (action != RECIEVE) {
            filename.push_back(c);
		    since_last_newline = -1;
        } else {
			std::cerr << "Fake server detected? Action not set." << std::endl;
			exit(1);
		}
	}

	switch (action) {
		case NONE:
			std::cerr << "Uncaught faulty server." << std::endl;
			exit(1);
		break;
		case RECIEVE: {
#if DEBUG
			std::cout << "output file: " << filename << std::endl;
#endif // if DEBUG
			FILE *output_file = fopen(filename.c_str(), "w");
			if (output_file == 0)
				error("Could not open new file for writing.");

			char buf[MAXBUFLEN + 1];
			int len = 0;
			while((len = read(sfd, buf, MAXBUFLEN)) > 0) {
											// Now works with binary!
				fwrite(buf, sizeof(char), len, output_file);
			}

			fclose(output_file);
		}
		break;
		case FILETABLE_SEND:
			for (std::map<std::string, std::vector<unsigned char>>::iterator 
				it=filetable.begin(); it != filetable.end(); it++) {
				for (std::vector<unsigned char>::iterator iter
					= it->second.begin(); iter != it->second.end(); iter++)
					dprintf(sfd, "%02x", *iter);	// print MD5 sum
				dprintf(sfd, "  %s\n", it->first.c_str());
													// print filename
			}
		break;
	}
			

	close(sfd);
	return 0;
}
