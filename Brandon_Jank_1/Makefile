CC=gcc
CPP=g++
CFLAGS=-Wall `pkg-config --cflags --libs openssl`
CPPFLAGS=$(CFLAGS) -std=c++0x `pkg-config --libs yaml-cpp`
SOURCES=server.c client.c
SOURCES_CPP=test_yaml.cpp test_md5.cpp
INCLUDES=BISON-Stuff.h
EXECUTABLES=$(SOURCES:.c=.o) $(SOURCES_CPP:.cpp=.o)

.c.o : $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

.cpp.o: $(INCLUDES)
	$(CPP) $(CPPFLAGS) $< -o $@

all: $(EXECUTABLES)

clean:
	rm $(EXECUTABLES)

