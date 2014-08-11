/*
  Copyright (c) 2014 Eliot Dresselhaus

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

#include <uclib/uclib.h>

#ifdef included_uclib_url_h

/* RFC 1738, 3986 */
clib_error_t * url_parse_components (u8 * url_string, url_t * url)
{
  clib_error_t * error = 0;
  u8 * u = url_string;
  u8 * v;

  memset (url, 0, sizeof (url[0]));

  /* Read scheme: */
  {
    u8 * colon = (u8 *) strchr ((char *) u, ':');

    if (! colon)
      {
        error = clib_error_return (0, "scheme: is required");
        goto done;
      }

    for (v = u; v < colon; v++)
      {
        /* <scheme> := [a-zA-Z+-.]+ case insensitive. */
        u8 c = v[0];
        switch (c)
          {
          case 'A' ... 'Z':
            c = 'a' + (v[0] - 'A'); /* to lowercase */
            /* fall through */
          case 'a' ... 'z':
          case '+': case '-': case '.':
            vec_add1 (url->components[URL_COMPONENT_scheme], c);
            break;

          default:
            error = clib_error_return (0, "illegal character '%c' in scheme", v[0]);
            goto done;
          }
      }

    u = colon + 1;
  }

  /* Match // as in http:// */
  if (u[0] != '/' && u[1] != '1')
    {
      error = clib_error_return (0, "expected // after scheme: found '%c%c'", u[0], u[1]);
      goto done;
    }
  u += 2;

  /* Match USER:PASSWORD@HOST:PORT before /PATH */
  {
    int user_password_specified = 0;
    v = u;
    while (v[0])
      {
        if (v[0] == '@')
          {
            user_password_specified = 1;
            break;
          }
        else if (v[0] == '/')
          {
            user_password_specified = 0;
            break;
          }
        v++;
      }

    if (user_password_specified)
      {
        for (; u[0] != 0; u++)
          {
            if (u[0] == ':' || u[0] == '@')
              break;
            vec_add1 (url->components[URL_COMPONENT_user], u[0]);
          }
        if (u[0] == ':')
          {
            for (u = u + 1; u[0] != '@'; u++)
              vec_add1 (url->components[URL_COMPONENT_password], u[0]);
          }
        /* Skip @ */
        ASSERT (u[0] == '@');
        u++;
      }
  }

  {
    int is_ip6_address = u[0] == '[';
    while (u[0] != 0)
      {
        if (is_ip6_address && u[0] == ']')
          {
            u++;                  /* skip close square bracket */
            break;
          }
        if (! is_ip6_address && (u[0] == ':' || u[0] == '/'))
          break;
        vec_add1 (url->components[URL_COMPONENT_host], u[0]);
        u++;
      }
  }

  /* Port specified? */
  if (u[0] == ':')
    {
      for (u = u + 1; u[0] != 0 && u[0] != '/'; u++)
        vec_add1 (url->components[URL_COMPONENT_port], u[0]);
    }

  /* Path follows. */
  if (u[0] == '/')
    {
      for (u = u + 1; u[0] != 0 && u[0] != '#' && u[0] != '?'; u++)
        vec_add1 (url->components[URL_COMPONENT_path], u[0]);

      if (u[0] == '?')
        {
          for (u = u + 1; u[0] != 0 && u[0] != '#'; u++)
            vec_add1 (url->components[URL_COMPONENT_query], u[0]);
        }

      if (u[0] == '#')
        {
          for (u = u + 1; u[0] != 0; u++)
            vec_add1 (url->components[URL_COMPONENT_fragment], u[0]);
        }
    }

  if (u[0] != 0)
    {
      error = clib_error_return (0, "found invalid data at end of url '%s'", u);
      goto done;
    }

 done:
  if (error)
    url_free (url);

  return error;
}

#endif /* included_uclib_url_h */
