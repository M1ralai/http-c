#include "server.h"
#include "connection.h"
#include "middleware.h"
#include "type/hash_map.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/epoll.h>
#else
#include <poll.h>
#endif

#define HCB_SERVER_BACKLOG 128
#define HCB_SERVER_MAX_EVENTS 64

typedef struct hcb_connection_node {
  hcb_connection_t *conn;
  struct hcb_connection_node *next;
} hcb_connection_node_t;

struct hcb_server {
  hcb_hash_map_t *handlers;
  hcb_middleware_t *middleware;
  hcb_connection_node_t *connections;
  size_t connection_count;
  char *port;
};

typedef struct hcb_listener_event {
  hcb_event_source_t event_source;
  int fd;
} hcb_listener_event_t;

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
  ret = calloc(1, sizeof *ret);
  if (ret == NULL)
    return NULL;

  ret->handlers = new_hash_map();
  ret->middleware = new_middleware(null_func);
  ret->port = port;
  return ret;
}

static void hcb_server_add_handler(hcb_server_t *srv, char *method,
                                   char *endpoint, hcb_handler_func_t func) {
  char *key = hcb_server_route_key(method, endpoint);

  if (key == NULL)
    return;

  hcb_hash_map_add(srv->handlers, key, func);
  free(key);
}

void hcb_server_get(hcb_server_t *srv, char *endpoint,
                    hcb_handler_func_t func) {
  hcb_server_add_handler(srv, "GET", endpoint, func);
}

void hcb_server_post(hcb_server_t *srv, char *endpoint,
                     hcb_handler_func_t func) {
  hcb_server_add_handler(srv, "POST", endpoint, func);
}

void hcb_server_patch(hcb_server_t *srv, char *endpoint,
                      hcb_handler_func_t func) {
  hcb_server_add_handler(srv, "PATCH", endpoint, func);
}

void hcb_server_put(hcb_server_t *srv, char *endpoint,
                    hcb_handler_func_t func) {
  hcb_server_add_handler(srv, "PUT", endpoint, func);
}

void hcb_server_delete(hcb_server_t *srv, char *endpoint,
                       hcb_handler_func_t func) {
  hcb_server_add_handler(srv, "DELETE", endpoint, func);
}

void hcb_server_connect(hcb_server_t *srv, char *endpoint,
                        hcb_handler_func_t func) {
  hcb_server_add_handler(srv, "CONNECT", endpoint, func);
}

void hcb_server_options(hcb_server_t *srv, char *endpoint,
                        hcb_handler_func_t func) {
  hcb_server_add_handler(srv, "OPTIONS", endpoint, func);
}

void hcb_server_trace(hcb_server_t *srv, char *endpoint,
                      hcb_handler_func_t func) {
  hcb_server_add_handler(srv, "TRACE", endpoint, func);
}

void hcb_server_head(hcb_server_t *srv, char *endpoint,
                     hcb_handler_func_t func) {
  hcb_server_add_handler(srv, "HEAD", endpoint, func);
}

static char *hcb_server_error_response(char *status) {
  hcb_response_t *resp = new_hcb_response();
  if (resp == NULL)
    return NULL;

  hcb_response_set_status(resp, status);
  hcb_response_set_header(resp, "Connection", "close");

  char *buf = hcb_response_return(resp);
  free_hcb_response(resp);
  return buf;
}

char *hcb_server_handle_request(hcb_server_t *srv, hcb_request_t *req) {
  if (srv == NULL || req == NULL || hcb_request_get_method(req) == NULL ||
      hcb_request_get_endpoint(req) == NULL) {
    return hcb_server_error_response("400 Bad Request");
  }

  char *key = hcb_server_route_key(hcb_request_get_method(req),
                                   hcb_request_get_endpoint(req));

  if (key == NULL) {
    return hcb_server_error_response("500 Internal Server Error");
  }

  hcb_handler_func_t handler = hcb_hash_map_get(srv->handlers, key);
  free(key);

  if (handler == NULL) {
    return hcb_server_error_response("404 Not Found");
  }

  hcb_response_t *resp = new_hcb_response();
  if (resp == NULL)
    return NULL;

  hcb_middleware_exec(srv->middleware, req, resp);
  handler(req, resp);
  hcb_response_set_header(resp, "Connection", "close");

  char *buf = hcb_response_return(resp);
  free_hcb_response(resp);
  return buf;
}

static int hcb_server_set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return -1;

  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int hcb_server_track_connection(hcb_server_t *srv,
                                       hcb_connection_t *conn) {
  hcb_connection_node_t *node = malloc(sizeof(*node));
  if (node == NULL)
    return -1;

  node->conn = conn;
  node->next = srv->connections;
  srv->connections = node;
  srv->connection_count++;
  return 0;
}

static void hcb_server_remove_connection(hcb_server_t *srv,
                                         hcb_connection_t *conn) {
  hcb_connection_node_t **curr = &srv->connections;

  while (*curr != NULL) {
    if ((*curr)->conn == conn) {
      hcb_connection_node_t *remove = *curr;
      *curr = remove->next;
      srv->connection_count--;
      free_hcb_connection(remove->conn);
      free(remove);
      return;
    }

    curr = &(*curr)->next;
  }
}

static void hcb_server_cleanup_closed_connections(hcb_server_t *srv) {
  hcb_connection_node_t *curr = srv->connections;

  while (curr != NULL) {
    hcb_connection_t *conn = curr->conn;
    curr = curr->next;

    if (hcb_connection_is_closed(conn)) {
      hcb_server_remove_connection(srv, conn);
    }
  }
}

static void hcb_server_accept_connections(hcb_server_t *srv, int socket_fd,
                                          int event_fd) {
  while (1) {
    struct sockaddr_storage conn_sock;
    socklen_t len = sizeof(conn_sock);
    int ret_fd = accept(socket_fd, (struct sockaddr *)&conn_sock, &len);

    if (ret_fd == -1) {
      if (errno == EINTR)
        continue;

      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;

      perror("accept");
      return;
    }

    if (hcb_server_set_nonblocking(ret_fd) == -1) {
      perror("client nonblocking");
      close(ret_fd);
      continue;
    }

    hcb_connection_t *conn = new_hcb_connection(ret_fd, event_fd, srv);
    if (conn == NULL) {
      close(ret_fd);
      continue;
    }

    if (hcb_server_track_connection(srv, conn) == -1) {
      free_hcb_connection(conn);
      continue;
    }

    if (hcb_connection_register(conn) == -1) {
      perror("register connection");
      hcb_server_remove_connection(srv, conn);
      continue;
    }
  }
}

#ifdef __linux__
static uint32_t hcb_server_epoll_events(uint32_t events) {
  uint32_t ret = 0;

  if (events & EPOLLIN)
    ret |= HCB_CONNECTION_EVENT_READ;

  if (events & EPOLLOUT)
    ret |= HCB_CONNECTION_EVENT_WRITE;

  if (events & (EPOLLERR | EPOLLHUP))
    ret |= HCB_CONNECTION_EVENT_ERROR;

#ifdef EPOLLRDHUP
  if (events & EPOLLRDHUP)
    ret |= HCB_CONNECTION_EVENT_ERROR;
#endif

  return ret;
}

static void hcb_server_epoll_loop(hcb_server_t *srv, int socket_fd) {
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_create1");
    return;
  }

  hcb_listener_event_t listener = {
      .event_source = {.type = HCB_EVENT_SOURCE_LISTENER},
      .fd = socket_fd,
  };

  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
  ev.data.ptr = &listener;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1) {
    perror("epoll add listener");
    close(epoll_fd);
    return;
  }

  struct epoll_event events[HCB_SERVER_MAX_EVENTS];

  while (1) {
    int n = epoll_wait(epoll_fd, events, HCB_SERVER_MAX_EVENTS, -1);

    if (n == -1) {
      if (errno == EINTR)
        continue;

      perror("epoll_wait");
      break;
    }

    for (int i = 0; i < n; i++) {
      hcb_event_source_t *source = events[i].data.ptr;
      if (source == NULL)
        continue;

      if (source->type == HCB_EVENT_SOURCE_LISTENER) {
        hcb_server_accept_connections(srv, socket_fd, epoll_fd);
        continue;
      }

      hcb_connection_t *conn = (hcb_connection_t *)source;
      hcb_connection_handle_events(conn,
                                   hcb_server_epoll_events(events[i].events));

      if (hcb_connection_is_closed(conn)) {
        hcb_server_remove_connection(srv, conn);
      }
    }
  }

  close(epoll_fd);
}
#else
static uint32_t hcb_server_poll_revents(short revents) {
  uint32_t ret = 0;

  if (revents & POLLIN)
    ret |= HCB_CONNECTION_EVENT_READ;

  if (revents & POLLOUT)
    ret |= HCB_CONNECTION_EVENT_WRITE;

  if (revents & (POLLERR | POLLHUP | POLLNVAL))
    ret |= HCB_CONNECTION_EVENT_ERROR;

  return ret;
}

static short hcb_server_poll_interest(hcb_connection_t *conn) {
  uint32_t interest = hcb_connection_interest(conn);
  short ret = 0;

  if (interest & HCB_CONNECTION_EVENT_READ)
    ret |= POLLIN;

  if (interest & HCB_CONNECTION_EVENT_WRITE)
    ret |= POLLOUT;

  return ret;
}

static void hcb_server_poll_loop(hcb_server_t *srv, int socket_fd) {
  while (1) {
    size_t max_fds = srv->connection_count + 1;
    struct pollfd *fds = calloc(max_fds, sizeof(*fds));
    hcb_connection_t **connections = calloc(max_fds, sizeof(*connections));

    if (fds == NULL || connections == NULL) {
      free(fds);
      free(connections);
      perror("poll alloc");
      return;
    }

    size_t nfds = 1;
    fds[0].fd = socket_fd;
    fds[0].events = POLLIN;

    for (hcb_connection_node_t *curr = srv->connections; curr != NULL;
         curr = curr->next) {
      if (hcb_connection_is_closed(curr->conn))
        continue;

      fds[nfds].fd = hcb_connection_fd(curr->conn);
      fds[nfds].events = hcb_server_poll_interest(curr->conn);
      connections[nfds] = curr->conn;
      nfds++;
    }

    int n = poll(fds, (nfds_t)nfds, -1);

    if (n == -1) {
      free(fds);
      free(connections);

      if (errno == EINTR)
        continue;

      perror("poll");
      return;
    }

    if (fds[0].revents & POLLIN) {
      hcb_server_accept_connections(srv, socket_fd, -1);
    }

    for (size_t i = 1; i < nfds; i++) {
      uint32_t events = hcb_server_poll_revents(fds[i].revents);

      if (events != 0) {
        hcb_connection_handle_events(connections[i], events);
      }
    }

    free(fds);
    free(connections);
    hcb_server_cleanup_closed_connections(srv);
  }
}
#endif

void hcb_server_start(hcb_server_t *srv) {
  if (srv == NULL)
    return;

  signal(SIGPIPE, SIG_IGN);

  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_INET;
  hints.ai_flags = AI_PASSIVE;

  int gaierr = getaddrinfo(NULL, srv->port, &hints, &res);
  if (gaierr) {
    printf("getting addr info error: %s", gai_strerror(gaierr));
    return;
  }

  int socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (socket_fd == -1) {
    perror("socket error");
    freeaddrinfo(res);
    return;
  }

  int yes = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
  int binderr = bind(socket_fd, res->ai_addr, res->ai_addrlen);
  if (binderr) {
    printf("binding error: %s", strerror(errno));
    close(socket_fd);
    freeaddrinfo(res);
    return;
  }
  freeaddrinfo(res);

  if (hcb_server_set_nonblocking(socket_fd) == -1) {
    perror("listener nonblocking");
    close(socket_fd);
    return;
  }

  if (listen(socket_fd, HCB_SERVER_BACKLOG) == -1) {
    perror("listen");
    close(socket_fd);
    return;
  }

#ifdef __linux__
  hcb_server_epoll_loop(srv, socket_fd);
#else
  hcb_server_poll_loop(srv, socket_fd);
#endif

  close(socket_fd);
}

void hcb_server_register_middleware(hcb_server_t *server,
                                    void (*func)(hcb_request_t *req,
                                                 hcb_response_t *resp)) {
  hcb_middleware_register(server->middleware, func);
}

hcb_server_t *free_hcb_server(hcb_server_t *srv) {
  if (srv == NULL)
    return srv;

  while (srv->connections != NULL) {
    hcb_connection_node_t *node = srv->connections;
    srv->connections = node->next;
    free_hcb_connection(node->conn);
    free(node);
  }

  free_hash_map(srv->handlers);
  free_middleware(srv->middleware);
  free(srv);
  return srv;
}
