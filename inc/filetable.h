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
const char *recalculate_MD5(const std::string path, const std::string &name,
	std::map<std::string, std::vector<unsigned char>> &filetable)
{
	std::vector<unsigned char> sum(16);	// for the MD5 sum storage
	char c[4096];						// sorry I use the same temporary -
										// for everything.
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

	filetable[name]=(sum);

	return 0;
}


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

		const char *ret = recalculate_MD5(path, std::string(dir_ent->d_name),
			filetable);
		if (ret)
			return ret;
    }
    closedir(directory);
	return 0;
}

// ========== Filetable Parsing ============
void md5_parse(int sfd, std::map<std::string, std::vector<unsigned char>>
	&tmp_filetable)
{
	std::vector<unsigned char> sum(16);
	std::string filename;
	int i = 0;
	// each entry should be in the form of 
	// (32 char md5 sum) (2 spaces) (a filename) (newline)
	char c;
	unsigned char tmp;
	int termState = 0;
	while (read(sfd, &c, 1) > 0) {
		// process whitespaces
		if (c == '\n') {
			// okay, time to end the parsing
			if (i == 0) {
				termState = 1;
				continue;
			}

			// check for premature newlines (filename should -
			// be at least one character long)
			if (i != 34) {
				close(sfd);
				throw(5);
			}

			// push elements into the filetable
			tmp_filetable.emplace(filename, sum);

			// clear filename and leave sum alone.
			filename.clear();
			i = 0;
			continue;
		}

		// throw an exception if this was supposed to -
		// mark a termination but did not
		if (termState) {
			close(sfd);
			throw(7);
		}

		// first 32 characters are md5 sum.
		if (i < 32) {
			// each unsigned char is two hexadecimal characters.
			// process the first hex. character
			if (i % 2 == 0) {
				tmp = 0;
				if (c >= 'a' && c <= 'f') {
					tmp += (c - 'a' + 10) << 4;
				} else if (c >= 'A' && c <= 'F') {
					tmp += (c - 'A' + 10) << 4;
				} else if (c >= '0' && c <= '9') {
					tmp += (c - '0') << 4;
				} else {
					close(sfd);
					throw(2);
				}
				// increment i for next character
				++i;
				continue;
			// process the second hex. character
			} else {
				if (c >= 'a' && c <= 'f') {
					tmp += (c - 'a' + 10);
				} else if (c >= 'A' && c <= 'F') {
					tmp += (c - 'A' + 10);
				} else if (c >= '0' && c <= '9') {
					tmp += (c - '0');
				} else {
					close(sfd);
					throw(2);
				}
				// increment i and take care of sum
				sum[(i++ - 1) / 2] = tmp;
				continue;
			}
		// next two characters are spaces.
		} else if (i >= 32 && i <= 33) {
			// check if spaces
			if (c != ' ') {
				close(sfd);
				throw(4);
			}
			++i;
			continue;
			// everything else is part of the filename
		} else {
			filename.push_back(c);
		}		continue;
	}

	// check if we are allowed to pass
	if (!termState) {
		std::cerr << "Parsing did not complete successfully."
			<< " Unexpected termination." << std::endl;
		close(sfd);
		exit(1);
	}
}
#endif // ifndef __BISON_FILETABLE__
