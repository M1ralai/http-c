#include "server.h"
#include "handler.h"

#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HANDLER_CAP 32

struct hcb_server {
  hcb_ihandler_t *handlers[HANDLER_CAP];
  char *port;
  int handlerIndex;
};

hcb_server_t *new_hcb_server(char *port) {
  hcb_server_t *ret;
  ret = malloc(sizeof *ret);
  ret->handlerIndex = 0;
  for (int i = 0; i < HANDLER_CAP; i++) {
    ret->handlers[i] = new_hcb_ihandler();
  }
  ret->port = port;
  return ret;
}

void hcb_server_add_handler(hcb_server_t *srv, char *method, char *endpoint,
                            void *func) {
  hcb_init_ihandler(srv->handlers[srv->handlerIndex], method, endpoint, func);
  srv->handlerIndex += 1;
}

static void hcb_default_404(int fd) {
  char *buf = "HTTP/1.1 404 Not Found\r\n"
              "Content-Length: 0\r\n"
              "Connection: close\r\n\r\n";
  send(fd, buf, strlen(buf), 0);
}

static void hcb_server_call_handler(hcb_server_t *srv, int fd,
                                    hcb_request_t *req) {
  for (int i = 0; i < HANDLER_CAP; i++) {
    if (!hcb_handler_endpoint_check(srv->handlers[i],
                                    hcb_request_get_endpoint(req))) {
      if (!hcb_handler_method_check(srv->handlers[i],
                                    hcb_request_get_method(req))) {
        hcb_handler_exec(srv->handlers[i], fd, req);
        return;
      }
    }
  }
  hcb_default_404(fd);
}

// just fucking basic parser for just getting endpoint for now
static void hcb_server_parser(hcb_server_t *srv, int ret_fd) {
  hcb_request_t *req = new_hcb_request(ret_fd);
  hcb_server_call_handler(srv, ret_fd, req);
  free_hcb_request(req);
  close(ret_fd);
}

void hcb_server_start(hcb_server_t *srv) {
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_PASSIVE;

  int gaierr = getaddrinfo(NULL, srv->port, &hints, &res);
  if (gaierr) {
    printf("getting addr info error: %s", gai_strerror(gaierr));
    return;
  }

  int socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (socket_fd == -1) {
    printf("socket error");
    return;
  }

  int yes = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
  int binderr = bind(socket_fd, res->ai_addr, res->ai_addrlen);
  if (binderr) {
    printf("binding error: %s", strerror(errno));
    return;
  }
  freeaddrinfo(res);

  listen(socket_fd, 5);
  while (1) {
    struct sockaddr conn_sock;
    int len = sizeof(conn_sock);
    int ret_fd = accept(socket_fd, &conn_sock, (socklen_t *)&len);
    if (ret_fd != -1) {
      hcb_server_parser(srv, ret_fd);
    } else {
      printf("accepted problematic connection request");
      return;
    }
  }
}

hcb_server_t *free_hcb_server(hcb_server_t *srv) {
  for (int i = 0; i < HANDLER_CAP; i++) {
    srv->handlers[i] = free_hcb_ihandler(srv->handlers[i]);
  }
  free(srv);
  return srv;
}
