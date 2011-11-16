#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "random.h"
#include "common.h"

struct Random
{
    RandomOps *ops;
    int seed;
};

Random *
random_free(Random *rnd)
{
    if (rnd != NULL) {
        free(rnd);
    }
    return NULL;
}

int
random_next(Random *rnd, int n)
{
    return (int)(rand() / (RAND_MAX + 1.0) * n);
}

static RandomOps rnd_ops = 
{
    random_free,
    random_next,
};

Random *
random_create(ConfigFile *cfg)
{
    if (!cfg) {
        return NULL;
    }
    Random *rnd = (Random *) calloc(1, sizeof(*rnd));
    rnd->ops = &rnd_ops;
    
    int err = config_get_int(cfg, "seed", &rnd->seed);
    if (!err) {
        rnd->seed = time(NULL);
    } else if (err < 0 || rnd->seed < 0) {
        char *text = strdup(config_get(cfg, "seed"));
        error_invalid("random_create", text);
        return NULL;
    }
    
    return rnd;
}
