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

int fat_create(uint32_t part_start, uint32_t sector_num)
{

}

uint32_t fat_alloc_block()
{
    uint32_t* addr = block_buff;
    uint32_t next = sb.total_block_num - sb.free_block_num;
    uint32_t block_offset = next / ADR_PER_BLOCK;
    uint32_t internal_offset = next % ADR_PER_BLOCK;
    for(int i = block_offset; i < sb.total_block_num; i++)
    {
        device_read(block_buff, PART_START + i + 1);
        for(int j = internal_offset; j < ADR_PER_BLOCK; j++)
        {
            if(addr[j] == 0)
            {
                addr[j] = EOC;
                device_write(block_buff, PART_START + i + 1);
                return (PART_START + i + 1) * ADR_PER_BLOCK + j;
            }
        }
    }
    return FAT_ERR_NO_SPC;
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

uint32_t fat_cluster_insert(uint32_t current, uint32_t next)
{
    uint32_t* addr = block_buff;
    uint32_t block_offset = current / ADR_PER_BLOCK;
    uint32_t internal_offset = current % ADR_PER_BLOCK;
    memset(block_buff, 0, BLOCK_SIZE);
    device_read(block_buff, PART_START + block_offset + 1);
    addr[internal_offset] = next;
    return next;
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

        temp = fat_next_block(next_block);        
        if(temp == EOC)
        {
            temp = alloc_block();
            fat_cluster_insert(next_block, temp);
        }
        
        next_block = temp;

    }
    
    return res;    
}

dir_entry fat_dir_read(dir_entry dir, char* name)
{
    uint32_t len = strlen(name);
    uint32_t next = dir.block_num;
    uint32_t stop = dir.file_size % DIR_PER_BLOCK;
    dir_entry* dirs = block_buff;
    while(dir.file_size > 0)
    {
        device_read(block_buff, next);
        stop = DIR_PER_BLOCK;   
        for(int i = 0; i < stop; i++)
        {
            if(!memcmp(dirs[i].name, name, len) && dirs[i].type == FAT_TYPE_DIR)
            {
                return dirs[i];
            }
        }
        next = fat_next_block(next);
    }
    return (dir_entry){.block_num = FAT_ERR_PATH_ERR};
}

uint32_t fat_dir_write(dir_entry dir, dir_entry new_file)
{
    uint32_t next = dir.block_num;
    for(int i = 0; i < dir.file_size / BLOCK_SIZE; i++)
    {
        next = fat_next_block(next);
    }

    if(dir.file_size % BLOCK_SIZE == 0)
    {
        uint32_t new_block = fat_alloc_block();
        
    }
}


int fat_fopen(char* path, uint8_t mode, file_desc* fd)
{
    char* token = strtok(path, "/");
    char* prev = token;
    uint32_t next = sb.fat_table_size + PART_START + 1;
    dir_entry* dirs = block_buff;
    while(token)
    {
        if(next == FAT_ERR_NOT_EXT) break; 
        
    }

    if(strtok(NULL, "/"))
    {
        return FAT_ERR_PATH_ERR;
    }
    
    if(mode & FAT_MODE_CREATE)
    {
        fat_dir_write(prev, next, FAT_TYPE_FILE);
    }

}