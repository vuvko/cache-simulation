#ifndef __PARSECFG_H__
#define __PARSECFG_H__

struct ConfigFile;
struct ConfigFile *config_read(const char *path);
struct ConfigFile *config_free(struct ConfigFile *config);
const char *config_get(struct ConfigFile *config, const char *name);
int config_get_int(struct ConfigFile *config, 
                   const char *name, 
                   int *p_int);
#endif
