#include <stdio.h>
#include <stddef.h>
#include <stdint.h>


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

int memcmp(uint8_t* ptr1, uint8_t* ptr2, uint32_t count)
{
    for(int i = 0; i < count; i++)
    {
        if(ptr1[i] != ptr2[i]) return ptr1[i] - ptr2[i];
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

int main()
{
    char test[] = "Test Tested Tests Testing";
    char pattern[] = " ";
    char* token = strtok(test, pattern);
    
    while(token)
    {
        printf("%s\n", token);
        token = strtok(NULL, pattern);
    }

    char* token = strtok(test, pattern);
    
    while(token)
    {
        printf("%s\n", token);
        token = strtok(NULL, pattern);
    }
    
}