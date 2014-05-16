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

#ifndef included_clib_bitops_h
#define included_clib_bitops_h

/* Population count from Hacker's Delight. */
always_inline uword count_set_bits (uword x)
{
#if uword_bits == 64
  const uword c1 = 0x5555555555555555;
  const uword c2 = 0x3333333333333333;
  const uword c3 = 0x0f0f0f0f0f0f0f0f;
#else
  const uword c1 = 0x55555555;
  const uword c2 = 0x33333333;
  const uword c3 = 0x0f0f0f0f;
#endif

  /* Sum 1 bit at a time. */
  x = x - ((x >> (uword) 1) & c1);

  /* 2 bits at a time. */
  x = (x & c2) + ((x >> (uword) 2) & c2);

  /* 4 bits at a time. */
  x = (x + (x >> (uword) 4)) & c3;

  /* 8, 16, 32 bits at a time. */
  x = x + (x >> (uword) 8);
  x = x + (x >> (uword) 16);
#if uword_bits == 64
  x = x + (x >> (uword) 32);
#endif

  return x & (2*BITS (uword) - 1);
}

always_inline uword
rotate_left (uword x, uword i)
{ return (x << i) | (x >> (BITS (i) - i)); }

always_inline uword
rotate_right (uword x, uword i)
{ return (x >> i) | (x << (BITS (i) - i)); }

/* Returns snoob from Hacker's Delight.  Next highest number
   with same number of set bits. */
always_inline uword
next_with_same_number_of_set_bits (uword x)
{
  uword smallest, ripple, ones;
  smallest = x & -x;
  ripple = x + smallest;
  ones = x ^ ripple;
  ones = ones >> (2 + log2_first_set (x));
  return ripple | ones;
}

#define foreach_set_bit(var,mask,body)					\
do {									\
  uword _foreach_set_bit_m_##var = (mask);				\
  uword _foreach_set_bit_f_##var;					\
  while (_foreach_set_bit_m_##var != 0)					\
    {									\
      _foreach_set_bit_f_##var = first_set (_foreach_set_bit_m_##var);	\
      _foreach_set_bit_m_##var ^= _foreach_set_bit_f_##var;		\
      (var) = min_log2 (_foreach_set_bit_f_##var);			\
      do { body; } while (0);						\
    }									\
} while (0)

#endif /* included_clib_bitops_h */
