#include <stdlib.h>
#include <string.h>

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

int
arrayidx_str(char *value, char **array, int size)
{
    int i;
    for (i = 0; i < size; i++) {
        if (!strcmp(value, array[i])) {
            return i;
        }
    }
    return -1;
}

char *
make_param_name(char *buf, int size, const char *prefix, const char *name)
{
    if (!prefix) prefix = "";
    snprintf(buf, size, "%s%s", prefix, name);
    return buf;
}

void
error_open(char *func, const char *file)
{
    fprintf(stderr, "%s: Failed to open %s for reading\n", func, file);
    exit(1);
}

void
error_undefined(const char *func, const char *param)
{
    fprintf(stderr, "%s: configuration parameter '%s' is undefined\n", func, param);
    exit(1);
}

void
error_invalid(const char *func, const char *param)
{
    fprintf(stderr, "%s: configuration parameter '%s' value is invalid\n", func, param);
    exit(1);
}
