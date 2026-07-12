#ifndef HCB_REQUEST
#define HCB_REQUEST

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#define BUFFER 4096

#define METHOD_SIZE 16
#define ENDPOINT_SIZE 256
#define VERSION_SIZE 16

typedef struct hcb_request hcb_request_t;

char *hcb_request_get_endpoint(hcb_request_t *req);

hcb_request_t *new_hcb_request(int fd);

hcb_request_t *free_hcb_request(hcb_request_t *req);

#endif // !HCB_REQUEST
