#ifndef __PARSECFG_H__
#define __PARSECFG_H__

struct ConfigFile;
typedef struct ConfigFile ConfigFile;
ConfigFile *config_read(const char *path);
ConfigFile *config_free(ConfigFile *config);
const char *config_get(ConfigFile *config, const char *name);
int config_get_int(ConfigFile *config, const char *name, int *p_int);

#endif
