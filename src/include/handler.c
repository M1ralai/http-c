#include "handler.h"

struct hcb_ihandler {
  char *method;
  char *endpoint;
  int (*handle_func)(hcb_request_t *req, hcb_response_t *resp);
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

static void hcb_send_response(hcb_response_t *resp, int fd) {
  char *ret = hcb_response_return(resp);
  send(fd, ret, strlen(ret), 0);
  free(ret);
}

void hcb_handler_exec(hcb_ihandler_t *handler, int fd, hcb_request_t *req) {
  hcb_response_t *resp = new_hcb_response();
  handler->handle_func(req, resp);
  hcb_send_response(resp, fd);
  free_hcb_response(resp);
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
