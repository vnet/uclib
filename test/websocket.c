#include <uclib/uclib.h>
#include <uclib/uclib.c>

typedef struct {
  u32 websocket_socket_index;
  u64 n_msgs_sent;
  u64 n_msgs_received;
  f64 done_time;
} test_websocket_socket_t;

typedef struct {
  websocket_main_t websocket_main;
  unix_file_poller_t unix_file_poller;
  test_websocket_socket_t * test_socket_pool;
  u32 n_clients;
  u64 n_msgs_to_send;
  char * config;
  u32 verbose;
  u32 is_echo;
} test_websocket_main_t;

static clib_error_t *
test_websocket_rx_frame_payload (websocket_main_t * wsm, websocket_socket_t * ws, u8 * rx_payload, u32 n_payload_bytes)
{
  test_websocket_main_t * tsm = uword_to_pointer (ws->opaque[0], test_websocket_main_t *);
  test_websocket_socket_t * tws = pool_elt_at_index (tsm->test_socket_pool, ws->opaque[1]);

  if (tsm->verbose)
    clib_warning ("%s: %*s",
                  ws->is_server_client ? "client -> server" : "server -> client",
                  n_payload_bytes, rx_payload);

  if (tsm->is_echo)
    {
      clib_socket_t * s = &ws->clib_socket;
      clib_socket_tx_add (s, rx_payload, n_payload_bytes);
      websocket_socket_tx_text_frame (ws);
    }
  else
    {
    }
  tws->n_msgs_received += 1;
  return 0;
}

static void
test_websocket_new_client_for_server (websocket_main_t * wsm, u32 client_ws_index, u32 server_ws_index)
{
  websocket_socket_t * cws = pool_elt_at_index (wsm->socket_pool, client_ws_index);
  test_websocket_main_t * tsm = uword_to_pointer (cws->opaque[0], test_websocket_main_t *);
  test_websocket_socket_t * tws;

  pool_get (tsm->test_socket_pool, tws);
  memset (tws, 0, sizeof (tws[0]));
  tws->n_msgs_sent = 0;
  tws->websocket_socket_index = client_ws_index;

  cws->opaque[1] = tws - tsm->test_socket_pool;
}

static void
test_websocket_connection_will_close (websocket_main_t * wsm, u32 ws_index, clib_error_t * error_reason)
{
  websocket_socket_t * ws = pool_elt_at_index (wsm->socket_pool, ws_index);
  test_websocket_main_t * tsm = uword_to_pointer (ws->opaque[0], test_websocket_main_t *);
  test_websocket_socket_t * tws = pool_elt_at_index (tsm->test_socket_pool, ws->opaque[1]);
  pool_put (tsm->test_socket_pool, tws);
  ws->opaque[1] = ~0;

  if (tsm->verbose)
    {
      if (error_reason)
        clib_warning ("closing reason: %U", format_clib_error, error_reason);
      else
        clib_warning ("closing end-of-file");
    }
  if (tws->n_msgs_received != tws->n_msgs_sent || tws->n_msgs_sent != tsm->n_msgs_to_send)
    {
      clib_warning ("%Ld msgs rx; %Ld msgs sent", tws->n_msgs_received, tws->n_msgs_sent);
      clib_warning ("%U", format_clib_mem_usage, /* verbose */ 0);
    }
}

static void
test_websocket_did_receive_handshake (websocket_main_t * wsm, u32 ws_index)
{
  websocket_socket_t * ws = pool_elt_at_index (wsm->socket_pool, ws_index);
  http_request_or_response_t * r = &ws->server.http_handshake_request;
  clib_warning ("request: %U", format_http_request, r);
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
  tsm.is_echo = 0;

  wsm = &tsm.websocket_main;
  wsm->verbose = 0;

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat (input, "listen %s", &tsm.config))
        ;
      else if (unformat (input, "n-clients %d", &tsm.n_clients))
        ;
      else if (unformat (input, "max-msg %d", &tsm.n_msgs_to_send))
        ;
      else if (unformat (input, "verbose"))
        {
          wsm->verbose = 1;
          tsm.verbose = 1;
        }
      else if (unformat (input, "echo"))
        {
          tsm.is_echo = 1;
        }
      else
        {
          clib_warning ("unknown input `%U'", format_unformat_error, input);
          return 1;
        }
    }

  if (tsm.is_echo)
    tsm.n_clients = 0;

  clib_warning ("%U", format_clib_mem_usage, /* verbose */ 0);
  clib_mem_trace (1);

  wsm->unix_file_poller = &tsm.unix_file_poller;
  wsm->rx_frame_payload = test_websocket_rx_frame_payload;
  wsm->new_client_for_server = test_websocket_new_client_for_server;
  wsm->connection_will_close = test_websocket_connection_will_close;
  wsm->did_receive_handshake = test_websocket_did_receive_handshake;

  error = websocket_init (wsm);
  if (error)
    goto done;

  {
    websocket_socket_t * ws;
    int i;
    u32 listen_ws_index;
    char * client_socket_config;
    char * host = "foo.bar.com";
    char * client_url = (char *) format (0, "ws://%s/path/to?a=10&b=20", host);

    if (0)
      websocket_server_add_host (wsm, host);

    error = websocket_server_add_listener (wsm, tsm.config, &listen_ws_index);
    if (error)
      goto done;

    ws = pool_elt_at_index (wsm->socket_pool, listen_ws_index);
    ws->opaque[0] = pointer_to_uword (&tsm);
    ws->opaque[1] = ~0;

    client_socket_config =
      (char *) format (0, "%U%c", format_sockaddr, &ws->clib_socket.self_addr, 0);

    for (i = 0; i < tsm.n_clients; i++)
      {
        u32 client_ws_index;
        test_websocket_socket_t * tws;

        error = websocket_client_add_connection (wsm, client_socket_config, client_url, &client_ws_index);
        if (error)
          goto done;

        pool_get (tsm.test_socket_pool, tws);
        memset (tws, 0, sizeof (tws[0]));
        tws->n_msgs_sent = 0;
        tws->websocket_socket_index = client_ws_index;

        ws = pool_elt_at_index (wsm->socket_pool, client_ws_index);
        ws->opaque[0] = pointer_to_uword (&tsm);
        ws->opaque[1] = tws - tsm.test_socket_pool;
      }
  }

  {
    websocket_socket_t * ws;
    clib_socket_t * s;
    f64 last_print_time = unix_time_now ();
    f64 last_scan_time = last_print_time;

    while (pool_elts (tsm.unix_file_poller.file_pool) > (tsm.is_echo ? 0 : 1))
      {
        test_websocket_socket_t * tws;
        f64 now;

        tsm.unix_file_poller.poll_for_input (&tsm.unix_file_poller, 10e-3);

        now = unix_time_now ();

        if (0 && now - last_print_time > 1)
          {
            clib_warning ("%U", format_clib_mem_usage, /* verbose */ 0);
            last_print_time += 1;
          }

        if (now - last_scan_time > 5)
          websocket_close_all_sockets_with_no_handshake (wsm);

        if (! tsm.is_echo)
          {
            pool_foreach (ws, wsm->socket_pool, ({
              s = &ws->clib_socket;
              if (! (s->is_client && ws->handshake_rx))
                continue;
              tws = pool_elt_at_index (tsm.test_socket_pool, ws->opaque[1]);
              if (tws->n_msgs_sent >= tsm.n_msgs_to_send)
                {
                  if (now - tws->done_time > .25)
                    websocket_close (wsm, ws - wsm->socket_pool);
                }
              else
                {
                  tws->n_msgs_sent++;
                  clib_socket_tx_add_formatted (s, "index %d msg %d", ws - wsm->socket_pool, tws->n_msgs_sent);
                  websocket_socket_tx_binary_frame (ws);
                  if (tws->n_msgs_sent >= tsm.n_msgs_to_send)
                    tws->done_time = now;
                }
            }));
          }
      }
  }

  unix_file_poller_free (&tsm.unix_file_poller);

  clib_warning ("%U", format_clib_mem_usage, /* verbose */ 1);

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
