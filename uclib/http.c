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

uword unformat_http_header_line (unformat_input_t * input, va_list * va)
{
  uword c;
  u8 * vs[2] = {0};
  u32 saw_colon = 0;
  http_header_line_t * result = va_arg (*va, http_header_line_t *);

  while (1)
    {
      c = unformat_get_input (input);
      switch (c)
        {
        case UNFORMAT_END_OF_INPUT:
          vec_free (vs[0]);
          vec_free (vs[1]);
          return 0;

        case '\r':
          if (unformat_peek_input (input) == '\n')
            unformat_get_input (input);
        case '\n':
          /* Reject empty key with non-empty value. */
          if (! vs[0] && vs[1])
            return 0;
          result->key = vs[0];
          result->value = vs[1];
          return 1;

        case ':':
          saw_colon = 1;
          break;

        case ' ': case '\t':
          if (saw_colon && vec_len (vs[saw_colon]) == 0)
            /* skip white space after colon */;
          else
            vec_add1 (vs[saw_colon], c);
          break;

        default:
          /* Normalize keys as lower case. */
          if (! saw_colon && c >= 'A' && c <= 'Z')
            c = (c - 'A') + 'a';
          vec_add1 (vs[saw_colon], c);
          break;
        }
    }

  /* Not reached. */
  return 0;
}

uword
http_request_unformat_value_for_key (http_request_or_response_t * r, char * key, char * fmt, ...)
{
  va_list va;
  uword result;
  http_header_line_t * l;
  unformat_input_t input;

  va_start (va, fmt);

  l = http_request_line_for_key (r, key);
  if (! l)
    return 0;

  unformat_init_vector (&input, l->value);
  result = va_unformat (&input, fmt, &va);

  input.buffer = 0;
  unformat_free (&input);

  va_end (va);
  return result;
}

uword unformat_http_request (unformat_input_t * input, va_list * va)
{
  http_request_or_response_t * r = va_arg (*va, http_request_or_response_t *);
  u8 * method_string = 0;
  uword is_error = 1;

  memset (r, 0, sizeof (r[0]));
  r->is_response = 0;
  r->request.method = HTTP_REQUEST_METHOD_INVALID;

  if (! unformat (input, "%s %s HTTP/%d.%d", &method_string, &r->request.path,
                  &r->http_version[0], &r->http_version[1]))
    goto done;

  switch (method_string[0])
    {
#define _(f,g) if (! strcmp ((char *) method_string, #f)) r->request.method = HTTP_REQUEST_METHOD_##g
    case 'G': _ (GET, GET); break;
    case 'g': _ (get, GET); break;
    case 'p': _ (put, PUT); _ (post, POST); break;
    case 'P': _ (PUT, PUT); _ (POST, POST); break;
#undef _
    default: break;
    }
  if (r->request.method == HTTP_REQUEST_METHOD_INVALID)
    goto done;

  while (1)
    {
      http_header_line_t l;
      if (! unformat_user (input, unformat_http_header_line, &l))
        goto done;
      if (http_header_line_is_terminal (&l))
        break;
      vec_add1 (r->lines, l);
    }

  {
    http_header_line_t * l;
    r->line_index_by_key = hash_create_vec (vec_len (r->lines), sizeof (u8), sizeof (uword));
    vec_foreach (l, r->lines)
      hash_set_mem (r->line_index_by_key, l->key, l - r->lines);
  }

  is_error = 0;

 done:
  vec_free (method_string);
  if (is_error)
    http_request_or_response_free (r);
  return is_error;
}

uword unformat_http_response (unformat_input_t * input, va_list * va)
{
  http_request_or_response_t * r = va_arg (*va, http_request_or_response_t *);

  memset (r, 0, sizeof (r[0]));
  r->is_response = 1;

  if (! unformat (input, "HTTP/%d.%d %d %U",
                  &r->http_version[0], &r->http_version[1],
                  &r->response.code,
                  unformat_line, &r->response.code_as_string))
    goto error;

  while (1)
    {
      http_header_line_t l;
      if (! unformat_user (input, unformat_http_header_line, &l))
        goto error;
      if (http_header_line_is_terminal (&l))
        break;
      vec_add1 (r->lines, l);
    }

  {
    http_header_line_t * l;
    r->line_index_by_key = hash_create_vec (vec_len (r->lines), sizeof (u8), sizeof (uword));
    vec_foreach (l, r->lines)
      hash_set_mem (r->line_index_by_key, l->key, l - r->lines);
  }

  return 1;

 error:
  http_request_or_response_free (r);
  return 0;
}

