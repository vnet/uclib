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

#include <uclib/unix_file_poller.h>

#ifdef __linux__

#include <sys/epoll.h>

typedef struct {
  int epoll_fd;
  struct epoll_event * epoll_events;

  /* Statistics. */
  u64 epoll_files_ready;
  u64 epoll_waits;
} linux_epoll_main_t;

static linux_epoll_main_t linux_epoll_main;

static void
linux_epoll_file_update (unix_file_poller_t * um,
                         unix_file_poller_file_t * f,
			 unix_file_poller_file_update_type_t update_type)
{
  linux_epoll_main_t * em = &linux_epoll_main;
  struct epoll_event e;

  memset (&e, 0, sizeof (e));

  e.events = EPOLLIN;
  if (f->flags & UNIX_FILE_POLLER_FILE_DATA_AVAILABLE_TO_WRITE)
    e.events |= EPOLLOUT;
  e.data.u32 = f - um->file_pool;

  if (epoll_ctl (em->epoll_fd,
		 (update_type == UNIX_FILE_POLLER_FILE_UPDATE_ADD
		  ? EPOLL_CTL_ADD
		  : (update_type == UNIX_FILE_POLLER_FILE_UPDATE_MODIFY
		     ? EPOLL_CTL_MOD
		     : EPOLL_CTL_DEL)),
		 f->file_descriptor,
		 &e) < 0)
    clib_unix_warning ("epoll_ctl");
}

static uword
linux_epoll_input (unix_file_poller_t * um, f64 timeout_in_sec)
{
  linux_epoll_main_t * em = &linux_epoll_main;
  struct epoll_event * e;
  int n_fds_ready;

  /* Allow any signal to wakeup our sleep. */
  {
    static sigset_t unblock_all_signals;
    n_fds_ready = epoll_pwait (em->epoll_fd,
                               em->epoll_events,
                               vec_len (em->epoll_events),
                               (int) (timeout_in_sec * 1e3),
                               &unblock_all_signals);
  }

  if (n_fds_ready < 0)
    {
      if (unix_error_is_fatal (errno))
        clib_unix_error ("epoll_wait");

      /* non fatal error (e.g. EINTR). */
      return 0;
    }

  em->epoll_waits += 1;
  em->epoll_files_ready += n_fds_ready;

  for (e = em->epoll_events; e < em->epoll_events + n_fds_ready; e++)
    {
      u32 i = e->data.u32;
      unix_file_poller_file_t * f = pool_elt_at_index (um->file_pool, i);
      clib_error_t * errors[4];
      int n_errors = 0;

      if (PREDICT_TRUE (! (e->events & EPOLLERR)))
	{
	  if (e->events & EPOLLIN)
	    {
	      errors[n_errors] = f->read_function (f);
	      n_errors += errors[n_errors] != 0;
	    }
	  if (e->events & EPOLLOUT)
	    {
	      errors[n_errors] = f->write_function (f);
	      n_errors += errors[n_errors] != 0;
	    }
	}
      else
	{
	  errors[n_errors] = f->error_function (f);
	  n_errors += errors[n_errors] != 0;
	}

      ASSERT (n_errors < ARRAY_LEN (errors));
      for (i = 0; i < n_errors; i++)
	{
	  unix_save_error (um, errors[i]);
	}
    }

  return n_fds_ready;
}

clib_error_t *
unix_file_poller_init (unix_file_poller_t * um)
{
  linux_epoll_main_t * em = &linux_epoll_main;
  
  /* Allocate some events. */
  vec_resize (em->epoll_events, 256);

  em->epoll_fd = epoll_create (vec_len (em->epoll_events));
  if (em->epoll_fd < 0)
    return clib_error_return_unix (0, "epoll_create");

  um->file_update = linux_epoll_file_update;
  um->poll_for_input = linux_epoll_input;

  return 0;
}

#endif /* __linux__ */
