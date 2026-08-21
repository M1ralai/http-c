# HTTP Server in C

An experimental HTTP/1.1 server written from scratch in C. The project explores socket programming, incremental connection state, request parsing, routing, middleware, and response serialization without an application framework.

## What this project demonstrates

- Non-blocking TCP sockets with an `epoll` event loop on Linux and a `poll` fallback
- Incremental read/write handling for multiple connections
- Parsing of the request line and headers into a request abstraction
- Method-and-path routing through a hash-map-backed handler registry
- Ordered middleware execution before the selected route handler
- Construction and serialization of status lines, headers, and response bodies

## Architecture and request lifecycle

```text
socket -> accept -> request parse -> middleware -> handler -> response -> write -> close
```

1. The server creates a listening socket and accepts clients in non-blocking mode.
2. Each connection accumulates bytes until the HTTP header terminator is present.
3. The parser extracts the method, path, HTTP version, and request headers.
4. Registered middleware functions run in registration order.
5. The method and path select a handler from the routing table.
6. The handler mutates a response object.
7. The response is serialized and written incrementally before the connection is closed.

On Linux, `epoll` tracks listener and connection readiness. Other supported POSIX builds use the `poll` loop. Connection objects retain read/write progress so partial socket operations can resume on the next readiness event.

## Routing and handlers

Handlers use a small request/response callback interface:

```c
void health_handler(hcb_request_t *req, hcb_response_t *resp) {
  hcb_response_set_status(resp, "200 OK");
  hcb_response_set_header(resp, "Content-Type", "text/plain");
  hcb_body_append(resp, "healthy\n");
}
```

Routes are registered by HTTP method:

```c
hcb_server_get(server, "/health", health_handler);
hcb_server_post(server, "/health", post_health_handler);
```

`GET`, `POST`, `PUT`, `PATCH`, and `DELETE` registration helpers are present. The router combines the method and endpoint into the lookup key.

## Middleware

Middleware shares the same request/response arguments as a route handler:

```c
void add_common_header(hcb_request_t *req, hcb_response_t *resp) {
  hcb_response_set_header(resp, "X-Server", "http-c");
}

hcb_server_register_middleware(server, add_common_header);
```

Registered middleware forms an ordered linked chain. Every middleware function runs before the route handler and may inspect the parsed request or mutate the response. The current API does not provide `next()` control, short-circuiting, or per-route middleware.

## Request and response abstractions

`hcb_request_t` exposes the parsed method, endpoint, and headers through accessor functions. `hcb_response_t` owns the status, response headers, and body; handlers update it through `hcb_response_set_status`, `hcb_response_set_header`, `hcb_body_set`, and `hcb_body_append`.

## Build and run

The sample application listens on port `8080`.

```bash
make
./bin/out
```

On Linux toolchains that hide POSIX networking declarations in strict C17 mode, enable the POSIX feature set explicitly:

```bash
make clean
make CFLAGS="-fsanitize=address -Wall -Wextra -std=c17 -D_POSIX_C_SOURCE=200809L -I./src/include"
./bin/out
```

## Example requests

```bash
curl -i http://localhost:8080/
curl -i http://localhost:8080/health
curl -i -X POST http://localhost:8080/health
```

## Performance

The repository does not include a repeatable benchmark suite or recorded benchmark results, so no throughput or latency claim is made. The event-driven design is intended to study non-blocking I/O rather than establish production performance.

## Limitations

- Supports only a small HTTP/1.1 subset; there is no TLS, HTTP/2, chunked transfer decoding, keep-alive, or request-body handling.
- Request buffers, header counts, header sizes, and routing capacity are fixed.
- Query-string and URL-decoding behavior is not implemented as a complete HTTP parser.
- Responses close the connection after one request.
- The project has no automated parser, integration, fuzz, or benchmark suite.
- Sanitizer-enabled builds currently surface compiler warnings in memory-management helpers; the code should be treated as an experiment, not a production server.
