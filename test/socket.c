#include <uclib/uclib.h>
#include <uclib/uclib.c>

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

#include <uclib/unix_file_poller.h>

typedef struct {
  clib_socket_t clib_socket;

  u32 n_tx_msgs;
} test_socket_t;

typedef struct {
  unix_file_poller_t unix_file_poller;
  test_socket_t * socket_pool;
  u32 is_server;
  u32 verbose;
  u32 max_tx_msgs;
  char * config;
} test_socket_main_t;

static clib_error_t *
socket_server_file_error (unix_file_poller_file_t * f)
{
  clib_error_t * error = 0;
  ASSERT (0);
  return error;
}

static clib_error_t *
socket_server_file_read_ready (unix_file_poller_file_t * f)
{
  test_socket_main_t * tsm = uword_to_pointer (f->private_data[0], test_socket_main_t *);
  test_socket_t * ts = pool_elt_at_index (tsm->socket_pool, f->private_data[1]);
  clib_socket_t * s = &ts->clib_socket;
  clib_error_t * error = clib_socket_rx (s, 4096);

  if (error)
    return error;

  if (s->rx_end_of_file)
    {
      if (tsm->verbose)
        clib_warning ("close connection %U", format_sockaddr, &s->peer_addr);
      unix_file_poller_del_file (&tsm->unix_file_poller, f);
      clib_socket_free (s);
      pool_put (tsm->socket_pool, ts);
    }
  else
    {
      if (tsm->verbose)
        clib_warning ("`%v' %d bytes", s->rx_buffer, vec_len (s->rx_buffer));
      vec_reset_length (s->rx_buffer);
    }

  return error;
}

static clib_error_t *
socket_server_file_accept_on_read_ready (unix_file_poller_file_t * f)
{
  test_socket_main_t * tsm = uword_to_pointer (f->private_data[0], test_socket_main_t *);
  test_socket_t * server_test_socket = pool_elt_at_index (tsm->socket_pool, f->private_data[1]);
  test_socket_t * client_test_socket;
  clib_socket_t * server_socket = &server_test_socket->clib_socket;
  clib_socket_t * client_socket;
  unix_file_poller_file_t client_file;
  clib_error_t * error = 0;

  pool_get (tsm->socket_pool, client_test_socket);
  client_socket = &client_test_socket->clib_socket;

  error = clib_socket_accept (server_socket, client_socket);
  if (error)
    goto done;

  memset (&client_file, 0, sizeof (client_file));
  client_file.private_data[0] = pointer_to_uword (tsm);
  client_file.private_data[1] = client_test_socket - tsm->socket_pool;
  client_file.file_descriptor = client_socket->fd;
  client_file.read_function = socket_server_file_read_ready;

  unix_file_poller_add_file (&tsm->unix_file_poller, &client_file);

  if (tsm->verbose)
    clib_warning ("new connection %U", format_sockaddr, &client_socket->peer_addr);

 done:
  if (error)
    {
      clib_socket_free (client_socket);
      pool_put (tsm->socket_pool, client_test_socket);
    }
  return error;
}

static clib_error_t *
socket_client_file_read_ready (unix_file_poller_file_t * f)
{
  test_socket_main_t * tsm = uword_to_pointer (f->private_data[0], test_socket_main_t *);
  test_socket_t * ts = pool_elt_at_index (tsm->socket_pool, f->private_data[1]);
  clib_socket_t * s = &ts->clib_socket;
  clib_error_t * error;

  ASSERT (0);

  error = clib_socket_rx (s, 4096);
  if (error)
    return error;

  if (s->rx_end_of_file)
    {
      if (tsm->verbose)
        clib_warning ("close connection %U", format_sockaddr, &s->peer_addr);
      unix_file_poller_del_file (&tsm->unix_file_poller, f);
      clib_socket_free (s);
      pool_put (tsm->socket_pool, ts);
    }
  else
    {
      if (tsm->verbose)
        clib_warning ("`%v' %d bytes", s->rx_buffer, vec_len (s->rx_buffer));
      vec_reset_length (s->rx_buffer);
    }

  return error;
}

static clib_error_t *
socket_client_file_write_ready (unix_file_poller_file_t * f)
{
  test_socket_main_t * tsm = uword_to_pointer (f->private_data[0], test_socket_main_t *);
  test_socket_t * ts = pool_elt_at_index (tsm->socket_pool, f->private_data[1]);
  clib_socket_t * s = &ts->clib_socket;
  clib_error_t * error = 0;

  if (s->non_blocking_connect_in_progress)
    {
      error = clib_socket_non_blocking_connect_status (s);
      if (error)
        return error;
    }
  else
    {
      error = clib_socket_tx (s);
      if (error && unix_error_is_fatal (errno))
        return error;
    }

  if (ts->n_tx_msgs < tsm->max_tx_msgs)
    {
      clib_socket_tx_add_formatted (s, "foo %U %d", format_sockaddr, &s->self_addr, ts->n_tx_msgs);
      ts->n_tx_msgs++;
    }
  else
    {
      unix_file_poller_del_file (&tsm->unix_file_poller, f);
      clib_socket_free (s);
      pool_put (tsm->socket_pool, ts);
      return 0;
    }

  unix_file_poller_set_data_available_to_write (&tsm->unix_file_poller,
                                                f - tsm->unix_file_poller.file_pool,
                                                vec_len (s->tx_buffer) > 0);

  return error;
}

int test_socket_main (unformat_input_t * input)
{
  test_socket_main_t tsm;
  clib_error_t * error = 0;

  memset (&tsm, 0, sizeof (tsm));
  tsm.config = "localhost";
  tsm.is_server = 1;
  tsm.max_tx_msgs = 100;
  tsm.verbose = 1;

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat (input, "listen %s", &tsm.config))
        tsm.is_server = 1;
      else if (unformat (input, "connect %s", &tsm.config))
        tsm.is_server = 0;
      else
        {
          clib_warning ("unknown input `%U'", format_unformat_error, input);
          return 1;
        }
    }

  {
    test_socket_t * ts;
    clib_socket_t * s;
    unix_file_poller_file_t pf;

    pool_get (tsm.socket_pool, ts);
    s = &ts->clib_socket;

    s->is_server = tsm.is_server;
    s->is_client = ! tsm.is_server;
    s->config = tsm.config;

    error = clib_socket_init (s);
    if (error)
      goto done;

    if (tsm.verbose)
      clib_warning ("%s %U",
                    tsm.is_server ? "listening" : "connecting",
                    format_sockaddr, &s->self_addr);

    unix_file_poller_init (&tsm.unix_file_poller);

    memset (&pf, 0, sizeof (pf));
    pf.file_descriptor = s->fd;
    pf.private_data[0] = pointer_to_uword (&tsm);
    pf.private_data[1] = ts - tsm.socket_pool;

    if (tsm.is_server)
      {
        pf.read_function = socket_server_file_accept_on_read_ready;
        pf.error_function = socket_server_file_error;
      }
    else
      {
        pf.read_function = socket_client_file_read_ready;
        pf.write_function = socket_client_file_write_ready;
        pf.error_function = socket_server_file_error;
        pf.flags = UNIX_FILE_POLLER_FILE_DATA_AVAILABLE_TO_WRITE;
      }

    unix_file_poller_add_file (&tsm.unix_file_poller, &pf);
  }

  while (pool_elts (tsm.unix_file_poller.file_pool) > 0)
    {
      tsm.unix_file_poller.poll_for_input (&tsm.unix_file_poller, 10e-3);
    }

 done:
  if (error)
    clib_error_report (error);
  return error ? 1 : 0;
}

int main (int argc, char * argv[])
{
  unformat_input_t i;
  int ret;

  unformat_init_command_line (&i, argv);
  ret = test_socket_main (&i);
  unformat_free (&i);

  return ret;
}

