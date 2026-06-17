

main: mkfs.c FAT.c FAT.h common.c common.h main.c
	gcc -g FAT.c FAT.h common.c common.h main.c -o main
	gcc -g FAT.c FAT.h common.c common.h mkfs.c -o mkfs

.PHONY = debug debug2 run clean

debug: main
	gdb main

debug2: mkfs
	gdb mkfs

run: main
	./main

clean:
	rm main mkfs