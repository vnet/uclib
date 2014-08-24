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

#ifndef included_clib_http_h
#define included_clib_http_h

typedef struct {
  u8 * key;
  u8 * value;
} http_header_line_t;

always_inline void
http_header_line_free (http_header_line_t * l)
{
  vec_free (l->key);
  vec_free (l->value);
}

always_inline uword
http_header_line_is_terminal (http_header_line_t * l)
{ return ! l->key && ! l->value; }

unformat_function_t unformat_http_header_line;

#define foreach_http_request_method             \
  _ (INVALID) _ (GET) _ (PUT) _ (POST)

typedef enum {
#define _(f) HTTP_REQUEST_METHOD_##f,
  foreach_http_request_method
#undef _
} http_request_method_type_t;

typedef struct {
  union {
    struct {
      http_request_method_type_t method;
      u8 * path;
    } request;
    struct {
      u32 code;
      u8 * code_as_string;
    } response;
  };
  u8 is_response;
  u8 http_version[2];
  http_header_line_t * lines;
  uword * line_index_by_key;
} http_request_or_response_t;

always_inline void
http_request_or_response_free (http_request_or_response_t * r)
{
  http_header_line_t * l;
  if (r->is_response)
    vec_free (r->response.code_as_string);
  else
    vec_free (r->request.path);
  vec_foreach (l, r->lines)
    http_header_line_free (l);
  vec_free (r->lines);
  hash_free (r->line_index_by_key);
}

always_inline http_header_line_t *
http_request_line_for_key (http_request_or_response_t * r, char * key)
{
  u8 * tmp_key = format (0, "%s", key);
  uword * p = hash_get (r->line_index_by_key, tmp_key);
  vec_free (tmp_key);
  if (! p)
    return 0;
  return vec_elt_at_index (r->lines, p[0]);
}

always_inline u8 *
http_request_value_for_key (http_request_or_response_t * r, char * key)
{
  http_header_line_t * l = http_request_line_for_key (r, key);
  return l ? l->value : 0;
}

always_inline uword
http_request_value_for_key_compare (http_request_or_response_t * r, char * key, char * value)
{
  http_header_line_t * l = http_request_line_for_key (r, key);
  uword n = strlen (value);
  return l && n == vec_len (l->value) && ! memcmp (value, l->value, n);
}

uword
http_request_unformat_value_for_key (http_request_or_response_t * r, char * key, char * fmt, ...);

unformat_function_t unformat_http_request;
unformat_function_t unformat_http_response;

#endif /* included_clib_http_h */
