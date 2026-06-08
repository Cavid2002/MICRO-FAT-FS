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

#define FAT_SEEK_SET                0
#define FAT_SEEK_CUR                1
#define FAT_SEEK_END                2

#define FAT_TYPE_FILE           1
#define FAT_TYPE_DIR            2
#define FAT_TYPE_EXEC           3

#define FAT_MODE_READ           (1 << 0)
#define FAT_MODE_WRITE          (1 << 1)
#define FAT_MODE_APPEND         (1 << 2)
#define FAT_MODE_CREATE         (1 << 3)

#define NULL_BLCK               0
#define FAT_ERR_NOT_EXT         -1
#define FAT_ERR_NO_SPC          -2
#define FAT_ERR_PATH_ERR        -3



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


uint32_t (*device_read)(uint8_t* buff, uint32_t lba, uint32_t sectors);
uint32_t (*device_write)(uint8_t* buff, uint32_t lba, uint32_t sectors);



#endif
