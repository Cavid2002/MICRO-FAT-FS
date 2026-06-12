

main: FAT.c FAT.h common.c common.h main.c
	gcc -g FAT.c FAT.h common.c common.h main.c -o main


.PHONY = debug run

debug: main
	gdb main


run: main
	./main
