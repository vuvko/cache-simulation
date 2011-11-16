#include <stdio.h>
#include <stdlib.h>

#include "trace.h"

int
main(int argc, char *argv[])
{
    Trace *t = trace_open(argv[1], stderr);
    if (!t) {
        fprintf(stderr, "NOOOO\n");
        return 1;
    }
    fprintf(stderr, "%d\n", t);
    if (!trace_next(t)) {
        fprintf(stderr, "Noo2\n");
        return 1;
    }
    TraceStep *st = trace_get(t);
    fprintf(stderr, "%d %x\n", st->size, st->addr);
    
    return 0;
}
