/*
 * unix_file_poller.h: Unix file descriptor polling
 *
 * Copyright (c) 2014 Eliot Dresselhaus
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef included_unix_file_poller_h
#define included_unix_file_poller_h

struct unix_file_poller_file;
typedef clib_error_t * (unix_file_poller_file_function_t) (struct unix_file_poller_file * f);

typedef struct unix_file_poller_file {
  /* Unix file descriptor from open/socket. */
  int file_descriptor;

  u32 flags;
#define UNIX_FILE_POLLER_FILE_DATA_AVAILABLE_TO_WRITE (1 << 0)

  /* Functions to be called when read/write data becomes ready. */
  unix_file_poller_file_function_t * read_function, * write_function, * error_function;

  /* Opaque data available for function's use. */
  uword private_data[2];
} unix_file_poller_file_t;

typedef struct {
  f64 time;
  clib_error_t * error;
} unix_error_history_t;

typedef enum {
  UNIX_FILE_POLLER_FILE_UPDATE_ADD,
  UNIX_FILE_POLLER_FILE_UPDATE_MODIFY,
  UNIX_FILE_POLLER_FILE_UPDATE_DELETE,
} unix_file_poller_file_update_type_t;

typedef struct unix_file_poller {
  /* Pool of files to poll for input/output. */
  unix_file_poller_file_t * file_pool;

  /* File descriptor update function (e.g. epoll for linux; kqueue for *BSD). */
  void (* file_update) (struct unix_file_poller * poller,
                        unix_file_poller_file_t * file,
                        unix_file_poller_file_update_type_t update_type);

  /* Poll file desciptors for input or output: returns number of files polled. */
  uword (* poll_for_input) (struct unix_file_poller * poller, f64 timeout_in_sec);

  /* Circular buffer of last unix errors. */
  unix_error_history_t error_history[128];
  u32 error_history_index;
  u64 n_total_errors;
} unix_file_poller_t;

always_inline void
unix_file_poller_free (unix_file_poller_t * um)
{
  uword i;
  for (i = 0; i < ARRAY_LEN (um->error_history); i++)
    if (um->error_history[i].error)
      clib_error_free (um->error_history[i].error);
}

always_inline uword
unix_file_poller_add_file (unix_file_poller_t * um, unix_file_poller_file_t * template)
{
  unix_file_poller_file_t * f;
  pool_get (um->file_pool, f);
  f[0] = template[0];
  um->file_update (um, f, UNIX_FILE_POLLER_FILE_UPDATE_ADD);
  return f - um->file_pool;
}

always_inline void
unix_file_poller_del_file (unix_file_poller_t * um, unix_file_poller_file_t * f)
{
  um->file_update (um, f, UNIX_FILE_POLLER_FILE_UPDATE_DELETE);
  close (f->file_descriptor);
  f->file_descriptor = ~0;
  pool_put (um->file_pool, f);
}

always_inline unix_file_poller_file_t *
unix_file_poller_get_file (unix_file_poller_t * um, u32 i)
{ return pool_elt_at_index (um->file_pool, i); }

always_inline uword
unix_file_poller_set_data_available_to_write (unix_file_poller_t * um,
                                              u32 unix_file_poller_file_index,
                                              uword is_available)
{
  unix_file_poller_file_t * uf = pool_elt_at_index (um->file_pool, unix_file_poller_file_index);
  uword was_available = (uf->flags & UNIX_FILE_POLLER_FILE_DATA_AVAILABLE_TO_WRITE);
  if ((was_available != 0) != (is_available != 0))
    {
      uf->flags ^= UNIX_FILE_POLLER_FILE_DATA_AVAILABLE_TO_WRITE;
      um->file_update (um, uf, UNIX_FILE_POLLER_FILE_UPDATE_MODIFY);
    }
  return was_available != 0;
}

always_inline void
unix_save_error (unix_file_poller_t * um, clib_error_t * error)
{
  unix_error_history_t * eh = um->error_history + um->error_history_index;
  clib_error_free_vector (eh->error);
  eh->error = error;
  eh->time = unix_time_now ();
  um->n_total_errors += 1;
  if (++um->error_history_index >= ARRAY_LEN (um->error_history))
    um->error_history_index = 0;
}

clib_error_t *
unix_file_poller_init (unix_file_poller_t * um);

#endif /* included_unix_file_poller_file_poller_h */
