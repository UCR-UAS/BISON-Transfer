CPP=g++
CPPFLAGS=-Wall -ggdb `pkg-config --cflags --libs openssl` -std=c++0x `pkg-config --libs yaml-cpp` -lboost_filesystem -lboost_system -iquote inc/
BINARY_DIR=usr/bin/
INCLUDE_DIR=inc/
SOURCES=BISON-Transferd.cpp BISON-Transfer.cpp
SOURCES_DIR=src/
SOURCES_CPP=$(addprefix $(SOURCES_DIR),$(SOURCES))
INCLUDES-F=BISON-Defaults.h filetable.h config-check.h parse-command.h
INCLUDES=$(addprefix $(INCLUDE_DIR),$(INCLUDES-F))
EXECUTABLES=$(addprefix $(BINARY_DIR),$(SOURCES:.cpp= ))

all: $(BINARY_DIR) $(EXECUTABLES) specs.pdf

$(BINARY_DIR):
	mkdir $(BINARY_DIR)

$(BINARY_DIR)%: $(SOURCES_DIR)%.cpp $(INCLUDES)
	$(CPP) $(CPPFLAGS) $< -o $@

# twice for good measure
specs.pdf: specs.tex
	pdflatex specs.tex
	pdflatex specs.tex
	rm specs.aux specs.log specs.toc specs.out

clean:
	rm -r $(BINARY_DIR) 

