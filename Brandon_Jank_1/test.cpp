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
#include "BISON-Stuff.h"
#define BRANDON_DEBUGGING 1

const char directory_test[] = "test/";

struct md5_t {
    unsigned char sum[32];
    char name[255];
};

#define MAXBUFLEN (255)

DIR *directory;

void error_terminate(const int status)
{
    closedir(directory);
    exit(status);
}

int main ()
{
    int32_t i;
    struct dirent *dir_ent;

    directory = opendir(directory_test);
    if (!directory)
        error("Could not open directory.");

    while ((dir_ent = readdir(directory))) {
        if (dir_ent->d_type != DT_REG)
            continue;
        struct md5_t temp_md5_struct;
        char *c;							// sorry I use the same temporary -
											// for everything.
        if (*dir_ent->d_name == '.')
            continue;
		strcpy(temp_md5_struct.name, dir_ent->d_name);

		FILE *fp;
		c = new char [4096];				// throws an exception if it -
											// cannot allocate memory
		strcpy(c, directory_test);
		strcat(c, temp_md5_struct.name);

		printf("%s\n", c);					// print the path

		fp = fopen(c, "r");
		if (!fp)
			error("Could not open file in directory!\n");

		char buf[MAXBUFLEN + 1];

		MD5_CTX md5_structure;
		if (!MD5_Init(&md5_structure))
			error("Could not initialize MD5.\n");

		while (!feof(fp)) {
			size_t newLen = fread(buf, sizeof(char), MAXBUFLEN, fp);
			if (!newLen && errno) {
				error("Could not read from file!");
			}
			buf[newLen + 1] = '\0';
			MD5_Update(&md5_structure, buf, newLen);
											// TODO
		}
		fclose(fp);

		MD5_Final(temp_md5_struct.sum, &md5_structure);
											// TODO

		for (i = 0; i < 16; i++)
			printf("%02x", temp_md5_struct.sum[i]);
		putchar('\n');

		free(c);
    }


    return 0;
}
