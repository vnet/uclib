#ifndef included_uclib_h
#define included_uclib_h

#define BITS(x)		(8*sizeof(x))
#define ARRAY_LEN(x)	(sizeof (x)/sizeof (x[0]))

#define _STRUCT_FIELD(t,f) (((t *) 0)->f)
#define STRUCT_OFFSET_OF(t,f) ((uword) & _STRUCT_FIELD (t, f))
#define STRUCT_BIT_OFFSET_OF(t,f) (BITS(u8) * (uword) & _STRUCT_FIELD (t, f))
#define STRUCT_SIZE_OF(t,f)   (sizeof (_STRUCT_FIELD (t, f)))
#define STRUCT_BITS_OF(t,f)   (BITS (_STRUCT_FIELD (t, f)))
#define STRUCT_ARRAY_LEN(t,f) ARRAY_LEN (_STRUCT_FIELD (t, f))

/* Stride in bytes between struct array elements. */
#define STRUCT_STRIDE_OF(t,f)			\
  (  ((uword) & (((t *) 0)[1].f))		\
   - ((uword) & (((t *) 0)[0].f)))

#define STRUCT_OFFSET_OF_VAR(v,f) ((uword) (&(v)->f) - (uword) (v))

/* Used to pack structure elements. */
#define CLIB_PACKED(x)	x __attribute__ ((packed))
#define CLIB_UNUSED(x)	x __attribute__ ((unused)) 

#ifndef CLIB_DEBUG
#define CLIB_DEBUG 0
#endif

#if CLIB_DEBUG > 0
#define always_inline static inline
#define static_always_inline static inline
#else
#define always_inline extern inline __attribute__ ((__always_inline__))
#define static_always_inline static inline __attribute__ ((__always_inline__))
#endif

/* Define signed and unsigned 8, 16, 32, and 64 bit types
   and machine signed/unsigned word for all architectures. */
typedef char i8;
typedef short i16;

typedef unsigned char u8;
typedef unsigned short u16;

#if (defined(i386) || defined(_mips) || defined(powerpc) || defined (__SPU__) || defined(__sparc__) || defined(__arm__) || defined (__xtensa__) || defined(__TMS320C6X__))

#define log2_uword_bits 5

typedef int i32;
typedef long long i64;

typedef unsigned int u32;
typedef unsigned long long u64;

#elif defined(alpha) || defined(__x86_64__) || defined (__powerpc64__)

#define log2_uword_bits 6

typedef int i32;
typedef long i64;

typedef unsigned int u32;
typedef unsigned long u64;

#else
#error "can't define types"
#endif

/* Floating point types. */
typedef double f64;
typedef float f32;

/* Word types. */

/* #ifdef's above define log2_uword_bits. */
#define uword_bits (1 << log2_uword_bits)

#if log2_uword_bits == 5
typedef int word;
typedef unsigned int uword;
#else
typedef long word;
typedef unsigned long uword;
#endif

always_inline uword
pointer_to_uword (void * p)
{ return (uword) p; }

#define uword_to_pointer(u,type) ((type) (uword) (u))

always_inline uword pow2_mask (uword x)
{ return ((uword) 1 << x) - (uword) 1; }

always_inline uword is_pow2 (uword x)
{ return 0 == (x & (x - 1)); }

always_inline uword round_pow2 (uword x, uword pow2)
{
  return (x + pow2 - 1) &~ (pow2 - 1);
}

always_inline uword first_set (uword x)
{ return x & -x; }

always_inline uword log2_first_set (uword x)
{ return __builtin_ctz (x); }

always_inline uword min_log2 (uword x)
{
  uword n = __builtin_clz (x);
  return BITS (uword) - n - 1;
}

always_inline uword max_pow2 (uword x)
{
  word y = (word) 1 << min_log2 (x);
  if (x > y) y *= 2;
  return y;
}

#define uclib_max(x,y)				\
({						\
  __typeof__ (x) _x = (x);			\
  __typeof__ (y) _y = (y);			\
  _x > _y ? _x : _y;				\
})

#define uclib_min(x,y)				\
({						\
  __typeof__ (x) _x = (x);			\
  __typeof__ (y) _y = (y);			\
  _x < _y ? _x : _y;				\
})

#define uclib_abs(x)				\
({						\
  __typeof__ (x) _x = (x);			\
  _x < 0 ? -_x : _x;				\
})

#include <uclib/os.h>
#include <uclib/error.h>
#include <uclib/mem.h>
#include <uclib/vec.h>

#include <uclib/bitops.h>
#include <uclib/bitmap.h>
#include <uclib/pool.h>

#include <uclib/format.h>

#endif /* included_uclib_h */
