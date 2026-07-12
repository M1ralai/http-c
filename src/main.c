#include <stdio.h>
#include <sys/socket.h>

#include "include/server.h"

void index_handler(int fd) {
  int size = 150;
  char buf[150] = "HTTP/1.1 200 OK\r\nContent-Length-Type: text/plain; "
                  "charset=utf-8\r\nContent-Length: 13\r\nConnection: "
                  "close\r\n\r\nIndex Handler";
  send(fd, buf, size, 0);
}

void health_handler(int fd) {
  int size = 150;
  char buf[150] = "HTTP/1.1 200 OK\r\nContent-Length-Type: text/plain; "
                  "charset=utf-8\r\nContent-Length: 21\r\nConnection: "
                  "close\r\n\r\nEverything is healthy";
  send(fd, buf, size, 0);
}

int main() {
  hcb_server_t *srv = new_hcb_server("8080");
  hcb_server_add_handler(srv, "/", index_handler);
  hcb_server_add_handler(srv, "/health", health_handler);
  hcb_server_start(srv);
  free_hcb_server(srv);
  return 0;
}
