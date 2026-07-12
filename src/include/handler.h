#ifndef HANDLER
#define HANDLER

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "request.h"
#include "response.h"

typedef struct hcb_ihandler hcb_ihandler_t;
hcb_ihandler_t *new_hcb_ihandler();

void hcb_init_ihandler(hcb_ihandler_t *handler, char *endpoint, void *func);

int hcb_handler_endpoint_check(hcb_ihandler_t *handler, char *endpoint);

void hcb_handler_exec(hcb_ihandler_t *handler, int fd, hcb_request_t *req);

hcb_ihandler_t *free_hcb_ihandler(hcb_ihandler_t *handler);

#endif // !HANDLER
