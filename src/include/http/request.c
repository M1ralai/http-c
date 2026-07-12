#include "request.h"

struct hcb_request {
  char *method;
  char *version;
  char *endpoint;
  char *raw_buffer;
  hcb_request_header_t *headers[MAX_REQUEST_HEADERS];
};

struct hcb_request_header {
  char *key;
  char *value;
};

static hcb_request_header_t *new_hcb_request_header(char *row) {
  hcb_request_header_t *ret = malloc(sizeof(*ret));
  ret->key = row;
  ret->value = NULL;
  for (int i = 0; row[i] != '\0'; i++) {
    if (row[i] == ':') {
      row[i] = '\0';

      int j = i + 1;
      while (row[j] == ' ') {
        j++;
      }
      ret->value = &row[j];
      while (row[j] != '\0') {
        if (row[j] == '\r' || row[j] == '\n') {
          row[j] = '\0';
          break;
        }
        j++;
      }
      break;
    }
  }
  return ret;
}

static void hcb_request_filler(hcb_request_t *req, char *first) {
  req->method = first;
  int j = 1;

  for (int i = 0; first[i] != '\0'; i++) {
    if (first[i] == ' ') {
      first[i] = '\0';
      if (j == 1) {
        req->endpoint = &first[i + 1];
        j++;
      } else if (j == 2) {
        req->version = &first[i + 1];
        j++;
        break;
      }
    }
  }
}

static void hcb_request_header_filler(hcb_request_t *req, char **rows,
                                      int row_count) {
  for (int i = 1; i < row_count; i++) {
    req->headers[i - 1] = new_hcb_request_header(rows[i]);
  }
}
static void hcb_request_extract_rows(hcb_request_t *req, int buffer_len) {
  char *rows[MAX_ROWS];
  int row_count = 0;
  rows[row_count++] = req->raw_buffer;
  for (int i = 0; i < buffer_len - 1; i++) {
    if (req->raw_buffer[i] == '\r' && req->raw_buffer[i + 1] == '\n') {
      req->raw_buffer[i] = '\0';
      if (i > 0 && req->raw_buffer[i - 1] == '\0') {
        break;
      }

      if (row_count < MAX_ROWS) {
        rows[row_count++] = &req->raw_buffer[i + 2];
      }
      i++;
    }
  }

  hcb_request_filler(req, rows[0]);
  hcb_request_header_filler(req, rows, row_count);
}

char *hcb_request_get_endpoint(hcb_request_t *req) { return req->endpoint; }

char *hcb_request_get_method(hcb_request_t *req) { return req->method; }

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
