#include "middleware.h"
#include "http/request.h"
#include "http/response.h"

struct hcb_middleware {
  void (*func)(hcb_request_t *req, hcb_response_t *resp);
  hcb_middleware_t *next;
};

hcb_middleware_t *new_middleware(void (*func)(hcb_request_t *req,
                                              hcb_response_t *resp)) {
  hcb_middleware_t *ret = malloc(sizeof(*ret));
  ret->func = func;
  ret->next = NULL;
  return ret;
}

static void hcb_middleware_set(hcb_middleware_t *middleware,
                               void (*func)(hcb_request_t *req,
                                            hcb_response_t *resp)) {
  middleware->next = new_middleware(func);
}

void hcb_middleware_exec(hcb_middleware_t *middleware, hcb_request_t *req,
                         hcb_response_t *resp) {
  for (hcb_middleware_t *curr = middleware; curr != NULL; curr = curr->next) {
    if (curr->func) {
      curr->func(req, resp);
    };
  }
}

void hcb_middleware_register(hcb_middleware_t *middleware,
                             void (*func)(hcb_request_t *req,
                                          hcb_response_t *resp)) {
  hcb_middleware_t *curr;
  for (curr = middleware; curr->next != NULL; curr = curr->next)
    ;
  if (curr->next == NULL) {
    hcb_middleware_set(middleware, func);
  }
}

hcb_middleware_t *free_middleware(hcb_middleware_t *middleware) {
  hcb_middleware_t *curr = middleware;

  while (curr != NULL) {
    hcb_middleware_t *next = curr->next;
    free(curr);
    curr = next;
  }

  return middleware;
}
