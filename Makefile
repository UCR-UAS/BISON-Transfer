CPP=g++
CPPFLAGS=-Wall -ggdb `pkg-config --cflags --libs openssl` -std=c++0x `pkg-config --libs yaml-cpp` -lboost_filesystem -lboost_system -iquote inc/
BINARY_DIR=bin/
INCLUDE_DIR=inc/
SOURCES=server.cpp client.cpp
SOURCES_DIR=src/
SOURCES_CPP=$(addprefix $(SOURCES_DIR),$(SOURCES))
INCLUDES-F=BISON-Defaults.h update-filetable.h
INCLUDES=$(addprefix $(INCLUDE_DIR),$(INCLUDES-F))
EXECUTABLES=$(addprefix $(BINARY_DIR),$(SOURCES:.cpp= ))

$(BINARY_DIR)%: $(SOURCES_DIR)%.cpp $(INCLUDES)
	$(CPP) $(CPPFLAGS) $< -o $@

all: $(EXECUTABLES)

clean:
	rm $(EXECUTABLES)

