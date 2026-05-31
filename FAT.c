#include "FAT.h"
#include "common.h"

static super_block sb;
uint8_t block_buff[BLOCK_SIZE];
file_desc fd_table[MAX_OPEN_FILE];
uint32_t fd_bitmap = 0;

int fat_mount()
{
    device_read(block_buff, PART_START, SECTOR_NUM);
    memcpy(block_buff, &sb, sizeof(super_block));
    memset(fd_table, 0, sizeof(fd_table) * MAX_FILE_NAME);       
    return sb.fat_magic == FAT_MAGIC;
}

int fat_create(uint32_t part_start, uint32_t sector_num)
{

}

int fat_get_free_fd()
{
    uint8_t max = 1 << (MAX_OPEN_FILE - 1);
    while(max)
    {
        if(!(fd_bitmap & max))
        {
            return max;
        }
        max = max >> 1;
    }
    return -1;
}

uint32_t fat_alloc_block()
{
    uint32_t* addr = block_buff;
    uint32_t next = sb.total_block_num - sb.free_block_num;
    uint32_t block_offset = next / ADR_PER_BLOCK;
    uint32_t internal_offset = next % ADR_PER_BLOCK;
    for(int i = block_offset; i < sb.total_block_num; i++)
    {
        device_read(block_buff, PART_START + i + 1, SECTOR_NUM);
        for(int j = internal_offset; j < ADR_PER_BLOCK; j++)
        {
            if(addr[j] == 0)
            {
                addr[j] = EOC;
                device_write(block_buff, PART_START + i + 1, SECTOR_NUM);
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
    device_read(block_buff, PART_START + block_offset + 1, SECTOR_NUM);
    return addr[internal_offset];
}

uint32_t fat_cluster_insert(uint32_t current, uint32_t next)
{
    uint32_t* addr = block_buff;
    uint32_t block_offset = current / ADR_PER_BLOCK;
    uint32_t internal_offset = current % ADR_PER_BLOCK;
    memset(block_buff, 0, BLOCK_SIZE);
    device_read(block_buff, PART_START + block_offset + 1, SECTOR_NUM);
    addr[internal_offset] = next;
    return next;
}

uint32_t fat_fread(int fdi, uint8_t* buff, uint32_t size)
{
    file_desc* fd = fd_table + fdi;
    uint32_t internal_offset ,bytes_to_read;
    uint32_t next_block = fd->curr_block;
    if(fd->offset + size > fd->fdir.file_size) 
        size = fd->fdir.file_size - fd->offset;   
    
    if(fd->offset == fd->fdir.file_size) 
        return 0;

    
    uint32_t res = 0;
    while(size > 0)
    {
        internal_offset = fd->offset % BLOCK_SIZE;
        bytes_to_read = BLOCK_SIZE - internal_offset;
        if(size < bytes_to_read) bytes_to_read = size;
        
        device_read(block_buff, next_block, SECTOR_NUM);
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


uint32_t fat_fwrite(int fdi, uint8_t* buff, uint32_t size)
{
    file_desc* fd = fd_table + fdi;
    uint32_t internal_offset, bytes_to_write;
    uint32_t next_block = fd->curr_block;
    uint32_t temp;

    int res = 0;
    while(size > 0)
    {
        internal_offset = fd->offset % BLOCK_SIZE;
        bytes_to_write = BLOCK_SIZE - internal_offset;
        if(size < bytes_to_write) bytes_to_write = size;
        
        if(internal_offset != 0) 
            disk_read(block_buff, next_block, SECTOR_NUM);
    
        memcpy(buff, block_buff + internal_offset, bytes_to_write);
        disk_write(block_buff, next_block, SECTOR_NUM);
        
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

dir_entry fat_dir_read(dir_entry* dir, char* name)
{
    uint32_t len = strlen(name);
    uint32_t next = dir->block_num;
    uint32_t stop = dir->file_size % DIR_PER_BLOCK;
    dir_entry* dirs = block_buff;
    while(dir->file_size > 0)
    {
        device_read(block_buff, next, SECTOR_NUM);
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

dir_entry fat_create_file(char* filename, uint8_t type)
{
    dir_entry new_file;
    uint32_t size = strlen(filename);
    size = size > MAX_FILE_NAME ? MAX_FILE_NAME : size;
    
    memset(&new_file, 0, sizeof(dir_entry));
    new_file.block_num = fat_alloc_block();
    new_file.file_size = size;
    new_file.type = type;
    memcpy(filename, new_file.name, size);
    return new_file;
}

int fat_dir_write(dir_entry* dir, dir_entry* new_file)
{
    dir_entry* dirs = block_buff;
    uint32_t next = dir->block_num;
    for(int i = 0; i < dir->file_size / BLOCK_SIZE; i++)
    {
        next = fat_next_block(next);
    }

    if(dir->file_size % BLOCK_SIZE == 0)
    {
        uint32_t new_block = fat_alloc_block();
        if(new_block == FAT_ERR_NO_SPC) return FAT_ERR_NO_SPC;
        fat_cluster_insert(next, new_block);
        memset(block_buff, 0, BLOCK_SIZE);
        memcpy(&new_file, block_buff, sizeof(new_file));
        device_write(block_buff, new_block, SECTOR_NUM);
    }

    device_read(block_buff, next, SECTOR_NUM);
    dirs[dir->file_size % DIR_PER_BLOCK] = *new_file;
    device_write(block_buff, next, SECTOR_NUM);
    return 0;
}


int fat_fopen(char* path, uint8_t mode)
{
    char* token = strtok(path, "/");
    char* prev_token = token;
    dir_entry next = sb.root;
    dir_entry prev = next;
    dir_entry* dirs = block_buff;
    uint8_t flag = 0;
    while(token)
    {
        if(next.block_num == FAT_ERR_NOT_EXT)
        {
            flag = 1;
            break;
        }
        prev = next;
        next = fat_dir_read(&next, token);
        prev_token = token;
        token = strtok(NULL, "/");
    }

    if(flag && strtok(NULL, "/"))
    {
        return FAT_ERR_PATH_ERR;
    }
    
    if(flag && (mode & FAT_MODE_CREATE))
    {
        dir_entry new_file = fat_create_file(prev_token, FAT_TYPE_FILE);
        
        return fat_dir_write(&prev, &new_file);
    }

    
}

int fat_fclose(int fd)
{
    if(fd->offset < fd->file_size)
    {
        return 0;
    }

    for(int i = 0; i )
}