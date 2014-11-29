CPP=g++
CPPFLAGS=-Wall -ggdb `pkg-config --cflags --libs openssl` -std=c++0x `pkg-config --libs yaml-cpp` -lboost_filesystem -lboost_system -iquote inc/
BINARY_DIR=bin
INCLUDE_DIR=inc
SOURCES_CPP=src/server.cpp src/client.cpp
INCLUDES=BISON-Defaults.h
EXECUTABLES=bin/server bin/client

bin/%: src/%.cpp $(INCLUDE_DIR)/$(INCLUDES)
	$(CPP) $(CPPFLAGS) $< -o $@

all: $(EXECUTABLES)

clean:
	rm $(EXECUTABLES)

