/*
 * This is a test file for Brandon's random shenanegans. (Is that how you -
 * spell it?)
 */

#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <openssl/md5.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <map>
#include <vector>
#include <string>
#include "BISON-Stuff.h"
#define BRANDON_DEBUGGING 1

const char directory_test[] = "test/";

#define MAXBUFLEN (255)


void error_terminate(const int status)
{
    exit(status);
}

int update_filetable ()
{
#if BRANDON_DEBUGGING
	printf("Opening directory...");
#endif \\ BRANDON_DEBUGGING
    directory = opendir(directory_test);
    if (!directory)
        error("Could not open BISON-Transfer directory");

    while ((dir_ent = readdir(directory))) {
        if (dir_ent->d_type != DT_REG) {	// if it is not a file
#if BRANDON_DEBUGGING
			printf("Skipped irregular file, %s\n", dir_ent->d_name);
#endif \\ BRANDON_DEBUGGING
            continue;
		}
        if (*dir_ent->d_name == '.') {		// if it has a dot
#if BRANDON_DEBUGGING
			printf("Skipped hidden file: %s\n", dir_ent->d_name);
#endif \\ BRANDON_DEBUGGING
            continue;
		}

		std::vector<unsigned char> sum;
        char *c;							// sorry I use the same temporary -
											// for everything.
		std::string name(dir_ent->d_name);

		FILE *fp;
		c = new char [4096];				// throws an exception if it -
											// cannot allocate memory
		strcpy(c, directory_test);
		strcat(c, name.c_str());	// concatenate to full system path

		printf("%s\n", c);					// print the path for debug

		fp = fopen(c, "r");
		if (!fp)
			error("Could not open file in directory!");

		char buf[MAXBUFLEN + 1];

		MD5_CTX md5_structure;
		if (!MD5_Init(&md5_structure))
			error("Could not initialize MD5");

		while (!feof(fp)) {
			size_t newLen = fread(buf, sizeof(char), MAXBUFLEN, fp);
			if (!newLen && errno) {
				error("Could not read from file");
			}
			buf[newLen + 1] = '\0';
			if (!MD5_Update(&md5_structure, buf, newLen))
				error("Could not update MD5 checksum");
		}
		fclose(fp);

		if(!MD5_Final(sum.data(), &md5_structure))
			error("Could not generate final MD5 checksum");

		filetable.insert(std::pair<std::string, std::vector<unsigned char>>
			(name, sum));
/*
Print function can later be used for printing to a file
		for (i = 0; i < 16; i++)
			printf("%02x", sum[i]);
		putchar('\n');
*/
		delete []c;
    }

}

int main ()
{
	DIR *directory;
	std::map<std::string, std::vector<unsigned char>> filetable;
    struct dirent *dir_ent;


    return 0;
}
