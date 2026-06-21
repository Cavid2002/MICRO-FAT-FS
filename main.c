#include "FAT.h"
#include "common.h"
#include <stdio.h>
#include <stdint.h>

FILE* disk;

int disk_start()
{
    disk = fopen("disk.bin", "r+");
    if(disk == NULL) return 1;
    return 0;
}

uint32_t disk_write(uint8_t* buff, uint32_t lba, uint32_t sectors)
{
    fseek(disk, lba * BLOCK_SIZE, SEEK_SET);
    return fwrite(buff, 1, BLOCK_SIZE * sectors, disk);
}

uint32_t disk_read(uint8_t* buff, uint32_t lba, uint32_t sectors)
{
    fseek(disk, lba * BLOCK_SIZE, SEEK_SET);
    return fread(buff, 1, BLOCK_SIZE * sectors, disk);
}


void disk_stop()
{
    fclose(disk);
}

int main()
{
    if(disk_start() != 0) return -1;
    device_read = disk_read;
    device_write = disk_write;

    fat_cb cb;

    if(fat_mount(&cb, 0))
    {
        printf("FAT DETECTED!!\n");
    }

    file_desc fd;
    char path[] = "/test.txt";
    if(fat_fopen(&fd, path, FAT_MODE_CREATE | FAT_MODE_WRITE) == 0)
    {
        printf("FILE OPENED!\n");
    }
    else
    {
        printf("FAIL\n");
        return 0;
    }


    char buff[1024] = "HELLO WORLD! Testing Data Precense in the file";
    char temp[20];
    fat_fwrite(&fd, (uint8_t*)buff, 1024);
    fat_fseek(&fd, 0, FAT_SEEK_SET);
    fat_fread(&fd, (uint8_t*)temp, 13);

    printf("%s\n", temp);

    fat_fclose(&fd);


    fat_umount(&cb);
    disk_stop();
    return 0;

}