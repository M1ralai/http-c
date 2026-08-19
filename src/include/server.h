#ifndef HCB_SERVER

#define HCB_SERVER

#include <stdlib.h>

#include "http/request.h"
#include "http/response.h"

typedef struct hcb_server hcb_server_t;

typedef void (*hcb_handler_func_t)(hcb_request_t *req, hcb_response_t *resp);

hcb_server_t *new_hcb_server(char *port);

void hcb_server_register_middleware(hcb_server_t *server,
                                    void (*func)(hcb_request_t *req,
                                                 hcb_response_t *resp));

char *hcb_server_handle_request(hcb_server_t *srv, hcb_request_t *req);

void hcb_server_start(hcb_server_t *srv);

void hcb_server_get(hcb_server_t *srv, char *endpoint, hcb_handler_func_t func);
void hcb_server_post(hcb_server_t *srv, char *endpoint,
                     hcb_handler_func_t func);
void hcb_server_put(hcb_server_t *srv, char *endpoint, hcb_handler_func_t func);
void hcb_server_patch(hcb_server_t *srv, char *endpoint,
                      hcb_handler_func_t func);
void hcb_server_delete(hcb_server_t *srv, char *endpoint,
                       hcb_handler_func_t func);

hcb_server_t *free_hcb_server(hcb_server_t *srv);

#endif // ! SERVER
