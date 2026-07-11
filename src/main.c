#include <stdio.h>
#include <sys/socket.h>

#include "include/server.h"

void HandlerIndex(int fd) {
  int size_top = 150;
  char buf_top[150] = "HTTP/1.1 200 OK\r\nContent-Length-Type: text/plain; "
                      "charset=utf-8\r\nContent-Length: 13\r\nConnection: "
                      "close\r\n\r\nIndex Handler";
  send(fd, buf_top, size_top, 0);
}

int main() {
  hcb_server_t *srv = new_hcb_server("8080");
  hcb_server_add_handler(srv, "/", HandlerIndex);
  hcb_server_start(srv);
  free_hcb_server(srv);
  return 0;
}
