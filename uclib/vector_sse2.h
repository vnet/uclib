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

#ifndef included_vector_sse2_h
#define included_vector_sse2_h

/* 128 bit interleaves. */
#ifndef __clang__
always_inline u8x16 u8x16_interleave_hi (u8x16 a, u8x16 b)
{ return (u8x16) __builtin_ia32_punpckhbw128 ((i8x16) a, (i8x16) b); }

always_inline u8x16 u8x16_interleave_lo (u8x16 a, u8x16 b)
{ return (u8x16) __builtin_ia32_punpcklbw128 ((i8x16) a, (i8x16) b); }

always_inline u16x8 u16x8_interleave_hi (u16x8 a, u16x8 b)
{ return (u16x8) __builtin_ia32_punpckhwd128 ((i16x8) a, (i16x8) b); }

always_inline u16x8 u16x8_interleave_lo (u16x8 a, u16x8 b)
{ return (u16x8) __builtin_ia32_punpcklwd128 ((i16x8) a, (i16x8) b); }

always_inline u32x4 u32x4_interleave_hi (u32x4 a, u32x4 b)
{ return (u32x4) __builtin_ia32_punpckhdq128 ((i32x4) a, (i32x4) b); }

always_inline u32x4 u32x4_interleave_lo (u32x4 a, u32x4 b)
{ return (u32x4) __builtin_ia32_punpckldq128 ((i32x4) a, (i32x4) b); }

always_inline u64x2 u64x2_interleave_hi (u64x2 a, u64x2 b)
{ return (u64x2) __builtin_ia32_punpckhqdq128 ((i64x2) a, (i64x2) b); }

always_inline u64x2 u64x2_interleave_lo (u64x2 a, u64x2 b)
{ return (u64x2) __builtin_ia32_punpcklqdq128 ((i64x2) a, (i64x2) b); }

#else

always_inline u8x16 u8x16_interleave_hi (u8x16 a, u8x16 b)
{
  return (u8x16) __builtin_shufflevector (a, b,
					  0x08, 0x18, 0x09, 0x19, 0x0a, 0x1a, 0x0b, 0x1b,
					  0x0c, 0x1c, 0x0d, 0x1d, 0x0e, 0x1e, 0x0f, 0x1f);
}

always_inline u8x16 u8x16_interleave_lo (u8x16 a, u8x16 b)
{
  return (u8x16) __builtin_shufflevector (a, b,
					  0x00, 0x10, 0x01, 0x11, 0x02, 0x12, 0x03, 0x13,
					  0x04, 0x14, 0x05, 0x15, 0x06, 0x16, 0x07, 0x17);
}

always_inline u16x8 u16x8_interleave_hi (u16x8 a, u16x8 b)
{
  return (u16x8) __builtin_shufflevector (a, b, 4, 4+8, 5, 5+8, 6, 6+8, 7, 7+8);
}

always_inline u16x8 u16x8_interleave_lo (u16x8 a, u16x8 b)
{
  return (u16x8) __builtin_shufflevector (a, b, 0, 0+8, 1, 1+8, 2, 2+8, 3, 3+8);
}

always_inline u32x4 u32x4_interleave_hi (u32x4 a, u32x4 b)
{ return (u8x16) __builtin_shufflevector (a, b, 2, 6, 3, 7); }

always_inline u32x4 u32x4_interleave_lo (u32x4 a, u32x4 b)
{ return (u8x16) __builtin_shufflevector (a, b, 0, 4, 1, 5); }

always_inline u64x2 u64x2_interleave_hi (u64x2 a, u64x2 b)
{ return (u64x2) __builtin_shufflevector (a, b, 0, 2); }

always_inline u64x2 u64x2_interleave_lo (u64x2 a, u64x2 b)
{ return (u64x2) __builtin_shufflevector (a, b, 1, 3); }

#endif

/* 64 bit interleaves. */
always_inline u8x8 u8x8_interleave_hi (u8x8 a, u8x8 b)
{ return (u8x8) __builtin_ia32_punpckhbw ((i8x8) a, (i8x8) b); }

always_inline u8x8 u8x8_interleave_lo (u8x8 a, u8x8 b)
{ return (u8x8) __builtin_ia32_punpcklbw ((i8x8) a, (i8x8) b); }

always_inline u16x4 u16x4_interleave_hi (u16x4 a, u16x4 b)
{ return (u16x4) __builtin_ia32_punpckhwd ((i16x4) a, (i16x4) b); }

always_inline u16x4 u16x4_interleave_lo (u16x4 a, u16x4 b)
{ return (u16x4) __builtin_ia32_punpcklwd ((i16x4) a, (i16x4) b); }

always_inline u32x2 u32x2_interleave_hi (u32x2 a, u32x2 b)
{ return (u32x2) __builtin_ia32_punpckhdq ((i32x2) a, (i32x2) b); }

always_inline u32x2 u32x2_interleave_lo (u32x2 a, u32x2 b)
{ return (u32x2) __builtin_ia32_punpckldq ((i32x2) a, (i32x2) b); }

/* 128 bit packs. */
always_inline u8x16 u16x8_pack (u16x8 lo, u16x8 hi)
{ return (u8x16) __builtin_ia32_packuswb128 ((i16x8) lo, (i16x8) hi); }

always_inline i8x16 i16x8_pack (i16x8 lo, i16x8 hi)
{ return (i8x16) __builtin_ia32_packsswb128 ((i16x8) lo, (i16x8) hi); }

always_inline u16x8 u32x4_pack (u32x4 lo, u32x4 hi)
{ return (u16x8) __builtin_ia32_packssdw128 ((i32x4) lo, (i32x4) hi); }

/* 64 bit packs. */
always_inline u8x8 u16x4_pack (u16x4 lo, u16x4 hi)
{ return (u8x8) __builtin_ia32_packuswb ((i16x4) lo, (i16x4) hi); }

always_inline i8x8 i16x4_pack (i16x4 lo, i16x4 hi)
{ return __builtin_ia32_packsswb (lo, hi); }

always_inline u16x4 u32x2_pack (u32x2 lo, u32x2 hi)
{ return (u16x4) __builtin_ia32_packssdw ((i32x2) lo, (i32x2) hi); }

always_inline i16x4 i32x2_pack (i32x2 lo, i32x2 hi)
{ return __builtin_ia32_packssdw (lo, hi); }

/* 128 bit shifts. */
#define _(t,ti,lr,f)						\
  always_inline t t##_ishift_##lr (t x, int i)			\
  { return (t) __builtin_ia32_##f##i128 ((ti) x, i); }		\
								\
  always_inline t t##_shift_##lr (t x, t y)			\
  { return (t) __builtin_ia32_##f##128 ((ti) x, (ti) y); }

_ (u16x8, i16x8, left, psllw);
_ (u32x4, i32x4, left, pslld);
_ (u64x2, i64x2, left, psllq);
_ (u16x8, i16x8, right, psrlw);
_ (u32x4, i32x4, right, psrld);
_ (u64x2, i64x2, right, psrlq);
_ (i16x8, i16x8, left, psllw);
_ (i32x4, i32x4, left, pslld);
_ (i64x2, i64x2, left, psllq);
_ (i16x8, i16x8, right, psraw);
_ (i32x4, i32x4, right, psrad);

#undef _

#define u8x16_word_shift_left(x,n_bytes)			\
  ((u8x16) __builtin_ia32_pslldqi128 ((i64x2) (x), (n_bytes)))

#define u8x16_word_shift_right(x,n_bytes)			\
  ((u8x16) __builtin_ia32_psrldqi128 ((i64x2) (x), (n_bytes)))

#define i8x16_word_shift_left(a,n)			\
  ((i8x16) u8x16_word_shift_left((i64x2) (a), (n)))
#define i8x16_word_shift_right(a,n)			\
  ((i8x16) u8x16_word_shift_right((i64x2) (a), (n)))

#define u16x8_word_shift_left(a,n) \
  ((u16x8) u8x16_word_shift_left((u8x16) (a), (n) * sizeof (u16)))
#define i16x8_word_shift_left(a,n) \
  ((u16x8) u8x16_word_shift_left((u8x16) (a), (n) * sizeof (u16)))
#define u16x8_word_shift_right(a,n) \
  ((u16x8) u8x16_word_shift_right((u8x16) (a), (n) * sizeof (u16)))
#define i16x8_word_shift_right(a,n) \
  ((i16x8) u8x16_word_shift_right((u8x16) (a), (n) * sizeof (u16)))

#define u32x4_word_shift_left(a,n) \
  ((u32x4) u8x16_word_shift_left((u8x16) (a), (n) * sizeof (u32)))
#define i32x4_word_shift_left(a,n) \
  ((u32x4) u8x16_word_shift_left((u8x16) (a), (n) * sizeof (u32)))
#define u32x4_word_shift_right(a,n) \
  ((u32x4) u8x16_word_shift_right((u8x16) (a), (n) * sizeof (u32)))
#define i32x4_word_shift_right(a,n) \
  ((i32x4) u8x16_word_shift_right((u8x16) (a), (n) * sizeof (u32)))

#define u64x2_word_shift_left(a,n) \
  ((u64x2) u8x16_word_shift_left((u8x16) (a), (n) * sizeof (u64)))
#define i64x2_word_shift_left(a,n) \
  ((u64x2) u8x16_word_shift_left((u8x16) (a), (n) * sizeof (u64)))
#define u64x2_word_shift_right(a,n) \
  ((u64x2) u8x16_word_shift_right((u8x16) (a), (n) * sizeof (u64)))
#define i64x2_word_shift_right(a,n) \
  ((i64x2) u8x16_word_shift_right((u8x16) (a), (n) * sizeof (u64)))

#define _signed_binop(n,m,f,g)						\
  /* Unsigned */							\
  always_inline u##n##x##m						\
  u##n##x##m##_##f (u##n##x##m x, u##n##x##m y)				\
  { return (u##n##x##m) __builtin_ia32_##g ((i##n##x##m) x, (i##n##x##m) y); } \
									\
  /* Signed */								\
  always_inline i##n##x##m						\
  i##n##x##m##_##f (i##n##x##m x, i##n##x##m y)				\
  { return (i##n##x##m) __builtin_ia32_##g ((i##n##x##m) x, (i##n##x##m) y); }

#ifndef __clang__
#define _(t,n,lr1,lr2)						\
  always_inline t##x##n						\
  t##x##n##_word_rotate2_##lr1 (t##x##n w0, t##x##n w1, int i)	\
  {								\
    int m = sizeof (t##x##n) / sizeof (t);			\
    ASSERT (i >= 0 && i < m);					\
    return (t##x##n##_word_shift_##lr1 (w0, i)			\
	    | t##x##n##_word_shift_##lr2 (w1, m - i));		\
  }								\
								\
  always_inline t##x##n						\
  t##x##n##_word_rotate_##lr1 (t##x##n w0, int i)		\
  { return t##x##n##_word_rotate2_##lr1 (w0, w0, i); }

_ (u8, 16, left, right);
_ (u8, 16, right, left);
_ (u16, 8, left, right);
_ (u16, 8, right, left);
_ (u32, 4, left, right);
_ (u32, 4, right, left);
_ (u64, 2, left, right);
_ (u64, 2, right, left);

#undef _
#endif

#define u32x4_select(A,MASK)						\
({									\
  u32x4 _x, _y;								\
  _x = (A);								\
  asm volatile ("pshufd %[mask], %[x], %[y]"				\
		: /* outputs */ [y] "=x" (_y)				\
		: /* inputs */  [x] "x" (_x), [mask] "i" (MASK));	\
  _y;									\
})

#define u32x4_splat_word(x,i)			\
  u32x4_select ((x), (((i) << (2*0))		\
		      | ((i) << (2*1))		\
		      | ((i) << (2*2))		\
		      | ((i) << (2*3))))

/* Converts all ones/zeros compare mask to bitmap. */
always_inline u32 u8x16_compare_byte_mask (u8x16 x)
{ return __builtin_ia32_pmovmskb128 ((i8x16) x); }

#undef _signed_binop

#endif /* included_vector_sse2_h */
