#include <uclib/uclib.h>

typedef struct {
  websocket_socket_t websocket_socket;
  u64 n_msgs_sent;
  u64 n_msgs_received;
  f64 done_time;
} test_websocket_socket_t;

typedef struct {
  websocket_main_t websocket_main;
  unix_file_poller_t unix_file_poller;
  u32 n_clients;
  u64 n_msgs_to_send;
  u64 n_msgs_rx_total;
  char * config;
  u32 verbose;
  u32 mem_trace;
  u32 n_close;
} test_websocket_main_t;

static clib_error_t *
test_websocket_rx_frame_payload (websocket_main_t * wsm, websocket_socket_t * ws, u8 * rx_payload, u32 n_payload_bytes)
{
  test_websocket_main_t * tsm = CONTAINER_OF (wsm, test_websocket_main_t, websocket_main);
  test_websocket_socket_t * tws = CONTAINER_OF (ws, test_websocket_socket_t, websocket_socket);

  if (tsm->verbose)
    clib_warning ("%s %d: %*s",
                  ws->is_server_client ? "client -> server" : "server -> client",
		  ws->index,
                  n_payload_bytes, rx_payload);

  tsm->n_msgs_rx_total += 1;
  tws->n_msgs_received += 1;
  return 0;
}

static void
test_websocket_connection_will_close (websocket_main_t * wsm, websocket_socket_t * ws, clib_error_t * error_reason)
{
  test_websocket_main_t * tsm = CONTAINER_OF (wsm, test_websocket_main_t, websocket_main);
  test_websocket_socket_t * tws = CONTAINER_OF (ws, test_websocket_socket_t, websocket_socket);

  if (tsm->verbose)
    {
      if (error_reason)
        clib_warning ("closing reason: %U", format_clib_error, error_reason);
      else
        clib_warning ("closing end-of-file");
    }

  tsm->n_close++;

  if (tws->n_msgs_received != tws->n_msgs_sent || tws->n_msgs_sent != tsm->n_msgs_to_send)
    {
      clib_warning ("close %d rx %Ld %U %d, %Ld msgs rx; %Ld msgs sent",
		    tsm->n_close, tsm->n_msgs_rx_total,
		    format_websocket_connection_type, websocket_connection_type (ws),
		    ws->index,
		    tws->n_msgs_received, tws->n_msgs_sent);
      if (tsm->verbose)
	clib_warning ("%U", format_clib_mem_usage, /* verbose */ 0);
    }
}

static clib_error_t *
test_websocket_did_receive_handshake (websocket_main_t * wsm, websocket_socket_t * ws)
{
  test_websocket_main_t * tsm = CONTAINER_OF (wsm, test_websocket_main_t, websocket_main);
  http_request_or_response_t * r = &ws->server.http_handshake_request;
  if (tsm->verbose && 0)
    clib_warning ("request: %U", format_http_request, r);
  return 0;
}

int test_websocket_main (unformat_input_t * input)
{
  test_websocket_main_t tsm;
  websocket_main_t * wsm;
  clib_error_t * error = 0;

  memset (&tsm, 0, sizeof (tsm));
  tsm.config = "localhost";
  tsm.n_clients = 1;
  tsm.n_msgs_to_send = 100;

  wsm = &tsm.websocket_main;
  wsm->verbose = 0;

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat (input, "listen %s", &tsm.config))
        ;
      else if (unformat (input, "n-clients %d", &tsm.n_clients))
        ;
      else if (unformat (input, "n-msg %d", &tsm.n_msgs_to_send))
        ;
      else if (unformat (input, "verbose"))
        {
          wsm->verbose = 1;
          tsm.verbose = 1;
        }
      else if (unformat (input, "mem-trace"))
	tsm.mem_trace = 1;
      else
        {
          clib_warning ("unknown input `%U'", format_unformat_error, input);
          return 1;
        }
    }

  clib_warning ("%U", format_clib_mem_usage, /* verbose */ 0);
  if (tsm.mem_trace) clib_mem_trace (1);

  wsm->unix_file_poller = &tsm.unix_file_poller;
  wsm->rx_frame_payload = test_websocket_rx_frame_payload;
  wsm->connection_will_close = test_websocket_connection_will_close;
  wsm->did_receive_handshake = test_websocket_did_receive_handshake;

  wsm->user_socket_n_bytes = sizeof (test_websocket_socket_t);
  wsm->user_socket_offset_of_websocket = STRUCT_OFFSET_OF (test_websocket_socket_t, websocket_socket);

  error = websocket_init (wsm);
  if (error)
    goto done;

  {
    websocket_socket_t * ws;
    int i;
    char * client_socket_config;
    char * host = "foo.bar.com";

    if (0)
      websocket_server_add_host (wsm, host);

    error = websocket_server_add_listener (wsm, tsm.config, &ws);
    if (error)
      goto done;

    client_socket_config =
      (char *) format (0, "%U%c", format_sockaddr, &ws->clib_socket.self_addr, 0);

    for (i = 0; i < tsm.n_clients; i++)
      {
	websocket_socket_t * ws;
        test_websocket_socket_t * tws;

        error = websocket_client_add_connection (wsm, &ws, "ws://%s/path/to?a=10&b=20", client_socket_config);
        if (error)
          goto done;

	tws = CONTAINER_OF (ws, test_websocket_socket_t, websocket_socket);
        tws->n_msgs_sent = 0;
      }
  }

  {
    f64 last_print_time = unix_time_now ();
    f64 last_scan_time = last_print_time;

    while (pool_elts (wsm->user_socket_pool) > 1)
      {
        f64 now;
	uword i;

	while (1)
	  {
	    uword n_input = tsm.unix_file_poller.poll_for_input (&tsm.unix_file_poller, 10e-3);
	    if (n_input == 0)
	      break;
	  }

        now = unix_time_now ();

        if (0 && now - last_print_time > 1)
          {
            clib_warning ("%U", format_clib_mem_usage, /* verbose */ 0);
            last_print_time += 1;
          }

        if (now - last_scan_time > 5)
          websocket_close_all_sockets_with_no_handshake (wsm);

	vec_foreach_index (i, wsm->user_socket_pool)
	  {
	    test_websocket_socket_t * tws = ((test_websocket_socket_t *) wsm->user_socket_pool) + i;
	    websocket_socket_t * ws = &tws->websocket_socket;

	    if (pool_is_free_index (wsm->user_socket_pool, i)
		|| websocket_connection_type (ws) != WEBSOCKET_CONNECTION_TYPE_client
		|| ! ws->handshake_rx)
	      continue;

	    if (tws->n_msgs_sent >= tsm.n_msgs_to_send)
	      {
		if (now - tws->done_time > 1)
		  websocket_close (wsm, ws);
	      }
	    else
	      {
		tws->n_msgs_sent++;
		clib_socket_tx_add_formatted (&ws->clib_socket, "client %d msg %d", ws->index, tws->n_msgs_sent);
		websocket_socket_tx_binary_frame (ws);
		if (tws->n_msgs_sent >= tsm.n_msgs_to_send)
		  tws->done_time = now;
	      }
	  }
      }
  }

  unix_file_poller_free (&tsm.unix_file_poller);
  websocket_main_free (wsm);

  clib_warning ("%U", format_clib_mem_usage, /* verbose */ 0);

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
  ret = test_websocket_main (&i);
  unformat_free (&i);

  return ret;
}
