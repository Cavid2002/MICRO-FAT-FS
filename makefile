

main: mkfs.c FAT.c FAT.h common.c common.h main.c
	gcc -g FAT.c FAT.h common.c common.h main.c -o main
	gcc -g FAT.c FAT.h common.c common.h mkfs.c -o mkfs

.PHONY = debug debug2 run clean disk


debug: main
	gdb main

debug2: mkfs
	gdb mkfs

run: main
	./main

disk:
	rm -f disk.bin
	truncate --size 512M disk.bin
	

clean:
	rm main mkfs disk.bin