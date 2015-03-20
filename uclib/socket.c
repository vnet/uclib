/*
  Copyright (c) 2001, 2002, 2003, 2005 Eliot Dresselhaus

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

#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>        /* TCP_NODELAY */
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>             /* strchr */

/* Return and bind to an unused port. */
static word find_free_port (word sock)
{
  word port;

  for (port = IPPORT_USERRESERVED; port < 1 << 16; port++)
    {
      struct sockaddr_in a;

      a.sin_family = PF_INET;
      a.sin_addr.s_addr = INADDR_ANY;
      a.sin_port = htons (port);

      if (bind (sock, (struct sockaddr *) &a, sizeof (a)) >= 0)
	break;
    }
	
  return port < 1 << 16 ? port : -1;
}

/* Convert a config string to a struct sockaddr and length for use
   with bind or connect. */
static clib_error_t *
socket_config (char * config,
	       clib_socket_addr_t * addr,
	       socklen_t * addr_len,
	       u32 ip4_default_address)
{
  clib_error_t * error = 0;

  if (! config)
    config = "";

  /* Anything that begins with a / is a local PF_LOCAL socket. */
  if (config[0] == '/')
    {
      addr->un.sun_family = PF_LOCAL;
      memcpy (&addr->un.sun_path, config,
	      clib_min (sizeof (addr->un.sun_path), 1 + strlen (config)));
      *addr_len = sizeof (addr->un);
      return 0;
    }

  /* Hostname or hostname:port or port. */
  {
    char * host_name;
    int port = -1;
    struct sockaddr_in * sa = &addr->in;

    host_name = 0;
    port = -1;
    if (config[0] != 0)
      {
        unformat_input_t i;

        unformat_init_string (&i, config, strlen (config));
        if (unformat (&i, "%s:%d", &host_name, &port)
            || unformat (&i, "%s:0x%x", &host_name, &port))
          ;
        else if (unformat (&i, "%s", &host_name))
          ;
        else
          error = clib_error_return (0, "unknown input `%U'",
                                     format_unformat_error, &i);
        unformat_free (&i);

        if (error)
          goto done;
      }

    sa->sin_family = PF_INET;
    *addr_len = sizeof (sa[0]);
    if (port != -1)
      sa->sin_port = htons (port);
    else
      sa->sin_port = 0;

    if (host_name)
      {
        struct in_addr host_addr;

        /* Recognize localhost to avoid host lookup in most common cast. */
        if (! strcmp (host_name, "localhost"))
          sa->sin_addr.s_addr = htonl (INADDR_LOOPBACK);

        else if (inet_aton (host_name, &host_addr))
          sa->sin_addr = host_addr;

        else if (host_name && strlen (host_name) > 0)
          {
            struct hostent * host = gethostbyname (host_name);
            if (! host)
              error = clib_error_return (0, "unknown host `%s'", config);
            else
              memcpy (&sa->sin_addr.s_addr, host->h_addr_list[0], host->h_length);
          }

        else
          sa->sin_addr.s_addr = htonl (ip4_default_address);

        vec_free (host_name);
        if (error)
          goto done;
      }
  }

 done:
  return error;
}

void clib_socket_tx_add_formatted (clib_socket_t * s, char * fmt, ...)
{
  va_list va;
  va_start (va, fmt);
  clib_socket_tx_add_va_formatted (s, fmt, &va);
  va_end (va);
}

clib_error_t *
clib_socket_tx (clib_socket_t * s)
{
  clib_error_t	* error = 0;
  word written = 0;

  /* Gather buffers to send. */
  {
    u8 ** tbf;
    struct iovec * iov;

    if (vec_len (s->current_tx_buffer) > 0)
      {
        clib_fifo_add1 (s->tx_buffer_fifo, s->current_tx_buffer);
        s->current_tx_buffer = 0;
      }

    vec_reset_length (s->tx_buffer_iovecs);
    clib_fifo_foreach (tbf, s->tx_buffer_fifo, ({
      u8 * tb = tbf[0];
      clib_socket_buffer_header_t * tbh = vec_header (tb, sizeof (tbh[0]));
      vec_add2 (s->tx_buffer_iovecs, iov, 1);
      iov->iov_base = tb + tbh->offset;
      iov->iov_len = vec_len (tb) - tbh->offset;
    }));
  }

  written = writev (s->fd, s->tx_buffer_iovecs, vec_len (s->tx_buffer_iovecs));

  /* Ignore certain errors. */
  if (written < 0 && ! unix_error_is_fatal (errno))
    written = 0;

  /* A "real" error occurred. */
  if (written <= 0)
    {
      if (written < 0)
        error = clib_error_return_unix (0, "write");
      goto done;
    }

  /* Reclaim the transmitted part of the tx buffer on successful writes. */
  {
    uword n_left = written;
    u8 * b;
    clib_socket_buffer_header_t * bh;

    while (n_left > 0)
      {
        /* Get fifo head buffer. */
        b = * clib_fifo_elt_at_index (s->tx_buffer_fifo, 0);
        bh = vec_header (b, sizeof (bh[0]));
        ASSERT (bh->offset < vec_len (b));
        if (bh->offset + n_left < vec_len (b))
          {
            bh->offset += n_left;
            break;
          }
        else
          {
            n_left -= vec_len (b);
            clib_socket_buffer_free (b);
            clib_fifo_advance_head (s->tx_buffer_fifo, 1);
          }
      }
  }

 done:
  return error;
}

clib_error_t *
clib_socket_rx (clib_socket_t * s, int n_bytes)
{
  word fd, n_read;
  u8 * buf;

  /* RX side of socket is down once end of file is reached. */
  if (s->rx_end_of_file)
    return 0;

  fd = s->fd;

  n_bytes = clib_max (n_bytes, 4096);
  vec_add2 (s->rx_buffer, buf, n_bytes);

  if ((n_read = read (fd, buf, n_bytes)) < 0)
    {
      n_read = 0;

      /* Ignore certain errors. */
      if (! unix_error_is_fatal (errno))
	goto non_fatal;
      
      return clib_error_return_unix (0, "read %d bytes", n_bytes);
    }
      
  /* Other side closed the socket. */
  s->rx_end_of_file = n_read == 0;

 non_fatal:
  _vec_len (s->rx_buffer) += n_read - n_bytes;

  return 0;
}

clib_error_t * clib_socket_close (clib_socket_t * s)
{
  if (close (s->fd) < 0)
    return clib_error_return_unix (0, "close");
  return 0;
}

clib_error_t *
clib_socket_init (clib_socket_t * s)
{
  socklen_t addr_len = 0;
  clib_error_t * error = 0;
  word port;

  error = socket_config (s->config, &s->self_addr, &addr_len,
			 (s->is_server ? INADDR_LOOPBACK : INADDR_ANY));
  if (error)
    goto done;

  s->fd = socket (s->self_addr.sa.sa_family, SOCK_STREAM, 0);
  if (s->fd < 0)
    {
      error = clib_error_return_unix (0, "socket");
      goto done;
    }

  port = 0;
  if (s->self_addr.in.sin_family == PF_INET)
    port = s->self_addr.in.sin_port;

  if (s->is_server)
    {
      uword need_bind = 1;

      if (s->self_addr.in.sin_family == PF_INET)
	{
	  if (port == 0)
	    {
	      port = find_free_port (s->fd);
	      if (port < 0)
		{
		  error = clib_error_return (0, "no free port");
		  goto done;
		}
              s->self_addr.in.sin_port = clib_host_to_net_u16 (port);
	      need_bind = 0;
	    }
	}
      if (s->self_addr.un.sun_family == PF_LOCAL)
	unlink (s->self_addr.un.sun_path);

      /* Make address available for multiple users. */
      {
	int v = 1;
	if (setsockopt (s->fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof (v)) < 0)
	  clib_unix_warning ("setsockopt SO_REUSEADDR fails");
      }

      /* Disable Nagle. */
      {
        int v = s->tcp_delay ? 0 : 1;
        if (setsockopt (s->fd, IPPROTO_TCP, TCP_NODELAY, &v, sizeof (v)) < 0)
          clib_unix_warning ("setsockopt TCP_NODELAY fails");
      }

      if (need_bind
	  && bind (s->fd, &s->self_addr.sa, addr_len) < 0)
	{
	  error = clib_error_return_unix (0, "bind");
	  goto done;
	}

      if (listen (s->fd, SOMAXCONN) < 0)
	{
	  error = clib_error_return_unix (0, "listen");
	  goto done;
	}
    }
  else
    {
      if (s->non_blocking_connect && fcntl (s->fd, F_SETFL, O_NONBLOCK) < 0)
	{
	  error = clib_error_return_unix (0, "fcntl NONBLOCK");
	  goto done;
	}

      s->peer_addr = s->self_addr;

      if (connect (s->fd, &s->peer_addr.sa, addr_len) < 0 && ! s->non_blocking_connect)
	{
	  error = clib_error_return_unix (0, "connect");
	  goto done;
	}

      {
        socklen_t len = sizeof (s->self_addr.in);
        if (getsockname (s->fd, (struct sockaddr *) &s->self_addr.in, &len) < 0)
          {
            error = clib_error_return_unix (0, "getsockname");
            goto done;
          }
      }

      s->non_blocking_connect_in_progress = s->non_blocking_connect;
    }

 done:
  if (error)
    {
      if (s->fd > 0)
        close (s->fd);
    }
  else
    {
      if (! s->max_tx_buffer_bytes)
        s->max_tx_buffer_bytes = ~0;
    }
  return error;
}

clib_error_t * clib_socket_accept (clib_socket_t * server, clib_socket_t * client)
{
  clib_error_t * error = 0;
  socklen_t len = 0;
  
  memset (client, 0, sizeof (client[0]));

  /* Accept the new socket connection. */
  len = sizeof (client->peer_addr.in);
  client->fd = accept (server->fd, (struct sockaddr *) &client->peer_addr.in, &len);
  if (client->fd < 0) 
    return clib_error_return_unix (0, "accept");
  
  /* Set the new socket to be non-blocking. */
  if (fcntl (client->fd, F_SETFL, O_NONBLOCK) < 0)
    {
      error = clib_error_return_unix (0, "fcntl O_NONBLOCK");
      goto close_client;
    }
    
  client->self_addr = server->self_addr;
  client->is_client = 1;
  return 0;

 close_client:
  close (client->fd);
  return error;
}
