#include "common.h"
#include "FAT.h"

uint32_t (*device_read)(uint8_t* buff, uint32_t lba, uint32_t sectors);
uint32_t (*device_write)(uint8_t* buff, uint32_t lba, uint32_t sectors);
static super_block* sb;
static uint8_t* block_buff;
static uint32_t fat_offset;

int fat_mount(fat_cb* cb, uint32_t part_start)
{
    fat_offset = part_start + 1;
    sb = &cb->s_block;
    block_buff = cb->block_buff;
    device_read(block_buff, part_start, SECTOR_NUM);
    memcpy(block_buff, sb, sizeof(super_block));
    return sb->fat_magic == FAT_MAGIC;
}

int fat_umount(fat_cb* cb)
{
    if(&cb->s_block != sb) return -1;
    if(memcmp(sb, &cb->s_block, sizeof(super_block)) == 0) return 0;

    memset(block_buff, 0, BLOCK_SIZE);
    memcpy(sb, block_buff, sizeof(super_block));
    device_write(block_buff, fat_offset - 1, SECTOR_NUM);
    return 0;
}

int fat_mkfs(fat_cb* cb, uint32_t part_start, uint32_t sector_num)
{
    fat_offset = part_start + 1;
    sb = &cb->s_block;
    block_buff = cb->block_buff;
    fat_offset = part_start + 1;
    sb->fat_magic = FAT_MAGIC;
    sb->block_size = BLOCK_SIZE;
    sb->total_block_num = (sector_num * 512) / BLOCK_SIZE;
    sb->fat_table_size = (sb->total_block_num * 4 + BLOCK_SIZE - 1) / BLOCK_SIZE;
    sb->free_block_num = sb->total_block_num - sb->fat_table_size - 1;

    memset(block_buff, 0, BLOCK_SIZE);
    for(int i = 0; i < sb->fat_table_size; i++)
    {
        device_write(block_buff, fat_offset + i, SECTOR_NUM);
    }

    int i = 0;
    uint32_t temp = (sb->fat_table_size + 1);
    memset(block_buff, 0xFF, BLOCK_SIZE);

    for(i = 0; i < temp / ADR_PER_BLOCK; i++)
    {
        device_write(block_buff, fat_offset + i, SECTOR_NUM);
    }
    memset(block_buff, 0, BLOCK_SIZE);
    memset(block_buff, 0xFF, (temp % ADR_PER_BLOCK) * sizeof(uint32_t));
    device_write(block_buff, fat_offset + i, SECTOR_NUM);


    sb->root.block_num = fat_alloc_block();
    sb->root.file_size = 0;
    sb->root.type = FAT_TYPE_DIR;
    sb->root.dir_block = sb->root.block_num;

    memcpy(sb, block_buff, sizeof(super_block));
    device_write(block_buff, part_start, SECTOR_NUM);
    return 0;
}


uint32_t fat_alloc_block()
{
    uint32_t* addr = (uint32_t*)block_buff;
    uint32_t next = sb->total_block_num - sb->free_block_num - 1;
    uint32_t block_offset = next / ADR_PER_BLOCK;
    uint32_t internal_offset = next % ADR_PER_BLOCK;
    uint32_t res = 0;
    for(int i = block_offset; i < sb->fat_table_size; i++)
    {
        device_read(block_buff, fat_offset + i, SECTOR_NUM);
        for(int j = internal_offset; j < ADR_PER_BLOCK; j++)
        {
            if(addr[j] == 0)
            {
                addr[j] = EOC;
                device_write(block_buff, fat_offset + i, SECTOR_NUM);
                sb->free_block_num--;
                res = (i * ADR_PER_BLOCK + j) + fat_offset;
                return res;
            }
        }
        internal_offset = 0;
    }
    return FAT_ERR_NO_SPC;
}

void fat_free_block(uint32_t block_num)
{
    uint32_t block_offset = block_num / ADR_PER_BLOCK;
    uint32_t internal_offset = block_num % ADR_PER_BLOCK;
    uint32_t* addr = (uint32_t*)block_buff;
    device_read(block_buff, fat_offset + block_offset, SECTOR_NUM);
    addr[internal_offset] = 0;
    device_write(block_buff, fat_offset + block_offset, SECTOR_NUM);
}

uint32_t fat_next_block(uint32_t current_block)
{
    uint32_t* addr = (uint32_t*)block_buff;
    uint32_t block_offset = current_block / ADR_PER_BLOCK;
    uint32_t internal_offset = current_block % ADR_PER_BLOCK;
    memset(block_buff, 0, BLOCK_SIZE);
    device_read(block_buff, fat_offset + block_offset, SECTOR_NUM);
    return addr[internal_offset];
}

uint32_t fat_cluster_insert(uint32_t current, uint32_t next)
{
    uint32_t* addr = (uint32_t*)block_buff;
    uint32_t block_offset = current / ADR_PER_BLOCK;
    uint32_t internal_offset = current % ADR_PER_BLOCK;
    memset(block_buff, 0, BLOCK_SIZE);
    device_read(block_buff, fat_offset + block_offset, SECTOR_NUM);
    addr[internal_offset] = next;
    return next;
}


int fat_dir_read(dir_entry* dir, dir_entry* res, char* name)
{
    uint32_t len = strlen(name);
    uint32_t next = dir->block_num;
    dir_entry* dirs = (dir_entry*)block_buff;

    if(len == 0)
    {
        *res = *dir;
        return 0;
    }

    while(next != EOC)
    {
        device_read(block_buff, next, SECTOR_NUM);
        for(int i = 0; i < DIR_PER_BLOCK; i++)
        {
            if(!memcmp(dirs[i].name, name, len))
            {
                *res = dirs[i];
                return 0;
            }
        }
        next = fat_next_block(next);
    }
    return -1;
}

int fat_dir_insert(dir_entry* dir, dir_entry* new_file)
{
    dir_entry* dirs = (dir_entry*)block_buff;
    uint32_t next = dir->block_num;
    uint32_t prev = next;
    while(next != EOC)
    {
        device_read(block_buff, next, SECTOR_NUM);
        for(int i = 0; i < DIR_PER_BLOCK; i++)
        {
            if(dirs[i].block_num == 0)
            {
                dirs[i] = *new_file;
                device_write(block_buff, next, SECTOR_NUM);
                return 0;
            }
        }
        prev = next;
        next = fat_next_block(next);
    }

    next = fat_alloc_block();
    if(fat_cluster_insert(prev, next) != 0) return -1;

    memset(block_buff, 0, BLOCK_SIZE);
    memcpy(new_file, block_buff, sizeof(dir_entry));
    device_write(block_buff, next, SECTOR_NUM);
    return 0;
}


int fat_dir_update(dir_entry* fdir)
{
    dir_entry* dirs = (dir_entry*)block_buff;
    uint32_t next = fdir->dir_block;
    uint32_t prev = next;

    while(next != EOC)
    {
        device_read(block_buff, next, SECTOR_NUM);
        for(int i = 0; i < DIR_PER_BLOCK; i++)
        {
            if(dirs[i].block_num == fdir->block_num)
            {
                dirs[i] = *fdir;
                device_write(block_buff, next, SECTOR_NUM);
                return 0;
            }
        }
        prev = next;
        next = fat_next_block(next);
    }
    return -1;
}

int fat_dir_delete(dir_entry* fdir)
{
    uint32_t dir = fdir->dir_block;
    dir_entry* dirs = (dir_entry*)block_buff;
    uint32_t next = fdir->dir_block;
    uint32_t prev = next;
    while(next != EOC)
    {
        device_read(block_buff, next, SECTOR_NUM);
        for(int i = 0; i < DIR_PER_BLOCK; i++)
        {
            if(dirs[i].block_num == fdir->block_num)
            {
                memset(dirs + i, 0, sizeof(dir_entry));
                return 0;
            }
        }
        prev = next;
        next = fat_next_block(next);
    }
    return -1;
}

dir_entry fat_new_dir_entry(char* filename, uint32_t dir_block, uint8_t type)
{
    dir_entry new_file;
    uint32_t size = strlen(filename);
    size = size > MAX_FILE_NAME ? MAX_FILE_NAME : size;

    memset(&new_file, 0, sizeof(dir_entry));
    new_file.block_num = fat_alloc_block();
    new_file.name_length = size;
    new_file.file_size = 0;
    new_file.type = type;
    new_file.dir_block = dir_block;
    memcpy(filename, new_file.name, size);
    return new_file;
}

uint32_t fat_fread(file_desc* fd, uint8_t* buff, uint32_t size)
{
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

uint32_t fat_fseek(file_desc* fd, uint32_t offset, uint32_t position)
{
    uint32_t new_offset;
    switch(position)
    {
        case FAT_SEEK_SET:
            new_offset = offset;
            break;
        case FAT_SEEK_CUR:
            new_offset = fd->offset + offset;
            break;
        case FAT_SEEK_END:
            new_offset = fd->fdir.file_size + offset;
            break;
        default:
            return -1;
    }

    if(new_offset > fd->fdir.file_size) return -1;

    uint32_t block_num = new_offset / BLOCK_SIZE;
    uint32_t next_block = fd->fdir.block_num;
    for(int i = 0; i < block_num; i++)
    {
        next_block = fat_next_block(next_block);
    }
    fd->curr_block = next_block;
    fd->offset = new_offset;
    return new_offset;
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

        if(internal_offset != 0)
            device_read(block_buff, next_block, SECTOR_NUM);

        memcpy(buff, block_buff + internal_offset, bytes_to_write);
        device_write(block_buff, next_block, SECTOR_NUM);

        buff += bytes_to_write;
        size -= bytes_to_write;
        fd->offset += bytes_to_write;
        res += bytes_to_write;

        temp = fat_next_block(next_block);
        if(temp == EOC)
        {
            temp = fat_alloc_block();
            fat_cluster_insert(next_block, temp);
        }

        next_block = temp;
    }

    return res;
}




#ifdef FAT_DIR_SUPPORT

int fat_fopen(file_desc* fd, char* path, uint8_t mode)
{
    char* token = strtok(path, "/");
    dir_entry next = sb->root;
    dir_entry res = next;
    uint8_t flag = 0;
    while(token)
    {
        if(fat_dir_read(&next, &res, token) != 0)
        {
            flag = 1;
            break;
        }

        if(res.type != FAT_TYPE_DIR && strtok(NULL, "/") == NULL)
        {
            fd->fdir = res;
            fd->curr_block = res.block_num;
            fd->offset = res.file_size * (mode & FAT_MODE_APPEND);
            return 0;
        }

        next = res;
        token = strtok(NULL, "/");
    }

    if(strtok(NULL, "/") != NULL) return -1;

    if(flag && (mode & FAT_MODE_CREATE))
    {
        res = fat_new_dir_entry(token, next.block_num, FAT_TYPE_FILE);
        if(fat_dir_insert(&next, &res) != 0) return -1;
        fd->fdir = res;
        fd->curr_block = res.block_num;
        fd->offset = 0;
        return 0;
    }

    return -1;
}

int fat_fcreate(char* path, uint8_t type)
{
    char* token = strtok(path, "/");
    dir_entry next = sb->root;
    dir_entry res = next;
    uint8_t flag = 0;

    while(token)
    {
        if(fat_dir_read(&next, &res, token) != 0)
        {
            flag = 1;
            break;
        }

        token = strtok(NULL, token);
        next = res;
    }

    if(flag == 0)
    {
        return FAT_ERR_EXT;
    }

    if(strtok(NULL, token) && flag)
    {
        return FAT_ERR_PATH_ERR;
    }

    res = fat_new_dir_entry(token, next.dir_block, type);
    return 0;
}

#else

int fat_fopen(file_desc* fd, char* path, uint8_t mode)
{
    dir_entry next = sb->root;
    dir_entry res = next;


    if(fat_dir_read(&next, &res, path) == 0)
    {
        fd->fdir = res;
        fd->mode = mode;
        fd->offset = (FAT_MODE_APPEND & mode) >> 3 * res.file_size;
        fd->curr_block = res.block_num;
        return 0;
    }


    if(mode & FAT_MODE_CREATE)
    {
        res = fat_new_dir_entry(path, next.block_num, FAT_TYPE_FILE);
        fat_dir_insert(&next, &res);
        fd->fdir = res;
        fd->mode = mode;
        fd->offset = 0;
        fd->curr_block = res.block_num;
        return 0;
    }

    return -1;
}

#endif

int fat_fclose(file_desc* fd)
{
    if(fd->mode & FAT_MODE_WRITE)
    {
        if(fd->offset <= fd->fdir.file_size)
            return 0;
        if(fd->offset > fd->fdir.file_size)
            fd->fdir.file_size = fd->offset;
        return fat_dir_update(&fd->fdir);
    }

    return 0;
}

int fat_ftrunc(file_desc* fd, uint32_t size)
{
    uint32_t next = fd->fdir.block_num;
    uint32_t prev = next;

    for(int i = 0; i < size / BLOCK_SIZE; i++)
    {
        if(next == EOC)
        {
            next = fat_alloc_block();
            fat_cluster_insert(prev, next);
        }
        prev = next;
        next = fat_next_block(next);
    }

    if(fd->fdir.file_size > size)
    {
        while(next != EOC)
        {
            prev = next;
            next = fat_next_block(next);
            fat_free_block(prev);
        }
    }

    fd->fdir.file_size = size;
    fd->offset = size;

    return 0;
}

