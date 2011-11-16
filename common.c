#include <stdio.h>
#include <stdlib.h>

#include "common.h"

char *
getline2(FILE *fin)
{
    char *str, c;
    unsigned int len = 0, size = MIN_STR_SIZE;
    str = (char *) calloc(size, sizeof(*str));
    while ((c = fgetc(fin)) != EOF && c != '\n') {
        str[len++] = c;
        if (len >= size) {
            str = (char *) realloc(str, (size *= 2) * sizeof(*str));
        }
    }
    str = (char *) realloc(str, (len + 2) * sizeof(*str));
    if (c == '\n') {
        str[len++] = '\n';
    }
    if (len > 0) {
        str[len] = '\0';
    } else {
        free(str);
        str = NULL;
    }
    
    return str;
}

int
inarrayd(int value, int *array, int size)
{
    int i;
    for (i = 0; i < size; i++) {
        if (value == array[i]) {
            return 1;
        }
    }
    return 0;
}

void
error_open(char *func, const char *file)
{
    fprintf(stderr, "%s: Cannot open file %s\n", func, file);
}

void
error_undefined(char *func, char *file, int line, char *param)
{
    fprintf(stderr, "%s, %s: on line %d - undefined \
    configuration parameter '%s'\n", func, file, line, param);
}

void
error_invalid(char *func, char *file, int line, char *value)
{
    fprintf(stderr, "%s, %s: on line %d - invalid \
    configuration value '%s'\n", func, file, line, value);
}

void
error_invalid_chr(char *func, char *file, int line, char chr)
{
    fprintf(stderr, "%s, %s:on line %d - invalid \
    character '%c'\n", func, file, line, chr);
}
