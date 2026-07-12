#ifndef HCB_REQUEST
#define HCB_REQUEST

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#define BUFFER 4096

#define HEADER_KEY_SIZE 64
#define HEADER_VALUE_SIZE 256

#define MAX_ROWS 100
#define MAX_REQUEST_HEADERS 32

typedef struct hcb_request hcb_request_t;

typedef struct hcb_request_header hcb_request_header_t;

char *hcb_request_get_endpoint(hcb_request_t *req);

hcb_request_t *new_hcb_request(int fd);

hcb_request_t *free_hcb_request(hcb_request_t *req);

#endif // !HCB_REQUEST
