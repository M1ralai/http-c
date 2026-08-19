#ifndef HCB_HASH_MAP

#define HCB_HASH_MAP

#include "http/request.h"
#include "http/response.h"

typedef struct hcb_hash_map hcb_hash_map_t;

typedef void (*data)(hcb_request_t *req, hcb_response_t *resp);

hcb_hash_map_t *new_hash_map();

void hcb_hash_map_add(hcb_hash_map_t *map, const char *str, data func);

data hcb_hash_map_get(hcb_hash_map_t *map, const char *str);

hcb_hash_map_t *free_hash_map(hcb_hash_map_t *hash_map);

#endif
