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
#include <iostream>
#include "BISON-Stuff.h"
#define BRANDON_DEBUGGING 1

const char directory_test[] = "test_old/";

#define MAXBUFLEN (255)


void error_terminate(const int status)
{
    exit(status);
}

const char *update_filetable (DIR *directory, std::map<std::string,
std::vector<unsigned char>> &filetable)
{
	struct dirent *dir_ent;
    while ((dir_ent = readdir(directory))) {
        if (dir_ent->d_type != DT_REG) {	// if it is not a file
#if BRANDON_DEBUGGING
			printf("Skipped irregular file, %s\n", dir_ent->d_name);
#endif // BRANDON_DEBUGGING
            continue;
		}
        if (*dir_ent->d_name == '.') {		// if it has a dot
#if BRANDON_DEBUGGING
			printf("Skipped hidden file: %s\n", dir_ent->d_name);
#endif // BRANDON_DEBUGGING
            continue;
		}

		std::vector<unsigned char> sum(16);	// for the MD5 sum storage
        char c[4096];						// sorry I use the same temporary -
											// for everything.
		std::string name(dir_ent->d_name);	// generate a string from the name

		strcpy(c, directory_test);
		strcat(c, name.c_str());	// concatenate to full system path
#if BRANDON_DEBUGGING
		printf("Processing File: %s\n", c);
#endif // BRANDON_DEBUGGING

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

int main ()
{
	DIR *directory;
	std::map<std::string, std::vector<unsigned char>> filetable;

#if BRANDON_DEBUGGING
	printf("Opening directory...\n");
#endif // BRANDON_DEBUGGING
    directory = opendir(directory_test);
    if (!directory)
        error("Could not open BISON-Transfer directory");

	if (const char *ret = update_filetable(directory, filetable))
		error(ret);

	for (std::map<std::string, std::vector<unsigned char>>::iterator 
		it=filetable.begin(); it != filetable.end(); it++) {
		for (std::vector<unsigned char>::iterator iter = it->second.begin();
			iter != it->second.end(); iter++)
			printf("%02x", *iter);			// print MD5 sum
		std::cout << "  " << it->first << std::endl;
											// print filename
	}
    return 0;
}
