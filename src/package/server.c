#include "server.h"
#include "middleware.h"
#include "type/hash_map.h"

#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct hcb_server {
  hcb_hash_map_t *handlers;
  hcb_middleware_t *middleware;
  char *port;
};

void null_func(hcb_request_t *req, hcb_response_t *resp) {
  (void)req;
  (void)resp;
};

static char *hcb_server_route_key(char *method, char *endpoint) {
  size_t method_len = strlen(method);
  size_t endpoint_len = strlen(endpoint);
  char *key = malloc(method_len + endpoint_len + 2);

  if (key == NULL)
    return NULL;

  snprintf(key, method_len + endpoint_len + 2, "%s %s", method, endpoint);
  return key;
}

hcb_server_t *new_hcb_server(char *port) {
  hcb_server_t *ret;
  ret = malloc(sizeof *ret);
  ret->handlers = new_hash_map();
  printf("Server created beside middleware \n");
  ret->middleware = new_middleware(null_func);
  printf("New middleware created for server \n");
  ret->port = port;
  return ret;
}

void hcb_server_add_handler(hcb_server_t *srv, char *method, char *endpoint,
                            void *func) {
  char *key = hcb_server_route_key(method, endpoint);

  if (key == NULL)
    return;

  hcb_hash_map_add(srv->handlers, key, (data)func);
  free(key);
}

static void hcb_default_404(int fd) {
  char *buf = "HTTP/1.1 404 Not Found\r\n"
              "Content-Length: 0\r\n"
              "Connection: close\r\n\r\n";
  send(fd, buf, strlen(buf), 0);
}

static void hcb_server_call_handler(hcb_server_t *srv, int fd,
                                    hcb_request_t *req) {
  char *key = hcb_server_route_key(hcb_request_get_method(req),
                                   hcb_request_get_endpoint(req));

  if (key == NULL) {
    hcb_default_404(fd);
    return;
  }

  data handler = hcb_hash_map_get(srv->handlers, key);
  free(key);

  if (handler == NULL) {
    hcb_default_404(fd);
    return;
  }

  hcb_response_t *resp = new_hcb_response();
  hcb_middleware_exec(srv->middleware, req, resp);
  handler(req, resp);

  char *buf = hcb_response_return(resp);
  send(fd, buf, strlen(buf), 0);
  free(buf);
  free_hcb_response(resp);
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

void hcb_server_register_middleware(hcb_server_t *server,
                                    void (*func)(hcb_request_t *req,
                                                 hcb_response_t *resp)) {
  hcb_middleware_register(server->middleware, func);
}

hcb_server_t *free_hcb_server(hcb_server_t *srv) {
  free_hash_map(srv->handlers);
  free(srv);
  return srv;
}
