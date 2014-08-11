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

#ifndef included_uclib_url_h
#define included_uclib_url_h

#define foreach_url_component                   \
  _ (scheme)                                    \
  _ (user)                                      \
  _ (password)                                  \
  _ (host)                                      \
  _ (port)                                      \
  _ (path)                                      \
  _ (query)                                     \
  _ (fragment)

typedef enum {
#define _(f) URL_COMPONENT_##f,
  foreach_url_component
#undef _
  URL_N_COMPONENT,
} url_component_type_t;

typedef struct {
  u8 * components[URL_N_COMPONENT];
} url_t;

always_inline void
url_free (url_t * u)
{
  int i;
  for (i = 0; i < ARRAY_LEN (u->components); i++)
    vec_free (u->components[i]);
}

/* Component accesor inlines. */
#define _(f) always_inline u8 * url_##f (url_t * u) { return u->components[URL_COMPONENT_##f]; }
foreach_url_component
#undef _

clib_error_t * url_parse_components (u8 * url_string, url_t * url);

#endif /* included_uclib_url_h */
