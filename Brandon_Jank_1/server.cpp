/* Engineer: Brandon Lu
 * Description: This is the BISON-Transfer server version 1.
 * What dose it do?  Tarring and insanity.
 * This is indeed a multi-threaded server.
 */

// ================= TODO =================
// File tracking implementation improvement
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

// ========== Filetable Update ============
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

		strcpy(c, BISON_TRANSFER_DIR.c_str());
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
 * Bear with it for now...
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
					DIR *directory;
					directory = opendir(BISON_TRANSFER_DIR.c_str());
					if (!directory)
						error("Could not open directory.");
					const char* ret;
					if ((ret = update_filetable(directory, filetable)))
						error(ret);

					char c[3];							// handle two chars
					std::vector<unsigned char> sum(16);
					std::string filename;
					std::map<std::string, std::vector<unsigned char>>
						tmp_filetable;
					int i = 0;
					int len;
					// read and process into filetable
					while ((len = read((*it)->pipe, c, 2)) > 0) {
						if (i < 16) {		// sum processing
							if (len < 2)
								throw(1);
							unsigned char tmp = 0;
							// process first 4 bits
							if (c[0] >= 'a' && c[0] <= 'f') {
								tmp += (c[0] - 'a' + 10) << 4;
							} else if (c[0] >= 'A' && c[0] <= 'F') {
								tmp += (c[0] - 'A' + 10) << 4;
							} else if (c[0] >= '0' && c[0] <= '9') {
								tmp += (c[0] - '0') << 4;
							} else {
								throw(2);
							}
							// process last 4 bits
							if (c[1] >= 'a' && c[1] <= 'f') {
								tmp += (c[1] - 'a' + 10);
							} else if (c[1] >= 'A' && c[1] <= 'F') {
								tmp += (c[1] - 'A' + 10);
							} else if (c[1] >= '0' && c[1] <= '9') {
								tmp += (c[1] - '0');
							} else {
								throw(2);
							}

							sum[i++] = tmp;
						} else if (i == 16) {
							if (len < 2)
								throw(3);
							if (c[0] != ' ' || c[1] != ' ')
								throw(4);
							i++;
						} else {
							// anything that returns a zero length will not -
							// be here.
							if (len == 2) {
								filename.push_back(c[0]);
								filename.push_back(c[1]);
							} else {
								filename.push_back(c[0]);
							}
						}
					}
					// check for files that exist here that don't exist on -
					// the client
											// the file queue must be empty -
											// if it is not, empty it.
					if (!filequeue.empty())
						std::cerr << "Filequeue is not empty! "
							<< "Prematurely emptying it." << std::endl;
					while (!filequeue.empty())
						filequeue.pop();

					for (std::map<std::string, std::vector<unsigned char>>
						::iterator it = filetable.begin();
						it != filetable.end(); it++) {
						std::cout << "Do we have: " << it->first << std::endl;
						std::map<std::string, std::vector<unsigned char>>
							::iterator it2 = tmp_filetable.find(it->first);
						// handle nonexistent files
						if (it2 == tmp_filetable.end()) {
							std::cout << "Nonexistent file: " << it->first
								<< std::endl;
							filequeue.push(it->first);
							continue;
						}
						// handle inconnect / incomplete files
						if (it2->second != it->second) {
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
