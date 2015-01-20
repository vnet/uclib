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

#include <uclib/uclib.h>

#ifdef __linux__

#include <sys/epoll.h>

typedef struct {
  int epoll_fd;
  struct epoll_event * epoll_events;

  /* Statistics. */
  u64 epoll_files_ready;
  u64 epoll_waits;
} linux_epoll_main_t;

typedef union {
  struct {
    u32 file_id, file_type;
  };
  u64 as_u64;
} linux_epoll_event_data_t;

static void
linux_epoll_update (unix_file_poller_t * fp, unix_file_poller_update_t * u)
{
  linux_epoll_main_t * em = fp->os_opaque;
  struct epoll_event e;
  linux_epoll_event_data_t ed;

  ed.file_id = u->file_id;
  ed.file_type = u->file_type;

  memset (&e, 0, sizeof (e));

  e.events = EPOLLIN;
  if (u->is_write_ready)
    e.events |= EPOLLOUT;
  e.data.u64 = ed.as_u64;

  if (epoll_ctl (em->epoll_fd,
		 (u->type == UNIX_FILE_POLLER_UPDATE_ADD
		  ? EPOLL_CTL_ADD
		  : (u->type == UNIX_FILE_POLLER_UPDATE_MODIFY
		     ? EPOLL_CTL_MOD
		     : EPOLL_CTL_DEL)),
		 u->file_descriptor,
		 &e) < 0)
    clib_unix_warning ("epoll_ctl");
}

static uword
linux_epoll_input (unix_file_poller_t * um, f64 timeout_in_sec)
{
  linux_epoll_main_t * em = um->os_opaque;
  struct epoll_event * e;
  int n_fds_ready;
  uword n_input_events = 0;

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
      return n_input_events;
    }

  em->epoll_waits += 1;
  em->epoll_files_ready += n_fds_ready;

  for (e = em->epoll_events; e < em->epoll_events + n_fds_ready; e++)
    {
      clib_error_t * errors[4];
      int n_errors = 0;
      linux_epoll_event_data_t ed;
      unix_file_poller_file_functions_t * ff;

      ed.as_u64 = e->data.u64;
      ff = vec_elt (um->file_functions_by_file_type, ed.file_type);

      if (PREDICT_TRUE (! (e->events & EPOLLERR)))
	{
	  if (e->events & EPOLLOUT)
	    {
	      errors[n_errors] = ff->write_function (ff, ed.file_id);
	      n_errors += errors[n_errors] != 0;
	    }
	  if (e->events & EPOLLIN)
	    {
	      errors[n_errors] = ff->read_function (ff, ed.file_id);
	      n_errors += errors[n_errors] != 0;
	      n_input_events += 1;
	    }
	}
      else
	{
	  errors[n_errors] = ff->error_function (ff, ed.file_id);
	  n_errors += errors[n_errors] != 0;
	}

      {
	uword i;
	ASSERT (n_errors < ARRAY_LEN (errors));
	for (i = 0; i < n_errors; i++)
	  unix_save_error (um, errors[i]);
      }
    }

  return n_input_events;
}

static void linux_epoll_free (unix_file_poller_t * fp)
{
  linux_epoll_main_t * em = fp->os_opaque;
  close (em->epoll_fd);
  vec_free (em->epoll_events);
  clib_mem_free (em);
  fp->os_opaque = 0;
}

clib_error_t *
unix_file_poller_init (unix_file_poller_t * fp)
{
  linux_epoll_main_t * em = clib_mem_alloc_no_fail (sizeof (em[0]));
  
  memset (em, 0, sizeof (em[0]));

  /* Allocate some events. */
  vec_resize (em->epoll_events, 256);

  em->epoll_fd = epoll_create (vec_len (em->epoll_events));
  if (em->epoll_fd < 0)
    return clib_error_return_unix (0, "epoll_create");

  fp->update = linux_epoll_update;
  fp->poll_for_input = linux_epoll_input;
  fp->free = linux_epoll_free;
  fp->os_opaque = em;

  return 0;
}

#endif /* __linux__ */

#ifdef __APPLE__

#include <sys/event.h>
#include <sys/time.h>

typedef struct {
  int kqueue_fd;
  struct kevent * kevents;

  /* Statistics. */
  u64 kevents_ready;
  u64 kevent_waits;
} bsd_kqueue_main_t;

static bsd_kqueue_main_t bsd_kqueue_main;

static void
bsd_kqueue_file_update (unix_file_poller_t * um,
                        unix_file_poller_file_t * f,
                        unix_file_poller_file_update_type_t update_type)
{
  bsd_kqueue_main_t * km = &bsd_kqueue_main;
  struct kevent changes[2];

  memset (&changes, 0, sizeof (changes));

  EV_SET (&changes[0], f->file_descriptor, EVFILT_READ,
          update_type == UNIX_FILE_POLLER_FILE_UPDATE_DELETE ? EV_DELETE : EV_ADD,
          0, 0,
          (void *) (f - um->file_pool));
  changes[1] = changes[0];
  changes[1].filter = EVFILT_WRITE;

  if (kevent (km->kqueue_fd,
              &changes[0],
              (f->flags & UNIX_FILE_POLLER_FILE_DATA_AVAILABLE_TO_WRITE) ? 2 : 1,
              /* eventlist */ 0,
              /* n_events */ 0,
              /* timeout */ 0) < 0)
    clib_unix_warning ("kevent");
}

static uword
bsd_kqueue_input (unix_file_poller_t * um, f64 timeout_in_sec)
{
  bsd_kqueue_main_t * km = &bsd_kqueue_main;
  struct kevent * e;
  int n_fds_ready;
  struct timespec timeout;

  timeout.tv_sec = timeout_in_sec;
  timeout.tv_nsec = 1e9*(timeout_in_sec - timeout.tv_sec);

  n_fds_ready = kevent (km->kqueue_fd,
                        /* changes */ 0,
                        /* n_changes */ 0,
                        km->kevents,
                        vec_len (km->kevents),
                        &timeout);

  if (n_fds_ready < 0)
    {
      if (unix_error_is_fatal (errno))
        clib_unix_error ("kevent");

      /* non fatal error (e.g. EINTR). */
      return 0;
    }

  km->kevent_waits += 1;
  km->kevents_ready += n_fds_ready;

  for (e = km->kevents; e < km->kevents + n_fds_ready; e++)
    {
      u32 i = pointer_to_uword (e->udata);
      unix_file_poller_file_t * f = pool_elt_at_index (um->file_pool, i);
      clib_error_t * errors[4];
      int n_errors = 0;

      if (e->filter == EVFILT_READ)
        {
          errors[n_errors] = f->read_function (f);
          n_errors += errors[n_errors] != 0;
        }
      if (e->filter == EVFILT_WRITE)
        {
          errors[n_errors] = f->write_function (f);
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
  bsd_kqueue_main_t * km = clib_mem_alloc_no_fail (sizeof (km[0]));
  
  /* Allocate some events. */
  vec_resize (km->kevents, 256);

  km->kqueue_fd = kqueue ();
  if (km->kqueue_fd < 0)
    return clib_error_return_unix (0, "kqueue");

  um->file_update = bsd_kqueue_file_update;
  um->poll_for_input = bsd_kqueue_input;
  um->os_opaque = km;

  return 0;
}

#endif /* __APPLE__ */
