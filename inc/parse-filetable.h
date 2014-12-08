/* Copyright (c) 2014 UCR-UAS
 * You should have recieved a copy of the MIT license with this file.
 * Engineer: Brandon Lu
 * Description: This is a parsing function made specfically for the MD5 file -
 * format.
 * What dose it do? MD5 parsing and insanity!
 */
#ifndef __BISON_MD5_PARSE__
#define __BISON_MD5_PARSE__
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
		if (termState)
			throw(7);

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
					throw(2);
				}
				// increment i and take care of sum
				sum[(i++ - 1) / 2] = tmp;
				continue;
			}
		// next two characters are spaces.
		} else if (i >= 32 && i <= 33) {
			// check if spaces
			if (c != ' ')
				throw(4);
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
			<< std::endl;
		exit(1);
	}
}
#endif
