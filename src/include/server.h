#ifndef SERVER

#define SERVER

#include <stdlib.h>

#include "http/request.h"
#include "http/response.h"

typedef struct hcb_server hcb_server_t;

hcb_server_t *new_hcb_server(char *port);

void hcb_server_add_handler(hcb_server_t *srv, char *method, char *endpoint,
                            void *func);

void hcb_server_register_middleware(hcb_server_t *server,
                                    void (*func)(hcb_request_t *req,
                                                 hcb_response_t *resp));

char *hcb_server_handle_request(hcb_server_t *srv, hcb_request_t *req);

void hcb_server_start(hcb_server_t *srv);

hcb_server_t *free_hcb_server(hcb_server_t *srv);

#endif // ! SERVER
