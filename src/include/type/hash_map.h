#ifndef HCB_HASH_MAP

#define HCB_HASH_MAP

#include "server.h"

typedef struct hcb_hash_map hcb_hash_map_t;

hcb_hash_map_t *new_hash_map();

void hcb_hash_map_add(hcb_hash_map_t *map, const char *str,
                      hcb_handler_func_t func);

hcb_handler_func_t hcb_hash_map_get(hcb_hash_map_t *map, const char *str);

hcb_hash_map_t *free_hash_map(hcb_hash_map_t *hash_map);

#endif
