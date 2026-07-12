#ifndef SERVER

#define SERVER

#include <stdlib.h>

#include "http/request.h"

typedef struct hcb_server hcb_server_t;

hcb_server_t *new_hcb_server(char *port);

void hcb_server_add_handler(hcb_server_t *srv, char *endpoint, void *func);

void hcb_server_start(hcb_server_t *srv);

hcb_server_t *free_hcb_server(hcb_server_t *srv);

#endif // ! SERVER
