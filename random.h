#ifndef __RANDOM_H__
#define __RANDOM_H__

#include "parsecfg.h"

struct RandomOps;
typedef struct RandomOps RandomOps;

typedef struct Random
{
    RandomOps *ops;
    int seed;
} Random;

struct RandomOps
{
    Random *(*free)(Random *rnd);
    int (*next)(Random *rnd, int n);
};

Random *random_create(ConfigFile *cfg);

#endif
