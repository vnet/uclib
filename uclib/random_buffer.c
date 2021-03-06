/*
  Copyright (c) 2005 Eliot Dresselhaus

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

/* Fill random buffer. */
void clib_random_buffer_fill (clib_random_buffer_t * b, uword n_words)
{
  uword * w, n = n_words;

  if (n < 256)
    n = 256;

  n = round_pow2 (n, 2 << ISAAC_LOG2_SIZE);

  vec_add2 (b->buffer, w, n);
  do {
    isaac2 (b->ctx, w);
    w += 2 * ISAAC_SIZE;
    n -= 2 * ISAAC_SIZE;
  } while (n > 0);
}

void clib_random_buffer_init (clib_random_buffer_t * b, uword seed)
{
  uword i, j;

  memset (b, 0, sizeof (b[0]));

  /* Seed ISAAC. */
  for (i = 0; i < ARRAY_LEN (b->ctx); i++)
    {
      uword s[ISAAC_SIZE];

      for (j = 0; j < ARRAY_LEN (s); j++)
	s[j] = ARRAY_LEN (b->ctx) * (seed + j) + i;

      isaac_init (&b->ctx[i], s);
    }
}

void clib_random_buffer_init_multiseed (clib_random_buffer_t * b, u8 * seed)
{
  int i;

  memset (b, 0, sizeof (b[0]));

  /* Seed ISAAC. */
  for (i = 0; i < ARRAY_LEN (b->ctx); i++)
    {
      uword s[ISAAC_SIZE];
      memcpy (s, seed + i*sizeof(s), sizeof (s));
      isaac_init (&b->ctx[i], s);
    }
}
