#ifndef HCB_HANDLER
#define HCB_HANDLER

#include <stdlib.h>
#include <string.h>

#include "http/request.h"
#include "http/response.h"
#include "middleware.h"

typedef struct hcb_ihandler hcb_ihandler_t;

hcb_ihandler_t *new_hcb_ihandler();

typedef void (*hcb_handler_func_t)(hcb_request_t *req, hcb_response_t *resp);

void hcb_init_ihandler(hcb_ihandler_t *handler, char *method, char *endpoint,
                       void *func);

int hcb_handler_endpoint_check(hcb_ihandler_t *handler, char *endpoint);

int hcb_handler_method_check(hcb_ihandler_t *handler, char *method);

void hcb_handler_exec(hcb_ihandler_t *handler, hcb_middleware_t *middleware,
                      hcb_request_t *req, hcb_response_t *resp);

hcb_ihandler_t *free_hcb_ihandler(hcb_ihandler_t *handler);

#endif // !HANDLER
