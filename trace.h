#ifndef __TRACE_H__
#define __TRACE_H__

#include <stdio.h>
#include "common.h"

struct Trace;
typedef struct Trace Trace;

typedef struct TraceStep
{
    char op;
    char mem;
    memaddr_t addr;
    int size;
    MemoryCell value[8];
} TraceStep;

Trace *trace_open(const char *path, FILE *log_f);
Trace *trace_close(Trace *t);
TraceStep *trace_get(Trace *t);
int trace_next(Trace *t);

#endif
