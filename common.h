#ifndef __COMMON_H__
#define __COMMON_H__

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
void error_open(char *func, const char *file);
void error_undefined(char *func, char *file, int line, char *param);
void error_invalid_chr(char *func, char *file, int line, char chr);
void error_invalid(char *func, char *file, int line, char *value);
#endif
