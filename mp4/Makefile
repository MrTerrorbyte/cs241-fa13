# CS 241 Fall 2013
# The University of Illinois

CC = gcc
INC = -I.
FLAGS = -W -Wall -g
LINKOPTS = -lpthread

all: msort gen doc/html

doc/html:
	doxygen doc/Doxyfile

msort: msort.c
	$(CC) $(INC) $(FLAGS) msort.c -o msort $(LINKOPTS)

gen: gen.c
	$(CC) $(INC) $(FLAGS) gen.c -o gen

clean:
	rm -rf msort gen doc/html
