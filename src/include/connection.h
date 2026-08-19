#ifndef HCB_CONNECTION
#define HCB_CONNECTION

#include <stdint.h>

#include "server.h"

#define HCB_CONNECTION_EVENT_READ (1u << 0)
#define HCB_CONNECTION_EVENT_WRITE (1u << 1)
#define HCB_CONNECTION_EVENT_ERROR (1u << 2)

typedef enum hcb_event_source_type {
  HCB_EVENT_SOURCE_LISTENER,
  HCB_EVENT_SOURCE_CONNECTION
} hcb_event_source_type_t;

typedef struct hcb_event_source {
  hcb_event_source_type_t type;
} hcb_event_source_t;

typedef struct hcb_connection hcb_connection_t;

hcb_connection_t *new_hcb_connection(int fd, int event_fd, hcb_server_t *srv);

int hcb_connection_register(hcb_connection_t *conn);

void hcb_connection_handle_events(hcb_connection_t *conn, uint32_t events);

uint32_t hcb_connection_interest(hcb_connection_t *conn);

int hcb_connection_fd(hcb_connection_t *conn);

int hcb_connection_is_closed(hcb_connection_t *conn);

void hcb_connection_close(hcb_connection_t *conn);

hcb_connection_t *free_hcb_connection(hcb_connection_t *conn);

#endif // !HCB_CONNECTION
