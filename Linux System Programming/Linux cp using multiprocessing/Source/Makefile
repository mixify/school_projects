CC = gcc
ssu_cp : ssu_cp.o dir_help.o cp_header.h
	$(CC) -o $@ ssu_cp.o dir_help.o

ssu_cp.o : ssu_cp.c cp_header.h
	$(CC) -c $*.c
dir_help.o : dir_help.c cp_header.h
	$(CC) -c $*.c
