#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "include/request.h"
#include "include/response.h"
#include "include/server.h"

// TODO change handlers as hcb_response_t *func(hcb_request_t *req,
// hcb_response_t *res);
void index_handler(hcb_request_t *req, hcb_response_t *resp) {
  hcb_response_set_header(resp, "Content-Type", "text/html");
  hcb_body_set(resp, "Hello Index Handler!!!");
}

void health_handler(hcb_request_t *req, hcb_response_t *resp) {
  hcb_response_set_header(resp, "Content-Type", "text/html");
  hcb_body_set(resp, "Hello Index Handler!!!");
}

int main() {
  hcb_server_t *srv = new_hcb_server("8080");
  hcb_server_add_handler(srv, "/", index_handler);
  hcb_server_add_handler(srv, "/health", health_handler);
  hcb_server_start(srv);
  free_hcb_server(srv);
  return 0;
}
