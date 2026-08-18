#include "type/hash_map.h"

#define BUCKET_SIZE 32
#define BUCKET_COUNT 32

typedef struct hcb_pair {
  data func;
  char *key;
} hcb_pair_t;

typedef struct hcb_bucket {
  hcb_pair_t pairs[BUCKET_SIZE];
} hcb_bucket_t;

struct hcb_hash_map {
  hcb_bucket_t buckets[BUCKET_COUNT];
};

static unsigned long hcb_hash_string(const char *str) {
  unsigned long hash = 5381;

  while (*str) {
    hash = hash * 33 + (unsigned char)*str;
    str++;
  }

  return hash;
}

hcb_hash_map_t *new_hash_map() {
  hcb_hash_map_t *ret;
  ret = calloc(1, sizeof *ret);
  return ret;
}

void hcb_hash_map_add(hcb_hash_map_t *map, const char *str, data func) {
  size_t bucket_index = hcb_hash_string(str) % BUCKET_COUNT;

  hcb_bucket_t *bucket = &map->buckets[bucket_index];

  for (size_t i = 0; i < BUCKET_SIZE; i++) {
    hcb_pair_t *pair = &bucket->pairs[i];

    // Key zaten varsa güncelle
    if (pair->key != NULL && strcmp(pair->key, str) == 0) {
      pair->func = func;
      return;
    }

    // Boş slot
    if (pair->key == NULL) {
      pair->key = strdup(str);

      if (pair->key == NULL)
        return;

      pair->func = func;
      return;
    }
  }
}

data hcb_hash_map_get(hcb_hash_map_t *map, const char *str) {
  size_t bucket_index = hcb_hash_string(str) % BUCKET_COUNT;

  hcb_bucket_t *bucket = &map->buckets[bucket_index];

  for (size_t i = 0; i < BUCKET_SIZE; i++) {
    hcb_pair_t *pair = &bucket->pairs[i];

    if (pair->key == NULL)
      continue;

    if (strcmp(pair->key, str) == 0)
      return pair->func;
  }

  return NULL;
}

hcb_hash_map_t *free_hash_map(hcb_hash_map_t *map) {
  if (map == NULL)
    return map;

  for (size_t i = 0; i < BUCKET_COUNT; i++) {
    for (size_t j = 0; j < BUCKET_SIZE; j++) {
      free(map->buckets[i].pairs[j].key);
    }
  }

  free(map);
  return map;
}
