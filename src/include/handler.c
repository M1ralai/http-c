#include "handler.h"

struct hcb_ihandler {
  char *endpoint;
  int (*handle_func)(int fd);
};

hcb_ihandler_t *new_hcb_ihandler() {
  hcb_ihandler_t *ret;
  ret = malloc(sizeof *ret);
  ret->handle_func = NULL;
  ret->endpoint = "";
  return ret;
}

void hcb_init_ihandler(hcb_ihandler_t *handler, char *endpoint, void *func) {
  handler->handle_func = func;
  handler->endpoint = endpoint;
}

void hcb_handler_exec(hcb_ihandler_t *handler, int fd) {
  handler->handle_func(fd);
}

int hcb_handler_endpoint_check(hcb_ihandler_t *handler, char *endpoint) {
  return strncmp(handler->endpoint, endpoint, 64);
}

hcb_ihandler_t *free_hcb_ihandler(hcb_ihandler_t *handler) {
  free(handler);
  return handler;
}
