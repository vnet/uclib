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
  u32 rx_end_of_file : 1;

  /* Transmit buffer.  Holds data waiting to be written. */
  u8 * tx_buffer;

  /* Receive buffer.  Holds data read from socket. */
  u8 * rx_buffer;

  /* Max size of tx buffers. */
  u32 max_tx_buffer_bytes;

  /* Peer socket we are connected to. */
  clib_socket_addr_t self_addr, peer_addr;

  clib_error_t * (* write_func) (struct _socket_t * sock);
  clib_error_t * (* read_func) (struct _socket_t * sock, int min_bytes);
  clib_error_t * (* close_func) (struct _socket_t * sock);
  void * private_data;
} clib_socket_t;

always_inline void
clib_socket_free (clib_socket_t *s)
{
  vec_free (s->tx_buffer);
  vec_free (s->rx_buffer);
  if (clib_mem_is_heap_object (s->config))
    vec_free (s->config);
  memset (s, 0, sizeof (s[0]));
}

clib_error_t * clib_socket_init (clib_socket_t * socket);

/* Accept a new connection. */
clib_error_t * clib_socket_accept (clib_socket_t * server, clib_socket_t * client);

always_inline void *
clib_socket_tx_add (clib_socket_t * s, int n_bytes)
{
  u8 * result;
  vec_add2 (s->tx_buffer, result, n_bytes);
  return result;
}

always_inline void
clib_socket_tx_add_va_formatted (clib_socket_t * s, char * fmt, va_list * va)
{ s->tx_buffer = va_format (s->tx_buffer, fmt, va); }

void clib_socket_tx_add_formatted (clib_socket_t * s, char * fmt, ...);

always_inline clib_error_t *
clib_socket_tx (clib_socket_t * s)
{ return s->write_func (s); }

always_inline clib_error_t *
clib_socket_rx (clib_socket_t * s, int n_bytes)
{ return s->read_func (s, n_bytes); }

always_inline clib_error_t *
clib_socket_close (clib_socket_t *sock)
{
  clib_error_t * err;
  err = (* sock->close_func) (sock);
  return err;
}

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
