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

#ifndef included_vector_arm_neon_h
#define included_vector_arm_neon_h

#include <arm_neon.h>

always_inline u16 u8x16_compare_byte_mask (u8x16 x)
{
#define _(i) (1 << i)
    u8x16 mask = {
        _ (0), _ (1), _ (2), _ (3), _ (4), _ (5), _ (6), _ (7),
        _ (0), _ (1), _ (2), _ (3), _ (4), _ (5), _ (6), _ (7),
    };
#undef _
    x &= mask;

#ifdef __aarch64__
    x = vpaddq_u8 (x, x);       /* 8 results */
    x = vpaddq_u8 (x, x);       /* 4 results */
    x = vpaddq_u8 (x, x);       /* 2 results */
    return vgetq_lane_u8 (x, 0) | (vgetq_lane_u8 (x, 1) << 8);
#else
    {
        u8x8 hi, lo;
        hi = vget_high_u8 (x);
        lo = vget_low_u8 (x);
        hi = vpadd_u8 (hi, hi); /* 4 results */
        lo = vpadd_u8 (lo, lo);
        hi = vpadd_u8 (hi, hi); /* 2 results */
        lo = vpadd_u8 (lo, lo);
        hi = vpadd_u8 (hi, hi); /* 1 result */
        lo = vpadd_u8 (lo, lo);
        return vget_lane_u8 (lo, 0) | (vget_lane_u8 (hi, 0) << 8);
    }
#endif
}

#endif /* included_vector_arm_neon_h */
