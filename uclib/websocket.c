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

#include <uclib/uclib.h>

always_inline websocket_socket_t *
websocket_socket_alloc (websocket_main_t * wsm)
{
  websocket_socket_t * ws;
  pool_get (wsm->socket_pool, ws);
  memset (ws, 0, sizeof (ws[0]));
  ws->index = ws - wsm->socket_pool;
  return ws;
}

static clib_error_t *
websocket_main_close_socket (websocket_main_t * wsm, websocket_socket_t * ws, clib_error_t * error)
{
  clib_socket_t * s = &ws->clib_socket;
  unix_file_poller_file_t * f;

  if (wsm->connection_will_close)
    wsm->connection_will_close (wsm, ws->index, error);

  if (wsm->verbose)
    clib_warning ("close connection %U -> %U",
                  format_sockaddr, &s->self_addr,
                  format_sockaddr, &s->peer_addr);

  f = unix_file_poller_get_file (wsm->unix_file_poller, ws->unix_file_poller_file_index);
  unix_file_poller_del_file (wsm->unix_file_poller, f);
  websocket_socket_free (ws);
  pool_put (wsm->socket_pool, ws);

  return error;
}

void websocket_close (websocket_main_t * wsm, u32 ws_index)
{
  websocket_socket_t * ws = pool_elt_at_index (wsm->socket_pool, ws_index);
  websocket_main_close_socket (wsm, ws, /* no error */ 0);
}

static clib_error_t *
websocket_rx_handshake_timeout (websocket_main_t * wsm, websocket_socket_t * ws)
{
  clib_error_t * error = clib_error_return (0, "receive handshake timeout");
  return websocket_main_close_socket (wsm, ws, error);
}

static int parse_rx_frame (websocket_main_t * wsm, websocket_socket_t * ws)
{
  u32 n_left_in_rx_buffer;
  u32 payload_byte_size, n_bytes_in_frame_header;
  u64 n_bytes_of_payload;
  u8 frame_mask[4];
  u8 * rx, * rx_frame_start, is_masked;
  clib_socket_t * s = &ws->clib_socket;
  clib_error_t * error = 0;
  uword rx_frame_parsed = 0;

  rx = s->rx_buffer;
  n_left_in_rx_buffer = vec_len (s->rx_buffer);

  while (1)
    {
      /* Shortest valid frame: 2 bytes header + 0 bytes of payload. */
      if (n_left_in_rx_buffer < 2)
        goto done;

      /* Keep it simple: no support for fragmentation. */
      rx_frame_start = rx;
      if (! (rx[0] & WEBSOCKET_DATA_FRAMING_IS_FINAL_FRAGMENT))
        {
          error = clib_error_return (0, "no support for fragmentation (opcode 0x%x)", rx[0]);
          goto done;
        }

      switch (rx[0] & 0xf)
        {
        case WEBSOCKET_DATA_FRAMING_OPCODE_BINARY_DATA:
        case WEBSOCKET_DATA_FRAMING_OPCODE_TEXT_DATA:
          break;

        case WEBSOCKET_DATA_FRAMING_OPCODE_CLOSE:
          error = clib_error_return (0, "close opcode received");
          goto done;

        default:
          error = clib_error_return (0, "unknown opcode 0x%x", rx[0]);
          goto done;
        }

      is_masked = (rx[1] & WEBSOCKET_DATA_FRAMING_PAYLOAD_IS_MASKED) != 0;

      n_bytes_of_payload = rx[1] & 0x7f;

      payload_byte_size = 0;
      payload_byte_size = n_bytes_of_payload == 126 ? 2 : payload_byte_size;
      payload_byte_size = n_bytes_of_payload == 127 ? 8 : payload_byte_size;

      n_bytes_in_frame_header = 2 + (is_masked ? sizeof (u32) : 0) + payload_byte_size;
      if (n_left_in_rx_buffer < n_bytes_in_frame_header)
        goto done;

      rx += 2;
      if (n_bytes_of_payload >= 126)
        {
          if (n_bytes_of_payload == 126)
            n_bytes_of_payload = (rx[0] << 8) | rx[1];
          else
#define _(i) ((u64) rx[i] << (u64) (8*(7-i)))
            n_bytes_of_payload = _ (0) | _ (1) | _ (2) | _ (3) | _ (4) | _ (5) | _ (6) | _ (7);
#undef _
        }

      /* Bit 63 (sign bit) must be clear.  Enforce this by having a sane
         max payload size. */
      if (n_bytes_of_payload > wsm->max_n_bytes_in_payload)
        {
          error = clib_error_return (0, "payload overflow %Ld > %Ld",
                                     n_bytes_of_payload, wsm->max_n_bytes_in_payload);
          goto done;
        }

      rx += payload_byte_size;

      memset (frame_mask, 0, sizeof (frame_mask));
      if (is_masked)
        {
          /* Host byte order. */
          memcpy (frame_mask, rx, sizeof (frame_mask));
          rx += sizeof (frame_mask);
        }

      if (n_left_in_rx_buffer < n_bytes_in_frame_header + n_bytes_of_payload)
        goto done;

      if (is_masked)
        {
          int i;
          for (i = 0; i < n_bytes_of_payload; i++)
            rx_frame_start[n_bytes_in_frame_header + i] ^= frame_mask[i % sizeof (frame_mask)];
        }

      ASSERT (wsm->rx_frame_payload);
      error = wsm->rx_frame_payload (wsm, ws, rx_frame_start + n_bytes_in_frame_header, n_bytes_of_payload);

      n_left_in_rx_buffer -= n_bytes_in_frame_header + n_bytes_of_payload;
      rx = rx_frame_start + n_bytes_in_frame_header + n_bytes_of_payload;
    }

  /* Remove header and payload from receive buffer. */
  vec_delete (s->rx_buffer, rx - s->rx_buffer, 0);
  rx_frame_parsed = error == 0;

 done:
  if (error)
    {
      websocket_main_close_socket (wsm, ws, error);
      ASSERT (rx_frame_parsed == 0);
      unix_save_error (wsm->unix_file_poller, error);
    }

  return rx_frame_parsed;
}

static int
parse_rx_handshake (websocket_main_t * wsm, websocket_socket_t * ws,
                    uword * rx_buffer_advance)
{
  clib_socket_t * s = &ws->clib_socket;
  unformat_input_t input;
  http_request_or_response_t r;
  u8 * websocket_key = 0;
  int is_ok = 0;

  unformat_init_vector (&input, s->rx_buffer);
  if (! unformat_user (&input, unformat_http_request, &r))
    goto done;

  if (r.request.method != HTTP_REQUEST_METHOD_GET)
    goto done;

  if (r.http_version[0] != 1 && r.http_version[1] != 1)
    goto done;

  /* Host: must match. */
  if (hash_elts (wsm->host_name_hash) > 0
      && ! hash_get_mem (wsm->host_name_hash, http_request_value_for_key (&r, "host")))
    goto done;
      
  /* Check client version. */
  if (! http_request_unformat_value_for_key (&r, "sec-websocket-version", "%d", &ws->websocket_version)
      || ws->websocket_version < 13
      || ws->websocket_version >= 256)
    goto done;

  if (! http_request_value_for_key_compare (&r, "upgrade", "websocket"))
    goto done;
  if (! http_request_value_for_key_compare (&r, "connection", "Upgrade"))
    goto done;

  {
    u8 * v = http_request_value_for_key (&r, "sec-websocket-key");
    u8 sum[20];

    if (! v)
      goto done;

    websocket_key = format (0, "%v%s", v, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    sha1 (sum, websocket_key, vec_len (websocket_key));

    clib_socket_tx_add_formatted
      (s,
       "HTTP/1.1 101 Switching Protocols\r\n"
       "Upgrade: websocket\r\n"
       "Connection: Upgrade\r\n"
       "Sec-WebSocket-Accept: %U\r\n"
       "\r\n",
       format_base64_data, sum, sizeof (sum));
    clib_socket_tx (s);

    is_ok = 1;
  }

 done:
  /* Let caller know how much we've advanced rx buffer. */
  *rx_buffer_advance = input.index;
  input.buffer = 0;             /* don't free clib_socket rx_buffer */
  unformat_free (&input);
  vec_free (websocket_key);
  if (is_ok)
    ws->server.http_handshake_request = r;
  else
    http_request_or_response_free (&r);
  return is_ok;
}

static int
parse_tx_handshake (websocket_main_t * wsm, websocket_socket_t * ws,
                    uword * rx_buffer_advance)
{
  clib_socket_t * s = &ws->clib_socket;
  unformat_input_t input;
  http_request_or_response_t r;
  int is_ok = 0;

  unformat_init_vector (&input, s->rx_buffer);
  if (! unformat_user (&input, unformat_http_response, &r))
    goto done;

  if (r.http_version[0] != 1 && r.http_version[1] != 1)
    goto done;
  if (r.response.code != 101)
    goto done;

  if (! http_request_value_for_key_compare (&r, "upgrade", "websocket"))
    goto done;
  if (! http_request_value_for_key_compare (&r, "connection", "Upgrade"))
    goto done;

  {
    u8 * sec_websocket_accept = 0;
    u8 * websocket_key = 0;
    u8 sum[20];
    uword sum_matches;

    if (! http_request_unformat_value_for_key (&r, "sec-websocket-accept", "%U", unformat_base64_data, &sec_websocket_accept))
      goto done;

    websocket_key = format (0, "%U%s",
                            format_base64_data, ws->client.sec_websocket_key_random_bytes, sizeof (ws->client.sec_websocket_key_random_bytes),
                            "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    sha1 (sum, websocket_key, vec_len (websocket_key));
    vec_free (websocket_key);

    sum_matches = (vec_len (sec_websocket_accept) == sizeof (sum)
                   && ! memcmp (sec_websocket_accept, sum, sizeof (sum)));
    vec_free (sec_websocket_accept);
    vec_free (websocket_key);

    if (! sum_matches)
      goto done;

    is_ok = 1;
  }

 done:
  /* Let caller know how much we've advanced rx buffer. */
  *rx_buffer_advance = input.index;
  input.buffer = 0;             /* don't free clib_socket rx_buffer */
  unformat_free (&input);
  http_request_or_response_free (&r);
  return is_ok;
}

static clib_error_t *
websocket_file_error_ready (unix_file_poller_file_t * f)
{
  websocket_main_t * wsm = uword_to_pointer (f->private_data[0], websocket_main_t *);
  websocket_socket_t * ws = pool_elt_at_index (wsm->socket_pool, f->private_data[1]);
  clib_error_t * error = 0;
  int error_errno = 0;
  socklen_t error_errno_len = sizeof (error_errno);

  if (getsockopt (f->file_descriptor, SOL_SOCKET, SO_ERROR, (void *) &error_errno, &error_errno_len) < 0)
    error = clib_error_return_unix (0, "getsockopt SO_ERROR");
  else
    error = clib_error_return (0, "error %s", strerror (error_errno));

  websocket_main_close_socket (wsm, ws, error); /* frees error */
  error = 0;

  return 0;
}

static clib_error_t *
websocket_server_file_read_ready (unix_file_poller_file_t * f)
{
  websocket_main_t * wsm = uword_to_pointer (f->private_data[0], websocket_main_t *);
  websocket_socket_t * ws = pool_elt_at_index (wsm->socket_pool, f->private_data[1]);
  clib_socket_t * s = &ws->clib_socket;
  clib_error_t * error = clib_socket_rx (s, 4096);

  if (error)
    return error;

  if (s->rx_end_of_file)
    {
      websocket_main_close_socket (wsm, ws, error);
      return 0;
    }

  if (! ws->handshake_rx)
    {
      uword rx_buffer_advance;
      if (! parse_rx_handshake (wsm, ws, &rx_buffer_advance))
        {
          f64 time_now = unix_time_now ();
          if (time_now > ws->time_stamp_of_connection_creation + wsm->rx_handshake_timeout_in_sec)
            return websocket_rx_handshake_timeout (wsm, ws);
          else
            return error;
        }

      ws->handshake_rx = 1;

      if (wsm->did_receive_handshake)
        {
          error = wsm->did_receive_handshake (wsm, ws->index);
          if (error)
            return websocket_main_close_socket (wsm, ws, error);
        }

      /* Remove handshake from RX buffer. */
      vec_delete (s->rx_buffer, rx_buffer_advance, 0);
    }

  parse_rx_frame (wsm, ws);

  return error;
}

static clib_error_t *
websocket_server_file_accept_on_read_ready (unix_file_poller_file_t * f)
{
  websocket_main_t * wsm = uword_to_pointer (f->private_data[0], websocket_main_t *);
  websocket_socket_t * server_ws = pool_elt_at_index (wsm->socket_pool, f->private_data[1]);
  websocket_socket_t * client_ws;
  clib_socket_t * server_socket = &server_ws->clib_socket;
  clib_socket_t * client_socket;
  unix_file_poller_file_t client_file;
  clib_error_t * error = 0;

  client_ws = websocket_socket_alloc (wsm);
  client_ws->is_server_client = 1;
  client_socket = &client_ws->clib_socket;

  error = clib_socket_accept (server_socket, client_socket);
  if (error)
    goto done;

  memset (&client_file, 0, sizeof (client_file));
  client_file.private_data[0] = pointer_to_uword (wsm);
  client_file.private_data[1] = client_ws->index;
  client_file.file_descriptor = client_socket->fd;
  client_file.read_function = websocket_server_file_read_ready;
  client_file.error_function = websocket_file_error_ready;

  client_ws->unix_file_poller_file_index = unix_file_poller_add_file (wsm->unix_file_poller, &client_file);

  memcpy (&client_ws->opaque, server_ws->opaque, sizeof (server_ws->opaque));

  client_ws->time_stamp_of_connection_creation = unix_time_now ();

  wsm->new_client_for_server (wsm, client_ws->index, server_ws->index);

  if (wsm->verbose)
    clib_warning ("new connection index %d %U",
                  client_ws->index,
                  format_sockaddr, &client_socket->peer_addr);

 done:
  if (error)
    {
      websocket_socket_free (client_ws);
      pool_put (wsm->socket_pool, client_ws);
    }
  return error;
}

static clib_error_t *
websocket_client_file_read_ready (unix_file_poller_file_t * f)
{
  websocket_main_t * wsm = uword_to_pointer (f->private_data[0], websocket_main_t *);
  websocket_socket_t * ws = pool_elt_at_index (wsm->socket_pool, f->private_data[1]);
  clib_socket_t * s = &ws->clib_socket;
  clib_error_t * error;

  error = clib_socket_rx (s, 4096);
  if (error)
    return error;

  if (s->rx_end_of_file)
    return websocket_main_close_socket (wsm, ws, error);

  if (! ws->handshake_rx)
    {
      uword rx_buffer_advance;
      if (! parse_tx_handshake (wsm, ws, &rx_buffer_advance))
        {
          f64 time_now = unix_time_now ();
          if (time_now > ws->time_stamp_of_connection_creation + wsm->rx_handshake_timeout_in_sec)
            return websocket_rx_handshake_timeout (wsm, ws);
          else
            return error;
        }

      ws->handshake_rx = 1;

      /* Remove handshake from RX buffer. */
      vec_delete (s->rx_buffer, rx_buffer_advance, 0);
    }

  parse_rx_frame (wsm, ws);

  return error;
}

static clib_error_t *
websocket_client_file_write_ready (unix_file_poller_file_t * f)
{
  websocket_main_t * wsm = uword_to_pointer (f->private_data[0], websocket_main_t *);
  websocket_socket_t * ws = pool_elt_at_index (wsm->socket_pool, f->private_data[1]);
  clib_socket_t * s = &ws->clib_socket;
  clib_error_t * error = 0;

  if (s->non_blocking_connect_in_progress)
    {
      error = clib_socket_non_blocking_connect_status (s);
      if (error)
        return error;

      ws->time_stamp_of_connection_creation = unix_time_now ();

      {
        u8 * k = clib_random_buffer_get_data (&wsm->random_buffer, sizeof (ws->client.sec_websocket_key_random_bytes));
        memcpy (ws->client.sec_websocket_key_random_bytes, k, sizeof (ws->client.sec_websocket_key_random_bytes));
      }

      /* Connection completed: send handshake. */
      {
        url_t * url = &ws->client.url;

        clib_socket_tx_add_formatted
          (s,
           "GET %v%s%v HTTP/1.1\r\n"
           "Host: %v\r\n"
           "Upgrade: websocket\r\n"
           "Connection: Upgrade\r\n"
           "Sec-WebSocket-Key: %U\r\n"
           "Sec-WebSocket-Version: 13\r\n"
           "\r\n",
           url_path (url),
           vec_len (url_query (url)) > 0 ? "?" : "",
           url_query (url),
           url_host (url),
           format_base64_data, ws->client.sec_websocket_key_random_bytes, sizeof (ws->client.sec_websocket_key_random_bytes));
      }
  }

  error = clib_socket_tx (s);
  if (error && unix_error_is_fatal (errno))
    return error;

  unix_file_poller_set_data_available_to_write (wsm->unix_file_poller,
                                                f - wsm->unix_file_poller->file_pool,
                                                clib_socket_tx_data_is_available_to_write (s));

  return error;
}

clib_error_t *
websocket_init (websocket_main_t * wsm)
{
  clib_error_t * error = 0;

  error = unix_file_poller_init (wsm->unix_file_poller);
  if (error)
    goto done;

  {
    u8 seed[clib_random_buffer_seed_bytes];

    error = unix_file_read_contents ("/dev/urandom", seed, sizeof (seed));
    if (error)
      goto done;

    clib_random_buffer_init_multiseed (&wsm->random_buffer, seed);
  }

  if (! wsm->max_n_bytes_in_payload)
    wsm->max_n_bytes_in_payload = 16 << 10;

  if (wsm->rx_handshake_timeout_in_sec <= 0)
    wsm->rx_handshake_timeout_in_sec = 5;

 done:
  return error;
}

/* Reap all sockets that have not sent proper handshakes. */
void websocket_close_all_sockets_with_no_handshake (websocket_main_t * wsm)
{
  websocket_socket_t * ws;
  f64 time_now = unix_time_now ();
  uword * close_bitmap = 0;

  pool_foreach (ws, wsm->socket_pool, ({
    if (! ws->is_server_client)
      continue;
    if (! ws->handshake_rx
        && time_now - ws->time_stamp_of_connection_creation > wsm->rx_handshake_timeout_in_sec)
      close_bitmap = clib_bitmap_ori (close_bitmap, ws->index);
  }));

  {
    uword ws_index;
    clib_error_t * error;

    clib_bitmap_foreach (ws_index, close_bitmap, ({
      ws = pool_elt_at_index (wsm->socket_pool, ws_index);
      error = websocket_rx_handshake_timeout (wsm, ws);
      unix_save_error (wsm->unix_file_poller, error);
    }));
  }

  clib_bitmap_free (close_bitmap);
}

static clib_error_t * websocket_socket_tx_frame_helper (websocket_socket_t * ws, uword is_binary)
{
  clib_socket_t * s = &ws->clib_socket;
  u32 l;
  u8 * f, framing_data[2 + 8];

  f = framing_data;

  *f++ = (WEBSOCKET_DATA_FRAMING_IS_FINAL_FRAGMENT |
          (is_binary ? WEBSOCKET_DATA_FRAMING_OPCODE_BINARY_DATA : WEBSOCKET_DATA_FRAMING_OPCODE_TEXT_DATA));

  l = vec_len (s->current_tx_buffer);
  if (l < 126)
    *f++ = l;
  else if (l < (1 << 16))
    {
      f[0] = 126;
      clib_mem_unaligned (f + 1, u16) = clib_host_to_net_u16 (l);
      f += 1 + sizeof (u16);
    }
  else
    {
      f[0] = 127;
      clib_mem_unaligned (f + 1, u64) = clib_host_to_net_u64 ((u64) l);
      f += 1 + sizeof (u64);
    }

  /* Add framing to buffer fifo.  Framing data will be added to tail of
     buffer fifo *before* current tx_buffer. */
  clib_socket_tx_add_to_buffer_fifo (s, framing_data, f - framing_data);

  return clib_socket_tx (s);
}

clib_error_t * websocket_socket_tx_binary_frame (websocket_socket_t * ws)
{ return websocket_socket_tx_frame_helper (ws, /* is_binary */ 1); }
clib_error_t * websocket_socket_tx_text_frame (websocket_socket_t * ws)
{ return websocket_socket_tx_frame_helper (ws, /* is_binary */ 0); }

void websocket_server_add_host (websocket_main_t * wsm, char * fmt, ...)
{
  va_list va;
  u8 * host;

  va_start (va, fmt);
  host = va_format (0, fmt, &va);
  va_end (va);

  if (! wsm->host_name_hash)
    wsm->host_name_hash = hash_create_vec (8, sizeof (u8), 0);

  hash_set1_mem (wsm->host_name_hash, host);
}

clib_error_t *
websocket_server_add_listener (websocket_main_t * wsm, char * config, u32 * ws_index)
{
  clib_error_t * error = 0;
  websocket_socket_t * ws;
  clib_socket_t * s;
  unix_file_poller_file_t pf;

  ws = websocket_socket_alloc (wsm);

  s = &ws->clib_socket;

  s->is_server = 1;
  s->config = config;
  error = clib_socket_init (s);
  if (error)
    goto done;

  if (wsm->verbose)
    clib_warning ("listening %U", format_sockaddr, &s->self_addr);

  memset (&pf, 0, sizeof (pf));
  pf.file_descriptor = s->fd;
  pf.private_data[0] = pointer_to_uword (wsm);
  pf.private_data[1] = ws->index;
  pf.read_function = websocket_server_file_accept_on_read_ready;
  pf.error_function = websocket_file_error_ready;
  ws->unix_file_poller_file_index = unix_file_poller_add_file (wsm->unix_file_poller, &pf);

 done:
  if (error)
    {
      websocket_socket_free (ws);
      pool_put (wsm->socket_pool, ws);
    }
  else
    *ws_index = ws->index;

  return error;
}

clib_error_t *
websocket_client_add_connection (websocket_main_t * wsm, u32 * ws_index, char * url_format, ...)
{
  va_list va;
  websocket_socket_t * ws;
  clib_socket_t * s;
  unix_file_poller_file_t pf;
  clib_error_t * error = 0;
  u8 * url = 0;

  va_start (va, url_format);
  url = va_format (0, url_format, &va);
  va_end (va);

  ws = websocket_socket_alloc (wsm);

  error = url_parse_components ((u8 *) url, &ws->client.url);
  if (error)
    goto done;

  s = &ws->clib_socket;
  s->is_client = 1;
  s->non_blocking_connect = 1;
  s->config = url_socket_config (&ws->client.url);
  error = clib_socket_init (s);
  if (error)
    goto done;
        
  memset (&pf, 0, sizeof (pf));
  pf.read_function = websocket_client_file_read_ready;
  pf.write_function = websocket_client_file_write_ready;
  pf.error_function = websocket_file_error_ready;
  if (s->non_blocking_connect)
    pf.flags = UNIX_FILE_POLLER_FILE_DATA_AVAILABLE_TO_WRITE;
  pf.file_descriptor = s->fd;
  pf.private_data[0] = pointer_to_uword (wsm);
  pf.private_data[1] = ws->index;
  ws->unix_file_poller_file_index = unix_file_poller_add_file (wsm->unix_file_poller, &pf);

done:
  if (error)
    {
      websocket_socket_free (ws);
      pool_put (wsm->socket_pool, ws);
    }
  else
    *ws_index = ws->index;
  vec_free (url);

  return error;
}
