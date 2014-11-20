CC=gcc
CFLAGS=-Wall `pkg-config --cflags --libs glib-2.0``pkg-config --cflags --libs openssl`
SOURCES=server.c client.c test.c
INCLUDES=BISON-Stuff.h
EXECUTABLES=$(SOURCES:.c=.o)

.c.o : $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@

all: $(EXECUTABLES)

clean:
	rm $(EXECUTABLES)

