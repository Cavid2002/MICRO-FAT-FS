
#ifdef _STRING_H
#else

#ifndef COMMON_H
#define COMMON_H


#include <stdint.h>
#include <stddef.h>

uint32_t memset(void* ptr, uint8_t c, uint32_t size);
uint32_t memcpy(void* src, void* dest, uint32_t count);
int memcmp(void* ptr1, void* ptr2, uint32_t count);
uint32_t strlen(uint8_t* ptr);
char* strtok(char* str, char* pattern);

#endif

#endif

