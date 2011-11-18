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
    BYTE_SIZE = 8
};

char *getline2(FILE *fin);
int inarrayd(int value, int *array, int size);
char *make_param_name(char *buf, int size, const char *prefix, const char *name);
void error_open(char *func, const char *file);
void error_undefined(const char *func, const char *param);
void error_invalid_chr(char *func, const char *file, int line, char chr);
void error_invalid(const char *func, const char *param);
#endif
