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

/* Error reporting. */
#include <stdarg.h>

void _clib_error (int how_to_die,
		  char * function_name,
		  uword line_number,
		  char * fmt, ...)
{
  u8 * msg = 0;
  va_list va;

  if (function_name)
    {
      msg = format (msg, "%s:", function_name);
      if (line_number > 0)
	msg = format (msg, "%wd:", line_number);
      msg = format (msg, " ");
    }

  va_start (va, fmt);
  msg = va_format (msg, fmt, &va);
  va_end (va);

#ifdef HAVE_ERRNO
  if (how_to_die & CLIB_ERROR_ERRNO_VALID)
    msg = format (msg, ": %s (errno %d)", strerror (errno), errno);
#endif

  if (vec_end (msg)[-1] != '\n')
    vec_add1 (msg, '\n');

  os_puts (msg, vec_len (msg), /* is_error */ 1);

  vec_free (msg);

  if (how_to_die & CLIB_ERROR_ABORT)
    os_panic ();
  if (how_to_die & CLIB_ERROR_FATAL)
    os_panic ();
}

clib_error_t * _clib_error_return (clib_error_t * errors,
                                   uword code,
                                   uword flags,
                                   char * where,
				   char * fmt, ...)
{
  clib_error_t * e;
  va_list va;

#ifdef HAVE_ERRNO
  /* Save errno since it may be re-set before we'll need it. */
  word errno_save = errno;
#endif

  va_start (va, fmt);
  vec_add2 (errors, e, 1);
  if (fmt)
    e->what = va_format (0, fmt, &va);

#ifdef HAVE_ERRNO
  if (flags & CLIB_ERROR_ERRNO_VALID)
    {
      if (e->what)
	e->what = format (e->what, ": ");
      e->what = format (e->what, "%s", strerror (errno_save));
    }
#endif
  
  e->where = (u8 *) where;
  e->code = code;
  e->flags = flags;
  va_end (va);
  return errors;
}

void * clib_error_free_vector (clib_error_t * errors)
{
  clib_error_t * e;
  vec_foreach (e, errors)
    vec_free (e->what);
  vec_free (errors);
  return 0;
}

u8 * format_clib_error (u8 * s, va_list * va)
{
  clib_error_t * errors = va_arg (*va, clib_error_t *);
  clib_error_t * e;

  vec_foreach (e, errors)
    {
      if (! e->what)
	continue;

      if (e->where)
	{
	  u8 * where = 0;

	  if (e > errors)
	    where = format (where, "from ");
	  where = format (where, "%s", e->where);

	  s = format (s, "%v: ", where);
	  vec_free (where);
	}

      s = format (s, "%v\n", e->what);
    }

  return s;
}

void * clib_error_free (clib_error_t * errors)
{
  clib_error_t * e;
  vec_foreach (e, errors)
    vec_free (e->what);
  vec_free (errors);
  return 0;
}

clib_error_t * _clib_error_report (clib_error_t * errors)
{
  if (errors)
    {
      u8 * msg = format (0, "%U", format_clib_error, errors);

      os_puts (msg, vec_len (msg), /* is_error */ 1);
      vec_free (msg);

      if (errors->flags & CLIB_ERROR_ABORT)
	os_panic ();
      if (errors->flags & CLIB_ERROR_FATAL)
	os_panic (1);

      clib_error_free (errors);
    }
  return 0;
}
