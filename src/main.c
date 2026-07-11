#include <stdio.h>
#include <sys/socket.h>

#include "include/server.h"

void HandlerIndex(int fd) {
  int size = 32;
  char buf[32] = "Index Handler";
  send(fd, buf, size, 0);
}

int main() {
  hcb_server_t *srv = new_hcb_server("8080");
  hcb_server_add_handler(srv, "/", HandlerIndex);
  hcb_server_start(srv);
  free_hcb_server(srv);
  return 0;
}
