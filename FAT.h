#ifndef FAT_H
#define FAT_H

#include <stdint.h>


#define BLOCK_SIZE              512
#define SECTOR_NUM              1



#define FAT_MAGIC               0xFEFEFEFE
#define FAT_OFFSET              PART_START + 1
#define FILE_NAME_MAX           20
#define FAT_START               2
#define ADR_PER_BLOCK           (BLOCK_SIZE / 4)
#define DIR_PER_BLOCK           (BLOCK_SIZE / 32)
#define MAX_FILE_NAME           16
#define EOC                     0xFFFFFFFF

#define FAT_SEEK_SET            0
#define FAT_SEEK_CUR            1
#define FAT_SEEK_END            2

#define FAT_TYPE_FILE           1
#define FAT_TYPE_DIR            2
#define FAT_TYPE_EXEC           3

#define FAT_MODE_READ           (1 << 0)
#define FAT_MODE_WRITE          (1 << 1)
#define FAT_MODE_APPEND         (1 << 2)
#define FAT_MODE_CREATE         (1 << 3)
#define FAT_MODE_TRUNC          (1 << 4)

#define NULL_BLCK               0
#define FAT_ERR_NOT_EXT         -1
#define FAT_ERR_NO_SPC          -2
#define FAT_ERR_PATH_ERR        -3
#define FAT_ERR_EXT             -4


typedef struct
{
    uint32_t block_num;
    uint32_t dir_block;
    uint32_t file_size;
    uint8_t type;
    uint8_t perms;
    uint8_t owner_id;
    uint8_t name_length;
    char name[MAX_FILE_NAME];
} __attribute__((packed)) dir_entry;


typedef struct
{
    dir_entry fdir;
    uint32_t offset;
    uint32_t curr_block;
    uint8_t mode;
} file_desc;


typedef struct
{
    uint32_t fat_magic;
    uint32_t block_size;
    uint32_t total_block_num;
    uint32_t fat_table_size;
    uint32_t free_block_num;
    dir_entry root;
} __attribute__((packed)) super_block;


typedef struct
{
    super_block s_block;
    uint8_t block_buff[BLOCK_SIZE];
} fat_cb;


extern uint32_t (*device_read)(uint8_t* buff, uint32_t lba, uint32_t sectors);
extern uint32_t (*device_write)(uint8_t* buff, uint32_t lba, uint32_t sectors);

static super_block* sb;
static uint8_t* block_buff;
static uint32_t fat_offset;

int fat_mount(fat_cb* cb, uint32_t part_start);
int fat_create(fat_cb* cb, uint32_t part_start, uint32_t sector_num);
uint32_t fat_alloc_block();
void fat_free_block(uint32_t block_num);
uint32_t fat_next_block(uint32_t current_block);
uint32_t fat_cluster_insert(uint32_t current, uint32_t next);
int fat_dir_insert(dir_entry* dir, dir_entry* new_file);
int fat_dir_update(dir_entry* fdir);
int fat_dir_delete(dir_entry* fdir);
uint32_t fat_fread(file_desc* fd, uint8_t* buff, uint32_t size);
uint32_t fat_fseek(file_desc* fd, uint32_t offset, uint32_t position);
uint32_t fat_fwrite(file_desc* fd, uint8_t* buff, uint32_t size);
int fat_fcreate(char* path, uint8_t type);
int fat_fopen(file_desc* fd, char* path, uint8_t mode);
int fat_fclose(file_desc* fd);
int fat_ftrunc(file_desc* fd, uint32_t size);

#endif
