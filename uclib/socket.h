/*
  Copyright (c) 2001, 2002, 2003 Eliot Dresselhaus

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

#ifndef _clib_included_socket_h
#define _clib_included_socket_h

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

typedef union {
  struct sockaddr sa;
  struct sockaddr_in in;
  struct sockaddr_un un;
} clib_socket_addr_t;

typedef struct {
  u32 offset;
} clib_socket_buffer_header_t;

typedef struct _socket_t {
  /* File descriptor. */
  int fd;

  /* Config string for socket HOST:PORT or just HOST.
     Unspecified port causes a free one to be chosen starting
     from IPPORT_USERRESERVED (5000). */
  char * config;

  u32 is_server : 1;
  u32 is_client : 1;
  u32 non_blocking_connect : 1;
  u32 non_blocking_connect_in_progress : 1;
  u32 tcp_delay : 1;            /* set to enable Nagle. */
  u32 rx_end_of_file : 1;

  /* Transmit buffer.  Holds data waiting to be written. */
  u8 * current_tx_buffer;

  /* FIFO of tx buffers to be sent out.  tx_buffer above will
     be added to tail of FIFO when tx function is called. */
  u8 ** tx_buffer_fifo;

  /* For writev system call. */
  struct iovec * tx_buffer_iovecs;

  u8 * tx_add_formatted_buffer;

  /* Receive buffer.  Holds data read from socket. */
  u8 * rx_buffer;

  /* Max size of tx buffers. */
  u32 max_tx_buffer_bytes;

  /* Peer socket we are connected to. */
  clib_socket_addr_t self_addr, peer_addr;
} clib_socket_t;

#ifndef CLIB_SOCKET_BUFFER_ALIGN
#define CLIB_SOCKET_BUFFER_ALIGN 64
#endif

always_inline void
clib_socket_buffer_free (u8 * b)
{ vec_free_h (b, sizeof (clib_socket_buffer_header_t)); }

always_inline void
clib_socket_free (clib_socket_t *s)
{
  uword i, n = clib_fifo_elts (s->tx_buffer_fifo);
  for (i = 0; i < n; i++)
    {
      u8 * b = * clib_fifo_elt_at_index (s->tx_buffer_fifo, i);
      clib_socket_buffer_free (b);
    }
  clib_fifo_free (s->tx_buffer_fifo);
  vec_free (s->tx_buffer_iovecs);
  clib_socket_buffer_free (s->current_tx_buffer);
  vec_free (s->tx_add_formatted_buffer);
  vec_free (s->rx_buffer);
  if (clib_mem_is_heap_object (vec_header (s->config, 0)))
    vec_free (s->config);
  memset (s, 0, sizeof (s[0]));
}

clib_error_t * clib_socket_init (clib_socket_t * socket);

/* Accept a new connection. */
clib_error_t * clib_socket_accept (clib_socket_t * server, clib_socket_t * client);

always_inline void *
clib_socket_tx_add2_with_offset (clib_socket_t * s, uword n_bytes, uword offset)
{
  u8 * result;
  clib_socket_buffer_header_t * bh;

  vec_add2_ha (s->current_tx_buffer, result, n_bytes,
               sizeof (clib_socket_buffer_header_t), CLIB_SOCKET_BUFFER_ALIGN);

  bh = vec_header (s->current_tx_buffer, sizeof (bh[0]));
  bh->offset = offset;

  return result;
}

always_inline void *
clib_socket_tx_add2 (clib_socket_t * s, uword n_bytes)
{ return clib_socket_tx_add2_with_offset (s, n_bytes, /* offset */ 0); }

always_inline void *
clib_socket_tx_add_with_offset (clib_socket_t * s, void * data, uword n_bytes, uword offset)
{
  void * d = clib_socket_tx_add2_with_offset (s, n_bytes, offset);
  memcpy (d, data, n_bytes);
  return d;
}

always_inline void *
clib_socket_tx_add (clib_socket_t * s, void * data, uword n_bytes)
{ return clib_socket_tx_add_with_offset (s, data, n_bytes, /* offset */ 0); }

always_inline void
clib_socket_tx_add_va_formatted (clib_socket_t * s, char * fmt, va_list * va)
{
  vec_reset_length (s->tx_add_formatted_buffer);
  s->tx_add_formatted_buffer = va_format (s->tx_add_formatted_buffer, fmt, va);
  vec_add_ha (s->current_tx_buffer, s->tx_add_formatted_buffer, vec_len (s->tx_add_formatted_buffer),
              sizeof (clib_socket_buffer_header_t), CLIB_SOCKET_BUFFER_ALIGN);
}

always_inline void
clib_socket_tx_add_to_buffer_fifo (clib_socket_t * s, u8 * data, uword n_data)
{
  u8 * buffer = 0;

  vec_add_ha (buffer, data, n_data,
              sizeof (clib_socket_buffer_header_t), CLIB_SOCKET_BUFFER_ALIGN);

  /* Buffer will be freed on transmit. */
  clib_fifo_add1 (s->tx_buffer_fifo, buffer);
}

always_inline uword
clib_socket_tx_data_is_available_to_write (clib_socket_t * s)
{
  return (vec_len (s->current_tx_buffer) > 0
          || clib_fifo_elts (s->tx_buffer_fifo) > 0);
}

void clib_socket_tx_add_formatted (clib_socket_t * s, char * fmt, ...);

clib_error_t * clib_socket_rx (clib_socket_t * s, int n_bytes);
clib_error_t * clib_socket_tx (clib_socket_t * s);
clib_error_t * clib_socket_close (clib_socket_t *sock);

always_inline clib_error_t *
clib_socket_non_blocking_connect_status (clib_socket_t * s)
{
  if (s->non_blocking_connect_in_progress)
    {
      u32 len, value;

      s->non_blocking_connect_in_progress = 0;

      len = sizeof (value);
      if (getsockopt (s->fd, SOL_SOCKET, SO_ERROR, &value, &len) < 0)
        return clib_error_return_unix (0, "getsockopt SO_ERROR");
      if (value != 0)
	return clib_error_return_code (0, value, CLIB_ERROR_ERRNO_VALID, "connect fails");
    }

  return /* no error */ 0;
}

#endif /* _clib_included_socket_h */
