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

#include <uclib/base64.h>

static char base64_encoding_table[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char base64_decoding_table[256];

u8 * format_base64_data (u8 * s, va_list * args)
{
  u8 * data_to_encode = va_arg (*args, u8 *);
  u32 n_bytes_of_data_to_encode = va_arg (*args, u32);

  u8 * d = data_to_encode;
  i32 n_bytes_left = n_bytes_of_data_to_encode;

  u32 n0 = n_bytes_of_data_to_encode / 3, n1 = n_bytes_of_data_to_encode % 3;

  u32 n_pad_bytes = n1 ? 3 - n1 : 0;
  u32 n_bytes_of_encoded_data = 4 * (n0 + (n_pad_bytes != 0));
  u8 * e;

  vec_add2 (s, e, n_bytes_of_encoded_data);

  while (n_bytes_left > 0)
    {
      u8 x0 = d[0];
      u8 x1 = n_bytes_left > 1 ? d[1] : 0;
      u8 x2 = n_bytes_left > 2 ? d[2] : 0;
      u32 x = (x0 << 8*2) | (x1 << 8*1) | (x2 << 8*0);

      e[0] = base64_encoding_table[(x >> 3*6) & ((1 << 6) - 1)];
      e[1] = base64_encoding_table[(x >> 2*6) & ((1 << 6) - 1)];
      e[2] = base64_encoding_table[(x >> 1*6) & ((1 << 6) - 1)];
      e[3] = base64_encoding_table[(x >> 0*6) & ((1 << 6) - 1)];

      n_bytes_left -= 3;
      d += 3;
      e += 4;
    }

  while (n_pad_bytes > 0)
    {
      *--e = '=';
      n_pad_bytes--;
    }

  return s;
}

uword unformat_base64_data (unformat_input_t * input, va_list * args)
{
  u8 ** result = va_arg (*args, u8 **);
  uword c, triple, n_chars_this_triple, n_pad_chars;

  if (base64_decoding_table[0] != -1)
    {
      int i;

      /* Initialize decoding table as inverse of encoding table. */
      for (i = 1; i < ARRAY_LEN (base64_decoding_table); i++)
        base64_decoding_table[i] = -1;

      for (i = 0; i < ARRAY_LEN (base64_encoding_table); i++)
        base64_decoding_table[(int) base64_encoding_table[i]] = i;

      base64_decoding_table[0] = -1;
    }

  triple = 0;
  n_chars_this_triple = 0;
  *result = 0;
  while ((c = unformat_get_input (input)) != UNFORMAT_END_OF_INPUT)
    {
      char d = base64_decoding_table[c];

      if (d == -1)
        break;

      triple = (triple << 6) | d;
      n_chars_this_triple++;

      if (n_chars_this_triple == 4)
        {
          u8 * p;
          vec_add2 (*result, p, 3);
          p[0] = triple >> (2 * 8);
          p[1] = triple >> (1 * 8);
          p[2] = triple;
          triple = n_chars_this_triple = 0;
        }
    }

  n_pad_chars = 0;
  while (c == '=' && n_chars_this_triple < 4)
    {
      triple = (triple << 6) | 0;
      n_chars_this_triple++;
      n_pad_chars++;
      c = unformat_get_input (input);
    }

  if (c != UNFORMAT_END_OF_INPUT)
    unformat_put_input (input);

  if (n_chars_this_triple == 4)
    {
      u8 * p;
      vec_add2 (*result, p, 3);
      p[0] = triple >> (2 * 8);
      p[1] = triple >> (1 * 8);
      p[2] = triple;

      ASSERT (n_pad_chars < 3);
      _vec_len (*result) -= n_pad_chars;
    }
  else if (n_chars_this_triple > 0)
    {
      /* Missing pad characters: error. */
      vec_free (*result);
    }

  return *result ? 1 : 0;
}
