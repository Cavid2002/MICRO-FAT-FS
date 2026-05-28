#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

uint32_t memset(uint8_t* ptr, uint8_t c, uint32_t size);
uint32_t memcpy(uint8_t* src, uint8_t* dest, uint32_t count);
uint32_t strlen(uint8_t* ptr);

#endif