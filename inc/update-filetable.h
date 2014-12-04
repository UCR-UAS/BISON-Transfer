/* Copyright (c) 2014 UCR-UAS
 * You should have recieved a copy of the MIT licence with this file.
 * Engineer: Brandon Lu
 * Description: This is the filetable generation program for the BISON -
 * software series.
 * What dose it do? It iterates through a directory of files and gives back -
 * a std::map of the files and their respective MD5 sums.
 */
#ifndef __BISON_FILETABLE__
#define __BISON_FILETABLE__
#include <stdlib.h>
#include <string>
#include <string.h>
// ========= Filetable Generation =========
const char *update_filetable (const std::string path, std::map<std::string,
std::vector<unsigned char>> &filetable)
{
    DIR *directory;
    directory = opendir(path.c_str());
    if (!directory)
        return "Could not open directory.";
	struct dirent *dir_ent;
    while ((dir_ent = readdir(directory))) {
        if (dir_ent->d_type != DT_REG) {	// if it is not a file
            continue;
		}
        if (*dir_ent->d_name == '.') {		// if it has a dot
            continue;
		}

		std::vector<unsigned char> sum(16);	// for the MD5 sum storage
        char c[4096];						// sorry I use the same temporary -
											// for everything.
		std::string name(dir_ent->d_name);	// generate a string from the name

		if (filetable.find(name) != filetable.end())
			continue;						// ignore if the filename already -
											// exists in the filetable

		strcpy(c, path.c_str());
		strcat(c, name.c_str());	// concatenate to full system path

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
    }
    closedir(directory);
	return 0;
}

#endif // ifndef __BISON_FILETABLE__
