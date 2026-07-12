#include "request.h"

struct hcb_request {
  char *method;
  char *version;
  char *endpoint;
  char *raw_buffer;
};

struct hcb_request_header {
  char key[HEADER_KEY_SIZE];
  char value[HEADER_VALUE_SIZE];
};

static void hcb_request_extract_rows(hcb_request_t *req, int buffer_len) {
  char *rows[MAX_ROWS];
  int row_count = 0;

  rows[row_count++] = req->raw_buffer;

  for (int i = 0; i < buffer_len - 1; i++) {
    if (req->raw_buffer[i] == '\r' && req->raw_buffer[i + 1] == '\n') {
      req->raw_buffer[i] = '\0';
      if (row_count < MAX_ROWS) {
        rows[row_count++] = &req->raw_buffer[i + 2];
      }
      i++;
    }
  }

  req->method = rows[0];
  int j = 1;

  for (int i = 0; rows[0][i] != '\0'; i++) {
    if (rows[0][i] == ' ') {
      rows[0][i] = '\0';
      if (j == 1) {
        req->endpoint = &rows[0][i + 1];
        j++;
      } else if (j == 2) {
        req->version = &rows[0][i + 1];
        j++;
        break;
      }
    }
  }
}
char *hcb_request_get_endpoint(hcb_request_t *req) { return req->endpoint; }

hcb_request_t *new_hcb_request(int fd) {
  hcb_request_t *ret;
  ret = calloc(1, sizeof *ret);
  int size = BUFFER;
  char *buffer;
  buffer = malloc(BUFFER);
  ret->raw_buffer = buffer;
  int n = recv(fd, ret->raw_buffer, size, 0);
  if (n > size) {
    printf("recieving data more than buffer, handle taht later");
    return ret;
  }
  ret->raw_buffer[n - 1] = '\0';
  hcb_request_extract_rows(ret, n);
  return ret;
}

hcb_request_t *free_hcb_request(hcb_request_t *req) {
  free(req->raw_buffer);

  free(req);
  return req;
}
