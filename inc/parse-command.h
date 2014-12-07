#ifndef __BISON_ACTION_PARSER__
#define __BISON_ACTION_PARSER__

typedef enum {NONE, SEND, FILETABLE_REQ} action_t;

void parse_command(int sfd, action_t &action, std::string &filename)
{
	int since_last_newline = -1;
    int count = 0;
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
					std::cerr << "Fake client detected?" << std::endl;
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
			count = 6;
			continue;
		} else if (count == 6 && c == 'T') {
			++count;
			continue;
		} else if (count == 7 && c == 'R') {
			++count;
			continue;
		} else if (count == 8 && c == 'E') {
			++count;
			continue;
		} else if (count == 9 && c == 'Q') {
			++count;
			action = FILETABLE_REC;
			continue;
		} else if (count == 0 && c == 'R') {
            ++count;
            continue;
        } else if (count == 1 && c == 'E') {
            ++count;
            continue;
        } else if (count == 2 && c == 'Q') {
            ++count;
            continue;
        } else if (count == 3 && c == ':') {
            ++count;
            continue;
        } else if (count == 4 && c == ' ') {
            ++count;
			action = SEND;
            continue;
        } else if (action == SEND && count == 5) {
            filename.push_back(c);
		    since_last_newline = -1;
        } else {
			std::cerr << "Fake client detected? Action not set." << std::endl;
			exit(1);
		}
	}
}

#endif // ifndef __BISON_ACTION_PARSER__