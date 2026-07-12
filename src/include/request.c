#include "request.h"

struct hcb_request {
  char method[METHOD_SIZE];
  char version[VERSION_SIZE];
  char endpoint[ENDPOINT_SIZE];
};

// just for method version and endpoint for now
static void hcb_request_extract_row(hcb_request_t *req, char *buffer) {
  for (int i = 0, j = 0, k = 0; i < BUFFER; i++, k++) {
    if (buffer[i] != ' ') {
      switch (j) {
      case 0:
        req->method[k] = buffer[i];
        break;
      case 1:
        req->endpoint[k] = buffer[i];
        break;
      case 2:
        req->version[k] = buffer[i];
        break;
      default:
        break;
      }
    } else {
      k = 0;
      j += 1;
    }
  }
}

char *hcb_request_get_endpoint(hcb_request_t *req) { return req->endpoint; }

// Get filedescriptor and recv then map values to a struct
hcb_request_t *new_hcb_request(int fd) {
  hcb_request_t *ret;
  ret = calloc(1, sizeof *ret);
  int size = BUFFER;
  char buffer[BUFFER];
  int n = recv(fd, buffer, size, 0);
  if (n > size) {
    printf("recieving data more than buffer, handle taht later");
    return ret;
  }
  buffer[n] = '\0';
  hcb_request_extract_row(ret, buffer);
  return ret;
}

hcb_request_t *free_hcb_request(hcb_request_t *req) {
  free(req);
  return req;
}
