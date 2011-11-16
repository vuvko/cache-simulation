#ifndef __RANDOM_H__
#define __RANDOM_H__

#include "parsecfg.h"

struct Random;
typedef struct Random Random;

typedef struct RandomOps
{
    Random *(*free)(Random *rnd);
    int (*next)(Random *rnd, int n);
} RandomOps;

Random *random_create(ConfigFile *cfg);

#endif
