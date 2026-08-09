#include "http/response.h"

struct hcb_response {
  char *version; // HTTP/1.1 as default for now
  char *status;

  char *body;

  int header_len;
  hcb_response_header_t *headers[MAX_RESPONSE_HEADERS];
};

struct hcb_response_header {
  char *key;
  char *value;
};

static hcb_response_header_t *new_response_header_t(char *key, char *value) {
  hcb_response_header_t *ret;
  ret = malloc(sizeof *ret);
  ret->key = key;
  ret->value = value;
  return ret;
}
hcb_response_t *new_hcb_response() {
  hcb_response_t *ret = calloc(1, sizeof(hcb_response_t));
  ret->version = "HTTP/1.1";
  ret->status = "200 OK";
  ret->header_len = 0;
  return ret;
}
void hcb_response_set_header(hcb_response_t *resp, char *key, char *value) {
  for (int i = 0; i < resp->header_len; i++) {
    if (!strcmp(resp->headers[i]->key, key)) {
      resp->headers[i]->value = value;
      return;
    }
  }
  resp->headers[resp->header_len] = new_response_header_t(key, value);
  resp->header_len++;
}

void hcb_body_set(hcb_response_t *resp, char *body) { resp->body = body; }

void hcb_body_append(hcb_response_t *resp, char *body) {
  if (resp->body == NULL) {
    resp->body = body;
    return;
  }
  size_t len_body = strlen(resp->body);
  size_t len_parsing = strlen(body);

  char *new_body = malloc(len_body + len_parsing + 1);

  if (new_body == NULL) {
    exit(EXIT_FAILURE);
  }

  memcpy(new_body, resp->body, len_body);
  memcpy(new_body + len_body, body, len_parsing);

  new_body[len_body + len_parsing] = '\0';

  resp->body = new_body;
}

char *hcb_response_return(hcb_response_t *resp) {
  char cl_buffer[32];
  int body_len = resp->body ? strlen(resp->body) : 0;
  snprintf(cl_buffer, sizeof(cl_buffer), "%d", body_len);

  hcb_response_set_header(resp, "Content-Length", strdup(cl_buffer));

  size_t total_size = strlen(resp->version) + 1 + strlen(resp->status) +
                      2; // "HTTP/1.1 200 OK\r\n"

  for (int i = 0; i < resp->header_len; i++) {
    total_size += strlen(resp->headers[i]->key) + 2 +
                  strlen(resp->headers[i]->value) + 2; // "Key: Value\r\n"
  }

  total_size += 2 + body_len;
  total_size += 1;

  char *response_str = malloc(total_size);
  if (!response_str) {
    return NULL;
  }

  char *ptr = response_str;
  size_t remaining = total_size;
  int written;

  written = snprintf(ptr, remaining, "%s %s\r\n", resp->version, resp->status);
  ptr += written;
  remaining -= written;

  for (int i = 0; i < resp->header_len; i++) {
    written = snprintf(ptr, remaining, "%s: %s\r\n", resp->headers[i]->key,
                       resp->headers[i]->value);
    ptr += written;
    remaining -= written;
  }

  written = snprintf(ptr, remaining, "\r\n");
  ptr += written;
  remaining -= written;

  if (resp->body && body_len > 0) {
    snprintf(ptr, remaining, "%s", resp->body);
  }

  return response_str;
}

hcb_response_t *free_hcb_response(hcb_response_t *resp) {
  for (int i = 0; i < resp->header_len; i++) {
    free(resp->headers[i]);
  }
  free(resp);
  return resp;
}
