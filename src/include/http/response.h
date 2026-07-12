#ifndef HCB_RESPONSE
#define HCB_RESPONSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RESPONSE_HEADERS 32

typedef struct hcb_response hcb_response_t;

typedef struct hcb_response_header hcb_response_header_t;

void hcb_response_set_header(hcb_response_t *resp, char *key, char *value);

void hcb_body_set(hcb_response_t *resp, char *body);

char *hcb_response_return(hcb_response_t *resp);

hcb_response_t *new_hcb_response();

hcb_response_t *free_hcb_response(hcb_response_t *resp);

#endif // !HCB_RESPONSE
