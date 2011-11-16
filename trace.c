#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "trace.h"

struct Trace
{
    FILE *f;
    FILE *log_f;
    int lineno;
    char *path;
    TraceStep step;
};

Trace *
trace_open(const char *path, FILE *log_f)
{
    Trace *t = (Trace *) calloc(1, sizeof(*t));
    if (!log_f) {
        t->log_f = stderr;
    } else {
        t->log_f = log_f;
    }
    if (!path) {
        t->path = strdup("<stdin>");
        t->f = stdin;
    } else {
        t->path = strdup(path);
        t->f = fopen(path, "r");
    }
    if (!t->f) {
        error_open("trace_open", path);
        t = trace_close(t);
    }
    return t;
}

Trace *
trace_close(Trace *t)
{
    if (t != NULL) {
        if (t->f != NULL && t->f != stdin) {
            fclose(t->f);
        }
        free(t->path);
    }
    return NULL;
}

TraceStep *trace_get(Trace *t) {
    return &t->step;
}

void
trace_error(Trace *t, const char *text)
{
    fprintf(t->log_f, "%s: %d: trace_next: %s\n", t->path, t->lineno, text);
}

int
trace_next(Trace *t)
{
    int offset, size;
    long long value;
    int validsize[] = {1, 2, 4, 8};
    char *p, *str, op, mem;
    while((str = getline2(t->f)) != NULL) {
        fprintf(stderr, "%s\n+++\n", str);
        t->lineno++;
        p = strrchr(str, '#');
        if (p != NULL) {
            *p = '\0';
        }
        p = str;
        fprintf(stderr, "%s\n---\n", p);
        for (; isspace(*p); p++) {fprintf(stderr, "@\n");}
        fprintf(stderr, "%s\n---\n", p);
        if (*p == '\0' || *p == '\n') {
            continue;
        }
        sscanf(p, "%c%c%n", &op, &mem, &offset);
        if (op != 'R' && op != 'W') {
            fprintf(stderr, "'%c' - %d\n", op, op);
            trace_error(t, "invalid operation type");
            goto fail;
        }
        if (mem != 'D' && mem != 'I') {
            trace_error(t, "invalid memory type");
            goto fail;
        }
        t->step.op = op;
        t->step.mem = mem;
        p += offset;
        char *endp;
        errno = 0;
        t->step.addr = strtol(p, &endp, 16);
        if (errno) {
            trace_error(t, "invalid address");
            goto fail;
        }
        p = endp;
        if (p == '\0') {
            size = 1;
            value = 0;
        } else {
            size = strtol(p, &endp, 10);
            if (errno || !inarrayd(size, validsize, sizeof(validsize))) {
                trace_error(t, "invalid size");
                goto fail;
            }
            p = endp;
            value = strtoll(p, &endp, 10);
            if (errno) {
                trace_error(t, "invalid value");
                goto fail;
            }
        }
        t->step.size = size;
        int i;
        for (i = size - 1; i >= 0; i--) {
            t->step.value[i].flags = 1;
            t->step.value[i].value = value & BYTE;
            value >>= BYTE_SIZE;
        }
        free(str);
        return 1;
    }
    return 0;
fail:
    free(str);
    t = trace_close(t);
    return -1;
}
