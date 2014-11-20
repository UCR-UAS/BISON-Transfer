/*
 * This is a test file for Brandon's random shenanegans. (Is that how you -
 * spell it?)
 */

#include <glib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <openssl/md5.h>
#include <stdlib.h>
#include <string.h>
#include "BISON-Stuff.h"

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

// TODO: add in error checking for dynamic memory allocation operations

int main ()
{
    int32_t i;
    GArray *array;
    array = g_array_new (FALSE, FALSE, sizeof(struct md5_t));
    struct dirent *dir_ent;

    printf("Program init.\n");

    printf("Opening a directory.\n");
    directory = opendir(directory_test);
    if (!directory)
        error("Could not open directory.");

    printf("Listing Directory.\n");

    while ((dir_ent = readdir(directory))) {
        if (dir_ent->d_type != DT_REG)
            continue;
        struct md5_t temp_md5_struct;
        char *c;							// sorry I use the same temporary -
											// for everything.
        if (*dir_ent->d_name == '.')
            continue;
		strcpy(temp_md5_struct.name, dir_ent->d_name);
		printf("%s \n", dir_ent->d_name);	// name printing for posterity at -
											// the moment
		printf("Hashing file.\n");			// posterity, once again.

		FILE *fp;
		c = malloc(sizeof(char [4096]));	// TODO
		strcpy(c, directory_test);
		strcat(c, temp_md5_struct.name);

		printf("%s\n", c);

		printf("Opening file \n");
		fp = fopen(c, "r");					// TODO

		char buf[MAXBUFLEN + 1];

		MD5_CTX md5_structure;
		MD5_Init(&md5_structure);			// TODO

		while (!feof(fp)) {
			size_t newLen = fread(buf, sizeof(char), MAXBUFLEN, fp);
											// TODO
			buf[newLen + 1] = '\0';
			MD5_Update(&md5_structure, buf, newLen);
											// TODO
		}
		fclose(fp);

		MD5_Final(temp_md5_struct.sum, &md5_structure);
											// TODO

		for (i = 0; i < 16; i++)
			printf("%x", temp_md5_struct.sum[i]);
		putchar('\n');

		free(c);
    }


    g_array_free(array, TRUE);
    printf("Array test worked!\n");
    return 0;
}
