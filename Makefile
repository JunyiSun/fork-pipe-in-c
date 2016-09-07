
all : phist

phist : phist.c
				gcc -Wall -std=c99 -g -o phist phist.c
