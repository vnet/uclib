#include <uclib/uclib.h>

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

typedef enum {
  TEST_SOCKET_TYPE_client,
  TEST_SOCKET_TYPE_server,
  TEST_SOCKET_TYPE_server_listen,
  TEST_SOCKET_N_TYPE,
} test_socket_type_t;

typedef struct {
  clib_socket_t clib_socket;

  unix_file_poller_file_t file_poller_file;

  test_socket_type_t type;

  u32 n_tx_msgs;
} test_socket_t;

typedef struct {
  unix_file_poller_t unix_file_poller;
  unix_file_poller_file_functions_t unix_file_poller_file_functions[TEST_SOCKET_N_TYPE];
  test_socket_t * socket_pool;
  u32 n_clients;
  u32 verbose;
  u32 max_tx_msgs;
  char * config;
} test_socket_main_t;

static clib_error_t *
socket_file_error_ready (unix_file_poller_file_functions_t * ff, u32 socket_type, u32 socket_index)
{
  clib_error_t * error = 0;
  ASSERT (0);
  return error;
}

static clib_error_t *
socket_server_file_read_ready (unix_file_poller_file_functions_t * ff, u32 socket_type, u32 socket_index)
{
  test_socket_main_t * tsm = CONTAINER_OF (ff, test_socket_main_t, unix_file_poller_file_functions[TEST_SOCKET_TYPE_server]);
  test_socket_t * ts = pool_elt_at_index (tsm->socket_pool, socket_index);
  clib_socket_t * s = &ts->clib_socket;
  clib_error_t * error = clib_socket_rx (s, 4096);

  if (error)
    return error;

  if (s->rx_end_of_file)
    {
      if (tsm->verbose)
        clib_warning ("close connection %U", format_sockaddr, &s->peer_addr);
      
      {
	unix_file_poller_update_t u = {
	  .type = UNIX_FILE_POLLER_UPDATE_DELETE,
	  .file_id = socket_index,
	  .file_type = TEST_SOCKET_TYPE_server,
	  .file_descriptor = s->fd,
	  .is_write_ready = 0,
	};
	tsm->unix_file_poller.update (&tsm->unix_file_poller, &u);
      }

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
socket_server_file_accept_on_read_ready (unix_file_poller_file_functions_t * ff, u32 socket_type, u32 socket_index)
{
  test_socket_main_t * tsm = CONTAINER_OF (ff, test_socket_main_t, unix_file_poller_file_functions[TEST_SOCKET_TYPE_server_listen]);
  test_socket_t * server_test_socket = pool_elt_at_index (tsm->socket_pool, socket_index);
  test_socket_t * client_test_socket;
  clib_socket_t * server_socket = &server_test_socket->clib_socket;
  clib_socket_t * client_socket;
  clib_error_t * error = 0;

  pool_get (tsm->socket_pool, client_test_socket);
  client_test_socket->type = TEST_SOCKET_TYPE_server;
  client_socket = &client_test_socket->clib_socket;

  error = clib_socket_accept (server_socket, client_socket);
  if (error)
    goto done;

  {
    unix_file_poller_update_t u = {
      .type = UNIX_FILE_POLLER_UPDATE_ADD,
      .file_descriptor = client_socket->fd,
      .file_id = client_test_socket - tsm->socket_pool,
      .file_type = TEST_SOCKET_TYPE_server,
      .is_write_ready = 0,
    };
    tsm->unix_file_poller.update (&tsm->unix_file_poller, &u);
  }

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
socket_client_file_read_ready (unix_file_poller_file_functions_t * ff, u32 socket_type, u32 socket_index)
{
  test_socket_main_t * tsm = CONTAINER_OF (ff, test_socket_main_t, unix_file_poller_file_functions[TEST_SOCKET_TYPE_client]);
  test_socket_t * ts = pool_elt_at_index (tsm->socket_pool, socket_index);
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

      {
	unix_file_poller_update_t u = {
	  .type = UNIX_FILE_POLLER_UPDATE_DELETE,
	  .file_descriptor = s->fd,
	  .file_id = ts - tsm->socket_pool,
	  .file_type = TEST_SOCKET_TYPE_client,
	  .is_write_ready = 0,
	};
	tsm->unix_file_poller.update (&tsm->unix_file_poller, &u);
      }

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
socket_client_file_write_ready (unix_file_poller_file_functions_t * ff, u32 socket_type, u32 socket_index)
{
  test_socket_main_t * tsm = CONTAINER_OF (ff, test_socket_main_t, unix_file_poller_file_functions[TEST_SOCKET_TYPE_client]);
  test_socket_t * ts = pool_elt_at_index (tsm->socket_pool, socket_index);
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
      unix_file_poller_update_t u = {
	.type = UNIX_FILE_POLLER_UPDATE_DELETE,
	.file_descriptor = s->fd,
	.file_id = ts - tsm->socket_pool,
	.file_type = TEST_SOCKET_TYPE_client,
	.is_write_ready = 0,
      };
      tsm->unix_file_poller.update (&tsm->unix_file_poller, &u);
      close (s->fd);
      clib_socket_free (s);
      pool_put (tsm->socket_pool, ts);
      return 0;
    }

  {
    unix_file_poller_update_t u = {
      .type = UNIX_FILE_POLLER_UPDATE_MODIFY,
      .file_descriptor = s->fd,
      .file_id = ts - tsm->socket_pool,
      .file_type = TEST_SOCKET_TYPE_client,
      .is_write_ready = clib_socket_tx_data_is_available_to_write (s),
    };

    unix_file_poller_set_data_available_to_write (&tsm->unix_file_poller,
						  &ts->file_poller_file,
						  &u);
  }

  return error;
}

static void add_file_type (test_socket_main_t * tsm, test_socket_type_t t, unix_file_poller_file_functions_t f)
{
  tsm->unix_file_poller_file_functions[t] = f;
  unix_file_poller_add_file_type (&tsm->unix_file_poller, t, &tsm->unix_file_poller_file_functions[t]);
}

int test_socket_main (unformat_input_t * input)
{
  test_socket_main_t tsm;
  clib_error_t * error = 0;
  unix_file_poller_t * fp = &tsm.unix_file_poller;

  memset (&tsm, 0, sizeof (tsm));
  tsm.config = "localhost";
  tsm.n_clients = 1;
  tsm.max_tx_msgs = 1;
  tsm.verbose = 1;

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat (input, "listen %s", &tsm.config))
        ;
      else if (unformat (input, "n-clients %d", &tsm.n_clients))
        ;
      else if (unformat (input, "n-msgs %d", &tsm.max_tx_msgs))
        ;
      else
        {
          clib_warning ("unknown input `%U'", format_unformat_error, input);
          return 1;
        }
    }

  error = unix_file_poller_init (fp);
  if (error)
    goto done;

  {
    test_socket_t * ts;
    clib_socket_t * s;
    int i;

    pool_get (tsm.socket_pool, ts);
    ts->type = TEST_SOCKET_TYPE_server_listen;
    s = &ts->clib_socket;

    s->is_server = 1;
    s->config = tsm.config;
    error = clib_socket_init (s);
    if (error)
      goto done;

    if (tsm.verbose)
      clib_warning ("listening %U", format_sockaddr, &s->self_addr);

    {
      unix_file_poller_update_t u = {
	.type = UNIX_FILE_POLLER_UPDATE_ADD,
	.file_descriptor = s->fd,
	.file_id = ts - tsm.socket_pool,
	.file_type = TEST_SOCKET_TYPE_server_listen,
	.is_write_ready = 0,
      };
      tsm.unix_file_poller.update (fp, &u);
    }

    for (i = 0; i < tsm.n_clients; i++)
      {
        pool_get (tsm.socket_pool, ts);
        s = &ts->clib_socket;
        s->is_client = 1;
        s->config = (char *) format (0, "%U%c", format_sockaddr, &tsm.socket_pool[0].clib_socket.self_addr, 0);
        error = clib_socket_init (s);
        if (error)
          goto done;
        
	{
	  unix_file_poller_update_t u = {
	    .type = UNIX_FILE_POLLER_UPDATE_ADD,
	    .file_descriptor = s->fd,
	    .file_id = ts - tsm.socket_pool,
	    .file_type = TEST_SOCKET_TYPE_client,
	    .is_write_ready = 1,
	  };
	  tsm.unix_file_poller.update (fp, &u);
	}
      }
  }

  {
    unix_file_poller_file_functions_t f = {
      .read_function = socket_client_file_read_ready,
      .write_function = socket_client_file_write_ready,
      .error_function = socket_file_error_ready,
    };
    add_file_type (&tsm, TEST_SOCKET_TYPE_client, f);
  }

  {
    unix_file_poller_file_functions_t f = {
      .read_function = socket_server_file_read_ready,
      .write_function = 0,
      .error_function = socket_file_error_ready,
    };
    add_file_type (&tsm, TEST_SOCKET_TYPE_server, f);
  }

  {
    unix_file_poller_file_functions_t f = {
      .read_function = socket_server_file_accept_on_read_ready,
      .write_function = 0,
      .error_function = socket_file_error_ready,
    };
    add_file_type (&tsm, TEST_SOCKET_TYPE_server_listen, f);
  }

  while (pool_elts (tsm.socket_pool) > 1)
    tsm.unix_file_poller.poll_for_input (fp, 10e-3);

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

