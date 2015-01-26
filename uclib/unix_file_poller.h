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

struct unix_file_poller_t;
struct unix_file_poller_file_functions_t;

typedef clib_error_t * (unix_file_poller_file_function_t) (struct unix_file_poller_file_functions_t * ff, u32 file_id);

/* Functions to be called when read/write data becomes ready. */
typedef struct unix_file_poller_file_functions_t {
  unix_file_poller_file_function_t * read_function, * write_function, * error_function;
} unix_file_poller_file_functions_t;

typedef struct {
  f64 time;
  clib_error_t * error;
} unix_error_history_t;

typedef struct {
  enum {
    UNIX_FILE_POLLER_UPDATE_ADD,
    UNIX_FILE_POLLER_UPDATE_MODIFY,
    UNIX_FILE_POLLER_UPDATE_DELETE,
  } type;
  int file_descriptor;
  u32 file_type;
  u32 file_id;
  u32 is_write_ready;
} unix_file_poller_update_t;

typedef struct unix_file_poller_t {
  /* File descriptor update function (e.g. epoll for linux; kqueue for *BSD). */
  void (* update) (struct unix_file_poller_t * fp, unix_file_poller_update_t * update);

  void (* free) (struct unix_file_poller_t * fp);

  /* Linux/BSD dependencies are here. */
  void * os_opaque;

  /* Poll file desciptors for input or output: returns number of files polled. */
  uword (* poll_for_input) (struct unix_file_poller_t * fp, f64 timeout_in_sec);

  /* Circular buffer of last unix errors. */
  unix_error_history_t error_history[128];
  u32 error_history_index;
  u64 n_total_errors;
} unix_file_poller_t;

/* Pool of read/write/error handler functions by file type. */
unix_file_poller_file_functions_t ** unix_file_poller_file_function_pool;

always_inline uword
unix_file_poller_register_file_functions (unix_file_poller_file_functions_t * f)
{ return pool_set_elt (unix_file_poller_file_function_pool, f); }

always_inline void
unix_file_poller_free (unix_file_poller_t * fp)
{
  uword i;
  if (fp->free)
    fp->free (fp);
  for (i = 0; i < ARRAY_LEN (fp->error_history); i++)
    if (fp->error_history[i].error)
      clib_error_free (fp->error_history[i].error);
}

always_inline void
unix_save_error (unix_file_poller_t * fp, clib_error_t * error)
{
  unix_error_history_t * eh = fp->error_history + fp->error_history_index;
  clib_error_free_vector (eh->error);
  eh->error = error;
  eh->time = unix_time_now ();
  fp->n_total_errors += 1;
  if (++fp->error_history_index >= ARRAY_LEN (fp->error_history))
    fp->error_history_index = 0;
}

clib_error_t *
unix_file_poller_init (unix_file_poller_t * fp);

#endif /* included_unix_file_poller_file_poller_h */
