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

/* Format vectors. */
u8 * format_vec32 (u8 * s, va_list * va)
{
  u32 * v = va_arg (*va, u32 *);
  char * fmt = va_arg (*va, char *);
  uword i;
  for (i = 0; i < vec_len (v); i++)
    {
      if (i > 0)
	s = format (s, ", ");
      s = format (s, fmt, v[i]);
    }
  return s;
}

u8 * format_vec_uword (u8 * s, va_list * va)
{
  uword * v = va_arg (*va, uword *);
  char * fmt = va_arg (*va, char *);
  uword i;
  for (i = 0; i < vec_len (v); i++)
    {
      if (i > 0)
	s = format (s, ", ");
      s = format (s, fmt, v[i]);
    }
  return s;
}

/* Ascii buffer and length. */
u8 * format_ascii_bytes (u8 * s, va_list * va)
{
  u8 * v = va_arg (*va, u8 *);
  uword n_bytes = va_arg (*va, uword);
  vec_add (s, v, n_bytes);
  return s;
}

/* Format hex dump. */
u8 * format_hex_bytes (u8 * s, va_list * va)
{
  u8 * bytes = va_arg (*va, u8 *);
  int n_bytes = va_arg (*va, int);
  uword i;

  /* Print short or long form depending on byte count. */
  uword short_form = n_bytes <= 32;
  uword indent = format_get_indent (s);

  if (n_bytes == 0)
    return s;

  for (i = 0; i < n_bytes; i++)
    {
      if (! short_form && (i % 32) == 0)
	s = format (s, "%08x: ", i);

      s = format (s, "%02x", bytes[i]);

      if (! short_form && ((i + 1) % 32) == 0 && (i + 1) < n_bytes)
	s = format (s, "\n%U", format_white_space, indent);
    }

  return s;
}

/* Add variable number of spaces. */
u8 * format_white_space (u8 * s, va_list * va)
{
  uword n = va_arg (*va, uword);
  while (n-- > 0)
    vec_add1 (s, ' ');
  return s;
}

u8 * format_time_interval (u8 * s, va_list * args)
{
  u8 * fmt = va_arg (*args, u8 *);
  f64 t = va_arg (*args, f64);
  u8 * f;

  const f64 seconds_per_minute = 60;
  const f64 seconds_per_hour = 60 * seconds_per_minute;
  const f64 seconds_per_day = 24 * seconds_per_hour;
  uword days, hours, minutes, secs, msecs, usecs;
  
  days = t / seconds_per_day;
  t -= days * seconds_per_day;

  hours = t / seconds_per_hour;
  t -= hours * seconds_per_hour;

  minutes = t / seconds_per_minute;
  t -= minutes * seconds_per_minute;

  secs = t;
  t -= secs;

  msecs = 1e3*t;
  usecs = 1e6*t;

  for (f = fmt; *f; f++)
    {
      uword what, c;
      char * what_fmt = "%d";

      switch (c = *f)
	{
	default:
	  vec_add1 (s, c);
	  continue;

	case 'd':
	  what = days;
	  what_fmt = "%d";
	  break;
	case 'h':
	  what = hours;
	  what_fmt = "%02d";
	  break;
	case 'm':
	  what = minutes;
	  what_fmt = "%02d";
	  break;
	case 's':
	  what = secs;
	  what_fmt = "%02d";
	  break;
	case 'f':
	  what = msecs;
	  what_fmt = "%03d";
	  break;
	case 'u':
	  what = usecs;
	  what_fmt = "%06d";
	  break;
	}

      s = format (s, what_fmt, what);
    }

  return s;
}

u8 * format_timeval (u8 * s, va_list * args)
{
  char * fmt = va_arg (*args, char *);
  struct timeval * tv = va_arg (*args, struct timeval *);
  struct tm * tm;
  word msec;
  char * f, c;

  if (! fmt)
    fmt = "y/m/d H:M:S:F";

  if (! tv)
    {
      static struct timeval now;
      gettimeofday (&now, 0);
      tv = &now;
    }

  msec = flt_round_nearest (1e-3 * tv->tv_usec);
  if (msec >= 1000)
    { msec = 0; tv->tv_sec++; }

  {
    time_t t = tv->tv_sec;
    tm = localtime (&t);
  }

  for (f = fmt; *f; f++)
    {
      uword what;
      char * what_fmt = "%d";

      switch (c = *f)
	{
	default:
	  vec_add1 (s, c);
	  continue;

	case 'y':
	  what = 1900 + tm->tm_year;
	  what_fmt = "%4d";
	  break;
	case 'm':
	  what = tm->tm_mon + 1;
	  what_fmt = "%02d";
	  break;
	case 'd':
	  what = tm->tm_mday;
	  what_fmt = "%02d";
	  break;
	case 'H':
	  what = tm->tm_hour;
	  what_fmt = "%02d";
	  break;
	case 'M':
	  what = tm->tm_min;
	  what_fmt = "%02d";
	  break;
	case 'S':
	  what = tm->tm_sec;
	  what_fmt = "%02d";
	  break;
	case 'F':
	  what = msec;
	  what_fmt = "%03d";
	  break;
	}

      s = format (s, what_fmt, what);
    }

  return s;
}

u8 * format_time_float (u8 * s, va_list * args)
{
  u8 * fmt = va_arg (*args, u8 *);
  f64 t = va_arg (*args, f64);
  struct timeval tv;
  if (t <= 0)
    t = unix_time_now ();
  tv.tv_sec = t;
  tv.tv_usec = 1e6*(t - tv.tv_sec);
  return format (s, "%U", format_timeval, fmt, &tv);
}

/* Unparse memory size e.g. 100, 100k, 100m, 100g. */
u8 * format_memory_size (u8 * s, va_list * va)
{
  uword size = va_arg (*va, uword);
  uword l, u, log_u;

  l = size > 0 ? min_log2 (size) : 0;
  if (l < 10)
    log_u = 0;
  else if (l < 20)
    log_u = 10;
  else if (l < 30)
    log_u = 20;
  else
    log_u = 30;

  u = (uword) 1 << log_u;
  if (size & (u - 1))
    s = format (s, "%.2f", (f64) size / (f64) u);
  else
    s = format (s, "%d", size >> log_u);

  if (log_u != 0)
    s = format (s, "%c", " kmg"[log_u / 10]);

  return s;
}

/* Parse memory size e.g. 100, 100k, 100m, 100g. */
uword unformat_memory_size (unformat_input_t * input, va_list * va)
{
  uword amount, shift, c;
  uword * result = va_arg (*va, uword *);

  if (! unformat (input, "%wd%_", &amount))
    return 0;

  c = unformat_get_input (input);
  switch (c)
    {
    case 'k': case 'K': shift = 10; break;
    case 'm': case 'M': shift = 20; break;
    case 'g': case 'G': shift = 30; break;
    default:
      shift = 0;
      unformat_put_input (input);
      break;
    }

  *result = amount << shift;
  return 1;
}

/* Format c identifier: e.g. a_name -> "a name".
   Words for both vector names and null terminated c strings. */
u8 * format_c_identifier (u8 * s, va_list * va)
{
  u8 * id = va_arg (*va, u8 *);
  uword i;

  if (id)
    for (i = 0; id[i] != 0; i++)
      {
	u8 c = id[i];

	if (c == '_')
	  c = ' ';
	vec_add1 (s, c);
      }

  return s;
}

/* Format generic network address: takes two arguments family and address.
   Assumes network byte order. */
u8 * format_network_address (u8 * s, va_list * args)
{
  uword family = va_arg (*args, uword);
  u8 * addr    = va_arg (*args, u8 *);

  switch (family)
    {
    case AF_INET:
      s = format (s, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
      break;

    case AF_UNSPEC:
      /* We use AF_UNSPEC for ethernet addresses. */
      s = format (s, "%02x:%02x:%02x:%02x:%02x:%02x",
		  addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
      break;

    default:
      clib_error ("unsupported address family %d", family);
    }

  return s;
}

u8 * format_sockaddr (u8 * s, va_list * args)
{
  void * v = va_arg (*args, void *);
  struct sockaddr * sa = v;

  switch (sa->sa_family)
    {
    case AF_INET:
      {
	struct sockaddr_in * i = v;
	s = format (s, "%U:%d",
		    format_network_address, AF_INET, &i->sin_addr.s_addr,
                    ntohs (i->sin_port));
      }
      break;

    default:
      s = format (s, "sockaddr family %d", sa->sa_family);
      break;
    }

  return s;
}

u8 * format_symbol_with_address (u8 * s, va_list * va)
{
  void * addr = va_arg (*va, void *);
#ifdef CLIB_HAVE_ELF
  s = format (s, "%U", format_clib_elf_symbol_with_address, addr);
#else
  s = format (s, "%p", addr);
#endif
  return s;
}
