CC=gcc
CPP=g++
CFLAGS=-Wall `pkg-config --cflags --libs openssl`
SOURCES=server.c client.c
SOURCES_CPP=test.cpp
INCLUDES=BISON-Stuff.h
EXECUTABLES=$(SOURCES:.c=.o) $(SOURCES_CPP:.cpp=.o)

.c.o : $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

.cpp.o: $(INCLUDES)
	$(CPP) $(CFLAGS) $< -o $@

all: $(EXECUTABLES)

clean:
	rm $(EXECUTABLES)

