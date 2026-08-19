#ifndef HCB_MIDDLEWARE
#define HCB_MIDDLEWARE

#include "http/request.h"
#include "http/response.h"

typedef struct hcb_middleware hcb_middleware_t;

hcb_middleware_t *new_middleware(void (*func)(hcb_request_t *req,
                                              hcb_response_t *resp));

void hcb_middleware_exec(hcb_middleware_t *middleware, hcb_request_t *req,
                         hcb_response_t *resp);

void hcb_middleware_register(hcb_middleware_t *middleware,
                             void (*func)(hcb_request_t *req,
                                          hcb_response_t *resp));

hcb_middleware_t *free_middleware(hcb_middleware_t *middleware);
#endif // !HCB_MIDDLEWARE
