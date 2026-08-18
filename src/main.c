#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "include/http/request.h"
#include "include/http/response.h"
#include "include/server.h"

void main_print_endpoint(hcb_request_t *req) {
  printf("request came to: %s endpoint \n", hcb_request_get_endpoint(req));
}

void index_handler(hcb_request_t *req, hcb_response_t *resp) {
  main_print_endpoint(req);
  hcb_response_set_header(resp, "Content-Type", "text/html");
  hcb_body_append(resp, "Hello Index Handler!!!");
}

void health_get_handler(hcb_request_t *req, hcb_response_t *resp) {
  main_print_endpoint(req);
  hcb_response_set_header(resp, "Content-Type", "text/html");
  hcb_body_append(resp, "Hello health GET Handler!!!");
}

void health_post_handler(hcb_request_t *req, hcb_response_t *resp) {
  main_print_endpoint(req);
  hcb_response_set_header(resp, "Content-Type", "text/html");
  hcb_body_append(resp, "Hello health POST Handler!!!");
}

void middleware_handler(hcb_request_t *req, hcb_response_t *resp) {
  main_print_endpoint(req);
  hcb_response_set_header(resp, "Content-Type", "text/html");
}

void middleware_one(hcb_request_t *req, hcb_response_t *resp) {
  main_print_endpoint(req);
  hcb_response_set_header(resp, "Content-Type", "text/html");
  hcb_body_append(resp, "Hello Middleware\r\n");
}

int main() {
  printf("program started \n");
  hcb_server_t *srv = new_hcb_server("8080");
  printf("Server created sucessfully \n");
  hcb_server_add_handler(srv, "GET", "/", index_handler);
  hcb_server_add_handler(srv, "GET", "/health", health_get_handler);
  hcb_server_add_handler(srv, "POST", "/health", health_post_handler);
  hcb_server_add_handler(srv, "GET", "/middleware", middleware_handler);
  hcb_server_register_middleware(srv, middleware_one);
  printf("server registers done, starting server");
  hcb_server_start(srv);
  free_hcb_server(srv);
  return 0;
}
