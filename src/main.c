#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "include/http/request.h"
#include "include/http/response.h"
#include "include/server.h"

void index_handler(hcb_request_t *req, hcb_response_t *resp) {
  hcb_response_set_header(resp, "Content-Type", "text/html");
  hcb_body_set(resp, "Hello Index Handler!!!");
}

void health_get_handler(hcb_request_t *req, hcb_response_t *resp) {
  hcb_response_set_header(resp, "Content-Type", "text/html");
  hcb_body_set(resp, "Hello health GET Handler!!!");
}

void health_post_handler(hcb_request_t *req, hcb_response_t *resp) {
  hcb_response_set_header(resp, "Content-Type", "text/html");
  hcb_body_set(resp, "Hello health POST Handler!!!");
}

int main() {
  hcb_server_t *srv = new_hcb_server("8080");
  hcb_server_add_handler(srv, "GET", "/", index_handler);
  hcb_server_add_handler(srv, "GET", "/health", health_get_handler);
  hcb_server_add_handler(srv, "POST", "/health", health_post_handler);
  hcb_server_start(srv);
  free_hcb_server(srv);
  return 0;
}
