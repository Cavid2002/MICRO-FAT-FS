#include "common.h"


uint32_t memset(void* ptr, uint8_t c, uint32_t size)
{
    while(--size)
    {
        ((uint8_t*)ptr)[size] = c;
    }
    return size;
}

uint32_t memcpy(void* src, void* dest, uint32_t count)
{
    uint32_t i;
    for(i = 0; i < count; i++)
    {
        ((uint8_t*)dest)[i] = ((uint8_t*)src)[i];
    }
    return 0;
}

int memcmp(void* ptr1, void* ptr2, uint32_t count)
{
    for(int i = 0; i < count; i++)
    {
        if(((uint8_t*)ptr1)[i] != ((uint8_t*)ptr2)[i]) return ((uint8_t*)ptr1)[i] - ((uint8_t*)ptr2)[i];
    }
    return 0;
}

uint32_t strlen(uint8_t* ptr)
{
    uint32_t len = 0;
    while(ptr[len])
    {
        len++;
    }
    return len;
}

char* strtok(char* str, char* pattern)
{
    static char* cur_str;
    if (str != NULL) cur_str = str;
    if (cur_str == NULL || *cur_str == '\0') return NULL;

    char* temp = cur_str;
    int len = strlen(cur_str);
    int pat_len = strlen(pattern);

    for (int i = 0; i < len; i++)
    {
        if (memcmp(cur_str + i, pattern, pat_len) == 0)
        {
            cur_str[i] = '\0';
            cur_str += i + pat_len;
            return temp;
        }
    }

    cur_str = cur_str + len;
    return temp;
}
