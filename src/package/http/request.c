#include "http/request.h"

#include <string.h>

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
  if (ret == NULL)
    return NULL;

  ret->key = row;
  ret->value = "";

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
  for (int i = 1; i < row_count && i <= MAX_REQUEST_HEADERS; i++) {
    req->headers[i - 1] = new_hcb_request_header(rows[i]);
  }
}

static void hcb_request_extract_rows(hcb_request_t *req, size_t buffer_len) {
  char *rows[MAX_ROWS];
  int row_count = 0;

  char *line = req->raw_buffer;
  char *end = req->raw_buffer + buffer_len;

  for (char *curr = req->raw_buffer; curr + 1 < end; curr++) {
    if (curr[0] == '\r' && curr[1] == '\n') {
      curr[0] = '\0';
      curr[1] = '\0';

      if (line[0] == '\0') {
        break;
      }

      if (row_count < MAX_ROWS) {
        rows[row_count++] = line;
      }

      line = curr + 2;
      curr++;
    }
  }

  if (row_count == 0)
    return;

  hcb_request_filler(req, rows[0]);
  hcb_request_header_filler(req, rows, row_count);
}

char *hcb_request_get_endpoint(hcb_request_t *req) { return req->endpoint; }

char *hcb_request_get_method(hcb_request_t *req) { return req->method; }

char *hcb_request_get_header(hcb_request_t *req, char *key) {
  if (req == NULL || key == NULL)
    return NULL;

  for (int i = 0; i < MAX_REQUEST_HEADERS && req->headers[i] != NULL; i++) {
    if (!strcmp(req->headers[i]->key, key)) {
      return req->headers[i]->value;
    }
  }
  return NULL;
}

int hcb_request_is_complete(const char *buffer, size_t buffer_len) {
  if (buffer == NULL || buffer_len < 4)
    return 0;

  for (size_t i = 0; i + 3 < buffer_len; i++) {
    if (buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' &&
        buffer[i + 3] == '\n') {
      return 1;
    }
  }

  return 0;
}

hcb_request_t *hcb_request_parse(const char *buffer, size_t buffer_len) {
  if (buffer == NULL || buffer_len == 0)
    return NULL;

  hcb_request_t *ret = calloc(1, sizeof *ret);
  if (ret == NULL)
    return NULL;

  ret->raw_buffer = malloc(buffer_len + 1);
  if (ret->raw_buffer == NULL) {
    free(ret);
    return NULL;
  }

  memcpy(ret->raw_buffer, buffer, buffer_len);
  ret->raw_buffer[buffer_len] = '\0';
  hcb_request_extract_rows(ret, buffer_len);
  return ret;
}

hcb_request_t *free_hcb_request(hcb_request_t *req) {
  if (req == NULL)
    return req;

  for (int i = 0; i < MAX_REQUEST_HEADERS; i++) {
    free(req->headers[i]);
  }

  free(req->raw_buffer);

  free(req);
  return req;
}
