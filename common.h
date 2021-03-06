#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>

typedef int memaddr_t;

typedef struct MemoryCell 
{
    unsigned char value;
    unsigned char flags;
} MemoryCell;
    
enum 
{
    MIN_STR_SIZE = 64,
    BYTE = 255,
    BYTE_SIZE = 8,
    KiB = 1024,
    MiB = 1024 * 1024,
    GiB = 1024 * 1024 * 1024
};

char *getline2(FILE *fin);
int inarrayd(int value, int *array, int size);
int arrayidx_str(char *value, char **array, int size);
char *make_param_name(
    char *buf, 
    int size, 
    const char *prefix, 
    const char *name);
void error_open(const char *file);
void error_undefined(const char *param);
void error_invalid(const char *param);
#endif
