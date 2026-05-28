#include "common.h"


uint32_t memset(uint8_t* ptr, uint8_t c, uint32_t size)
{
    while(--size)
    {
        ptr[size] = c;
    }
    return size;
}

uint32_t memcpy(uint8_t* src, uint8_t* dest, uint32_t count)
{
    uint32_t i;
    for(i = 0; i < count; i++)
    {
        dest[i] = src[i];
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

