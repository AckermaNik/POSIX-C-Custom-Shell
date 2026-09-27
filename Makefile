
###################################################
#
# file: Makefile
#
# @Author:  Nikoleta Xenaki
# @Version:  14-10-2023
#
# Makefile
#
####################################################

all: shell

CFLAGS = -ansi -pedantic -Wall

shell.o: shell.c declarations.h
	gcc $(CFLAGS) -c  $<

shell: shell.o 
	gcc  $(CFLAGS) shell.o -o $@

run: shell
	./shell


clean:
	-rm shell.o
	-rm shell.out
	