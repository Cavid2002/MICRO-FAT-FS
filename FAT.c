#include "FAT.h"
#include "common.h"

uint8_t block_buff[BLOCK_SIZE];
static super_block sb;

int fat_mount()
{
    device_read(block_buff, PART_START);
    memcpy(block_buff, &sb, sizeof(super_block));
        
    return sb.fat_magic == FAT_MAGIC;
}


uint32_t fat_next_block(uint32_t current_block)
{
    uint32_t* addr = block_buff;
    uint32_t block_offset = current_block / ADR_PER_BLOCK;
    uint32_t internal_offset = current_block % ADR_PER_BLOCK;
    memset(block_buff, 0, BLOCK_SIZE);
    device_read(block_buff, PART_START + block_offset + 1);
    return addr[internal_offset];
}

uint32_t fat_fread(file_desc* fd, uint8_t* buff, uint32_t size)
{
    uint32_t internal_offset ,bytes_to_read;
    uint32_t next_block = fd->curr_block;
    if(fd->offset + size > fd->file_size) 
        size = fd->file_size - fd->offset;   
    
    if(fd->offset == fd->file_size) 
        return 0;

    
    uint32_t res = 0;
    while(size > 0)
    {
        internal_offset = fd->offset % BLOCK_SIZE;
        bytes_to_read = BLOCK_SIZE - internal_offset;
        if(size < bytes_to_read) bytes_to_read = size;
        
        device_read(block_buff, next_block);
        memcpy(block_buff + internal_offset, buff, bytes_to_read);
        
        buff += bytes_to_read;
        size -= bytes_to_read;
        fd->offset += bytes_to_read;
        res += bytes_to_read;

        next_block = fat_next_block(next_block);
    }
    
    fd->curr_block = next_block;
    return res;

}

uint32_t fat_fwrite(file_desc* fd, uint8_t* buff, uint32_t size)
{
    uint32_t internal_offset, bytes_to_write;
    uint32_t next_block = fd->curr_block;
    uint32_t temp;

    int res = 0;
    while(size > 0)
    {
        internal_offset = fd->offset % BLOCK_SIZE;
        bytes_to_write = BLOCK_SIZE - internal_offset;
        if(size < bytes_to_write) bytes_to_write = size;
        
        if(internal_offset != 0) disk_read(block_buff, next_block);
        memcpy(buff, block_buff + internal_offset, bytes_to_write);
        disk_write(block_buff, next_block);
        
        buff += bytes_to_write;
        size -= bytes_to_write;
        fd->offset += bytes_to_write;
        res += bytes_to_write;

        temp = get_next_block(next_block);        
        if(temp == EOC)
        {
            temp = alloc_block();
            add_to_cluster(next_block, temp);
        }
        
        next_block = temp;

    }
    
    return res;    
}

