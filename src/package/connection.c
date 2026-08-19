#include "connection.h"

#include "http/request.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/epoll.h>
#endif

#define HCB_CONNECTION_READ_BUFFER_SIZE 8192

typedef enum hcb_connection_state {
  HCB_CONN_READING,
  HCB_CONN_WRITING,
  HCB_CONN_CLOSED
} hcb_connection_state_t;

struct hcb_connection {
  hcb_event_source_t event_source;

  int fd;
  int event_fd;
  hcb_server_t *server;

  char read_buf[HCB_CONNECTION_READ_BUFFER_SIZE];
  size_t read_len;

  char *write_buf;
  size_t write_len;
  size_t write_offset;

  hcb_connection_state_t state;
  int closed;
};

static int hcb_connection_watch(hcb_connection_t *conn) {
#ifdef __linux__
  if (conn == NULL || conn->event_fd < 0 || conn->closed)
    return 0;

  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.data.ptr = conn;
  ev.events = EPOLLERR | EPOLLHUP;
#ifdef EPOLLRDHUP
  ev.events |= EPOLLRDHUP;
#endif

  if (conn->state == HCB_CONN_READING) {
    ev.events |= EPOLLIN;
  } else if (conn->state == HCB_CONN_WRITING) {
    ev.events |= EPOLLOUT;
  }

  return epoll_ctl(conn->event_fd, EPOLL_CTL_MOD, conn->fd, &ev);
#else
  (void)conn;
  return 0;
#endif
}

static ssize_t hcb_connection_send(hcb_connection_t *conn) {
#ifdef MSG_NOSIGNAL
  return send(conn->fd, conn->write_buf + conn->write_offset,
              conn->write_len - conn->write_offset, MSG_NOSIGNAL);
#else
  return send(conn->fd, conn->write_buf + conn->write_offset,
              conn->write_len - conn->write_offset, 0);
#endif
}

hcb_connection_t *new_hcb_connection(int fd, int event_fd, hcb_server_t *srv) {
  hcb_connection_t *conn = calloc(1, sizeof(*conn));
  if (conn == NULL)
    return NULL;

  conn->event_source.type = HCB_EVENT_SOURCE_CONNECTION;
  conn->fd = fd;
  conn->event_fd = event_fd;
  conn->server = srv;
  conn->state = HCB_CONN_READING;

  return conn;
}

int hcb_connection_register(hcb_connection_t *conn) {
#ifdef __linux__
  if (conn == NULL || conn->event_fd < 0)
    return -1;

  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
#ifdef EPOLLRDHUP
  ev.events |= EPOLLRDHUP;
#endif
  ev.data.ptr = conn;

  return epoll_ctl(conn->event_fd, EPOLL_CTL_ADD, conn->fd, &ev);
#else
  (void)conn;
  return 0;
#endif
}

static void hcb_connection_prepare_response(hcb_connection_t *conn) {
  hcb_request_t *req = hcb_request_parse(conn->read_buf, conn->read_len);
  conn->write_buf = hcb_server_handle_request(conn->server, req);
  free_hcb_request(req);

  if (conn->write_buf == NULL) {
    hcb_connection_close(conn);
    return;
  }

  conn->write_len = strlen(conn->write_buf);
  conn->write_offset = 0;
  conn->state = HCB_CONN_WRITING;

  if (hcb_connection_watch(conn) == -1) {
    perror("epoll mod connection");
    hcb_connection_close(conn);
  }
}

static void hcb_connection_read(hcb_connection_t *conn) {
  while (!conn->closed && conn->state == HCB_CONN_READING) {
    if (conn->read_len >= sizeof(conn->read_buf)) {
      hcb_connection_close(conn);
      return;
    }

    ssize_t n = recv(conn->fd, conn->read_buf + conn->read_len,
                     sizeof(conn->read_buf) - conn->read_len, 0);

    if (n > 0) {
      conn->read_len += (size_t)n;

      if (hcb_request_is_complete(conn->read_buf, conn->read_len)) {
        hcb_connection_prepare_response(conn);
        return;
      }

      continue;
    }

    if (n == 0) {
      hcb_connection_close(conn);
      return;
    }

    if (errno == EINTR)
      continue;

    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return;

    perror("recv");
    hcb_connection_close(conn);
    return;
  }
}

static void hcb_connection_write(hcb_connection_t *conn) {
  while (!conn->closed && conn->write_offset < conn->write_len) {
    ssize_t n = hcb_connection_send(conn);

    if (n > 0) {
      conn->write_offset += (size_t)n;
      continue;
    }

    if (n == 0)
      return;

    if (errno == EINTR)
      continue;

    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return;

    perror("send");
    hcb_connection_close(conn);
    return;
  }

  if (conn->write_offset >= conn->write_len) {
    hcb_connection_close(conn);
  }
}

void hcb_connection_handle_events(hcb_connection_t *conn, uint32_t events) {
  if (conn == NULL || conn->closed)
    return;

  if (events & HCB_CONNECTION_EVENT_ERROR) {
    hcb_connection_close(conn);
    return;
  }

  if ((events & HCB_CONNECTION_EVENT_READ) &&
      conn->state == HCB_CONN_READING) {
    hcb_connection_read(conn);
  }

  if ((events & HCB_CONNECTION_EVENT_WRITE) &&
      conn->state == HCB_CONN_WRITING) {
    hcb_connection_write(conn);
  }
}

uint32_t hcb_connection_interest(hcb_connection_t *conn) {
  if (conn == NULL || conn->closed)
    return 0;

  if (conn->state == HCB_CONN_READING)
    return HCB_CONNECTION_EVENT_READ;

  if (conn->state == HCB_CONN_WRITING)
    return HCB_CONNECTION_EVENT_WRITE;

  return 0;
}

int hcb_connection_fd(hcb_connection_t *conn) {
  if (conn == NULL)
    return -1;

  return conn->fd;
}

int hcb_connection_is_closed(hcb_connection_t *conn) {
  return conn == NULL || conn->closed;
}

void hcb_connection_close(hcb_connection_t *conn) {
  if (conn == NULL || conn->closed)
    return;

#ifdef __linux__
  if (conn->event_fd >= 0) {
    epoll_ctl(conn->event_fd, EPOLL_CTL_DEL, conn->fd, NULL);
  }
#endif

  close(conn->fd);
  conn->fd = -1;
  conn->state = HCB_CONN_CLOSED;
  conn->closed = 1;
}

hcb_connection_t *free_hcb_connection(hcb_connection_t *conn) {
  if (conn == NULL)
    return conn;

  hcb_connection_close(conn);
  free(conn->write_buf);
  free(conn);
  return conn;
}
