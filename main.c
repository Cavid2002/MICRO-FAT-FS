#include "FAT.h"
#include "common.h"
#include <stdio.h>
#include <stdint.h>

FILE* disk;

uint32_t file_write(uint8_t* buff, uint32_t lba, uint32_t sectors)
{
    fwrite(buff, 1, BLOCK_SIZE, disk);
    return BLOCK_SIZE;
}

uint32_t file_read(uint8_t* buff, uint32_t lba, uint32_t sectors)
{
    fread(buff, 1, BLOCK_SIZE, disk);
    return BLOCK_SIZE;
}

int main()
{
    disk = fopen("disk.bin", "r+");
    device_read = file_read;
    device_write = file_write;



}