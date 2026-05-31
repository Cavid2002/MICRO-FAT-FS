
#ifdef _STRING_H
#else

#ifndef COMMON_H
#define COMMON_H


#include <stdint.h>
#include <stddef.h>

uint32_t memset(uint8_t* ptr, uint8_t c, uint32_t size);
uint32_t memcpy(uint8_t* src, uint8_t* dest, uint32_t count);
int memcmp(uint8_t* ptr1, uint8_t* ptr2, uint32_t count);
uint32_t strlen(uint8_t* ptr);
uint32_t strtok(char* str, char* pattern);

#endif

#endif

