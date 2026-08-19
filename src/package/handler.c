#include "handler.h"

struct hcb_ihandler {
  char *method;
  char *endpoint;
  void (*handle_func)(hcb_request_t *req, hcb_response_t *resp);
};

hcb_ihandler_t *new_hcb_ihandler() {
  hcb_ihandler_t *ret;
  ret = malloc(sizeof *ret);
  ret->handle_func = NULL;
  ret->endpoint = "";
  ret->method = "";
  return ret;
}

void hcb_init_ihandler(hcb_ihandler_t *handler, char *method, char *endpoint,
                       void *func) {
  handler->method = method;
  handler->handle_func = func;
  handler->endpoint = endpoint;
}

void hcb_handler_exec(hcb_ihandler_t *handler, hcb_middleware_t *middleware,
                      hcb_request_t *req, hcb_response_t *resp) {
  if (handler == NULL || handler->handle_func == NULL || resp == NULL)
    return;

  hcb_middleware_exec(middleware, req, resp);
  handler->handle_func(req, resp);
}

int hcb_handler_endpoint_check(hcb_ihandler_t *handler, char *endpoint) {
  return strncmp(handler->endpoint, endpoint, 64);
}

int hcb_handler_method_check(hcb_ihandler_t *handler, char *method) {
  return strncmp(handler->method, method, 16);
}

hcb_ihandler_t *free_hcb_ihandler(hcb_ihandler_t *handler) {
  free(handler);
  return handler;
}
