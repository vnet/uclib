/*
  Copyright (c) 2014 Eliot Dresselhaus

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef included_clib_websocket_h
#define included_clib_websocket_h

typedef struct {
  clib_socket_t clib_socket;

  /* Index in socket pool. */
  u32 index;

  /* Normally 13. */
  u32 websocket_version;

  u8 is_server_client : 1;
  u8 handshake_rx : 1;

  /* Used to timeout inactive connections which don't complete handshake. */
  f64 time_stamp_of_connection_creation;

  u32 unix_file_poller_file_index;

  uword opaque[2];

  union {
    struct {
      /* Web socket url. */
      url_t url;

      /* 16 bytes sent via Sec-WebSocket-Key. */
      u8 sec_websocket_key_random_bytes[16];
    } client;

    struct {
      http_request_or_response_t http_handshake_request;
    } server;
  };
} websocket_socket_t;

typedef enum {
  WEBSOCKET_CONNECTION_TYPE_SERVER_LISTEN,
  WEBSOCKET_CONNECTION_TYPE_SERVER_CLIENT,
  WEBSOCKET_CONNECTION_TYPE_CLIENT,
} websocket_connection_type_t;

always_inline websocket_connection_type_t
websocket_connection_type (websocket_socket_t * ws)
{
  clib_socket_t * s = &ws->clib_socket;
  return (s->is_server
          ? WEBSOCKET_CONNECTION_TYPE_SERVER_LISTEN
          : (ws->is_server_client
             ? WEBSOCKET_CONNECTION_TYPE_SERVER_CLIENT
             : WEBSOCKET_CONNECTION_TYPE_CLIENT));
}

always_inline void
websocket_socket_free (websocket_socket_t * ws)
{
  clib_socket_free (&ws->clib_socket);
  if (ws->is_server_client)
    {
      http_request_or_response_free (&ws->server.http_handshake_request);
    }
  else
    {
      url_free (&ws->client.url);
    }
  memset (ws, ~0, sizeof (ws[0])); /* poison */
}

#define foreach_websocket_data_framing_opcode   \
  /* Data frames. */                            \
  _ (CONTINUATION, 0x0)                         \
  _ (TEXT_DATA, 0x1)                            \
  _ (BINARY_DATA, 0x2)                          \
  /* Control frames have 0x8 bit set. */        \
  _ (CLOSE, 0x8)                                \
  _ (PING, 0x9)                                 \
  _ (PONG, 0xa)

typedef enum {
#define _(f,n) WEBSOCKET_DATA_FRAMING_OPCODE_##f = n,
  foreach_websocket_data_framing_opcode
#undef _
} websocket_data_framing_opcode_type_t;

/* Added to opcode byte to indicate final fragment. */
#define WEBSOCKET_DATA_FRAMING_IS_FINAL_FRAGMENT (1 << 7)

/* Added to length byte to indicate payload is masked with 32bit value. */
#define WEBSOCKET_DATA_FRAMING_PAYLOAD_IS_MASKED (1 << 7)

typedef struct websocket_main_t {
  /* Pool of connections. */
  websocket_socket_t * socket_pool;

  /* Frames with payload larger than this size will cause connection to close. */
  u64 max_n_bytes_in_payload;

  clib_error_t *
  (* rx_frame_payload) (struct websocket_main_t * wsm, websocket_socket_t * c, u8 * rx_payload, u32 n_payload_bytes);

  void (* new_client_for_server) (struct websocket_main_t * wsm,
                                  u32 client_ws_index,
                                  u32 server_ws_index);

  void (* connection_will_close) (struct websocket_main_t * wsm, u32 ws_index, clib_error_t * reason);

  void (* did_receive_handshake) (struct websocket_main_t * wsm, u32 ws_index);

  /* "Host:" field in handshake must match something in hash table. */
  uword * host_name_hash;

  unix_file_poller_t * unix_file_poller;

  /* Random buffer for sec_websocket_key.  Seeded with /dev/urandom. */
  clib_random_buffer_t random_buffer;

  /* If correct handshake is not received before a certain time close connection. */
  f64 rx_handshake_timeout_in_sec;

  u32 verbose;
} websocket_main_t;

clib_error_t * websocket_init (websocket_main_t * wsm);
void websocket_close (websocket_main_t * wsm, u32 ws_index);
void websocket_close_all_sockets_with_no_handshake (websocket_main_t * wsm);
clib_error_t * websocket_socket_tx_binary_frame (websocket_socket_t * ws);
clib_error_t * websocket_socket_tx_text_frame (websocket_socket_t * ws);

void websocket_server_add_host (websocket_main_t * wsm, char * fmt, ...);
clib_error_t * websocket_server_add_listener (websocket_main_t * wsm, char * config, u32 * ws_index);

clib_error_t *
websocket_client_add_connection (websocket_main_t * wsm, char * config, char * url, u32 * ws_index);

#endif /* included_clib_websocket_h */
