/*
  Copyright (c) 2001, 2002, 2003 Eliot Dresselhaus

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

#ifndef included_vec_h
#define included_vec_h

/*
   CLIB vectors are ubiquitous dynamically resized arrays with by user
   defined "headers".  Many CLIB data structures (e.g. hash, heap,
   pool) are vectors with various different headers.

   The memory layout looks like this:

                      user header (aligned to uword boundary)
                      vector length: number of elements
user's pointer ->     vector element #1
                      vector element #2
                      ...

   A user is returned a pointer to element # 1.
   Null pointer vectors are valid and mean a zero length vector.
   You can also have an allocated non-null zero length vector by just
   setting the vector length field to zero (e.g. _vec_len (v) = 0).

   Typically, the header is not present.  Headers allow for other
   data structures to be built atop CLIB vectors.

   Users may specify the alignment for data elements via the
   vec_*_aligned macros.

   Vectors elements can be any C type e.g. (int, double, struct bar).
   This is also true for data types built atop vectors (e.g. heap,
   pool, etc.).

   Many macros have _a variants supporting alignment of vector data
   and _h variants supporting non zero length vector headers.
   The _ha variants support both.

   Standard programming error: memorize a pointer to the ith element 
   of a vector then expand it. Vectors expand by 3/2, so such code
   may appear to work for a period of time. Memorize vector indices
   which are invariant. 
 */

/* Low-level resize allocation function. */
void * vec_resize_allocate_memory (void * v,
				   word length_increment,
				   uword old_length,
				   uword n_bytes_per_elt,
				   uword header_bytes,
				   uword data_align);

/* Vector resize function.  Called as needed by various macros such as
   vec_add1() when we need to allocate memory. */
always_inline void *
_vec_resize (void * v,
	     word length_increment,
	     uword old_length,
	     uword n_bytes_per_elt,
	     uword header_bytes,
	     uword data_align)
{
  vec_header_t * vh;
  uword aligned_header_bytes, new_data_bytes, old_data_bytes;
  void * p;

  aligned_header_bytes = vec_header_bytes (header_bytes);

  old_data_bytes = old_length * n_bytes_per_elt;
  new_data_bytes = (old_length + length_increment) * n_bytes_per_elt;

  old_data_bytes += aligned_header_bytes;
  new_data_bytes += aligned_header_bytes;

  /* Check for overflow (security problems). */
  if ((length_increment >= 0) != (new_data_bytes >= old_data_bytes))
    goto slow_path;

  vh = _vec_find (v);

  if (! v)
    goto slow_path;

  p = v - aligned_header_bytes;

  /* Vector header must start heap object. */
  ASSERT (clib_mem_is_heap_object (p));

  /* Typically we'll not need to resize. */
  if (new_data_bytes > clib_mem_size (p))
    goto slow_path;

  vh->len += length_increment;

  return v;

 slow_path:
  /* Slow path: call helper function. */
  return vec_resize_allocate_memory (v,
				     length_increment,
				     old_length,
				     n_bytes_per_elt,
				     header_bytes,
				     clib_max (sizeof (vec_header_t), data_align));
}

uword clib_mem_is_vec_h (void * v, uword header_bytes);

always_inline uword clib_mem_is_vec (void * v)
{ return clib_mem_is_vec_h (v, 0); }

/* Local variable naming macro (prevents collisions with other macro naming). */
#define _v(var) _vec_##var

/* Resize a vector (general version).
   Add N elements to end of given vector V, return pointer to start of vector.
   Vector will have room for H header bytes and will have user's data aligned
   at alignment A (rounded to next power of 2). */
#define vec_resize_ha(V,N,H,A)						\
do {									\
  word _v(resize_n) = (N);						\
  word _v(resize_l) = vec_len (V);					\
  V = _vec_resize ((V), _v(resize_n), _v(resize_l), sizeof ((V)[0]), (H), (A));	\
} while (0)

/* Resize a vector (unspecified alignment). */
#define vec_resize(V,N)     vec_resize_ha(V,N,0,0)
/* Resize a vector (aligned). */
#define vec_resize_aligned(V,N,A) vec_resize_ha(V,N,0,A)

/* Allocate space for N more elements but keep size the same (general version). */
#define vec_alloc_ha(V,N,H,A)			\
do {						\
    uword _v(alloc_l) = vec_len (V);		\
    vec_resize_ha (V, N, H, A);			\
    _vec_len (V) = _v(alloc_l);			\
} while (0)

/* Allocate space for N more elements but keep size the same (unspecified alignment) */
#define vec_alloc(V,N) vec_alloc_ha(V,N,0,0)
/* Allocate space for N more elements but keep size the same (alignment specified but no header) */
#define vec_alloc_aligned(V,N,A) vec_alloc_ha(V,N,0,A)

/* Create new vector of given type and length (general version). */
#define vec_new_ha(T,N,H,A)					\
({								\
  word _v(new_n) = (N);						\
  _vec_resize ((T *) 0, _v(new_n), 0, sizeof (T), (H), (A));	\
})

/* Create new vector of given type and length (unspecified alignment, no header). */
#define vec_new(T,N)           vec_new_ha(T,N,0,0)
/* Create new vector of given type and length (alignment specified but no header). */
#define vec_new_aligned(T,N,A) vec_new_ha(T,N,0,A)

/* Free vector's memory (general version). */
#define vec_free_h(V,H)				\
do {						\
  if (V)					\
    {						\
      clib_mem_free (vec_header ((V), (H)));	\
      V = 0;					\
    }						\
} while (0)

/* Free vector's memory (unspecified alignment, no header). */
#define vec_free(V) vec_free_h(V,0)
/* Free vector user header */
#define vec_free_header(h) clib_mem_free (h)

/* Return copy of vector (general version). */
#define vec_dup_ha(V,H,A)					\
({								\
  __typeof__ ((V)[0]) * _v(dup_v) = 0;				\
  uword _v(dup_l) = vec_len (V);				\
  if (_v(dup_l) > 0)						\
    {								\
      vec_resize_ha (_v(dup_v), _v(dup_l), (H), (A));		\
      memcpy (_v(dup_v), (V), _v(dup_l) * sizeof ((V)[0]));	\
    }								\
  _v(dup_v);							\
})

/* Return copy of vector (no alignment). */
#define vec_dup(V) vec_dup_ha(V,0,0)
/* Return copy of vector (alignment specified). */
#define vec_dup_aligned(V,A) vec_dup_ha(V,0,A)

/* Copy a vector */
#define vec_copy(DST,SRC) memcpy (DST, SRC, vec_len (DST) * sizeof ((DST)[0]))

/* Clone a vector

    Make a new vector with the same size as a given vector but
   possibly with a different type. */
#define vec_clone(NEW_V,OLD_V)			\
do {						\
  word _v(clone_l) = vec_len (OLD_V);		\
  (NEW_V) = 0;					\
  vec_resize ((NEW_V), _v(clone_l));		\
} while (0)

/* Make sure vector is long enough for given index (general version). */
#define vec_validate_ha(V,I,H,A)					\
do {									\
  word _v(validate_i) = (I);						\
  word _v(validate_l) = vec_len (V);					\
  if (_v(validate_i) >= _v(validate_l))					\
    {									\
      vec_resize_ha ((V), 1 + (_v(validate_i) - _v(validate_l)), (H), (A)); \
      /* Must zero new space since user may have previously		\
	 used e.g. _vec_len (v) -= 10 */				\
      memset ((V) + _v(validate_l), 0, (1 + (_v(validate_i) - _v(validate_l))) * sizeof ((V)[0])); \
    }									\
} while (0)

/* Make sure vector is long enough for given index (unspecified alignment). */
#define vec_validate(V,I)           vec_validate_ha(V,I,0,0)
/* Make sure vector is long enough for given index (alignment specified but no header). */
#define vec_validate_aligned(V,I,A) vec_validate_ha(V,I,0,A)

/* Make sure vector is long enough for given index and initialize empty space (general version). */
#define vec_validate_init_empty_ha(V,I,INIT,H,A)			\
do {									\
  word _v(validate_i) = (I);						\
  word _v(validate_l) = vec_len (V);					\
  if (_v(validate_i) >= _v(validate_l))					\
    {									\
      vec_resize_ha ((V), 1 + (_v(validate_i) - _v(validate_l)), (H), (A)); \
      while (_v(validate_l) <= _v(validate_i))				\
	{								\
	  (V)[_v(validate_l)] = (INIT);					\
	  _v(validate_l)++;						\
	}								\
    }									\
} while (0)

/* Make sure vector is long enough for given index and initialize empty space (unspecified alignment). */
#define vec_validate_init_empty(V,I,INIT) \
  vec_validate_init_empty_ha(V,I,INIT,0,0)
/* Make sure vector is long enough for given index and initialize empty space (alignment specified). */
#define vec_validate_init_empty_aligned(V,I,INIT,A) \
  vec_validate_init_empty_ha(V,I,INIT,0,A)

/* Add 1 element to end of vector (general version). */
#define vec_add1_ha(V,E,H,A)						\
do {									\
  word _v(add_l) = vec_len (V);						\
  V = _vec_resize ((V), 1, _v(add_l), sizeof ((V)[0]), (H), (A));	\
  (V)[_v(add_l)] = (E);							\
} while (0)

/* Add 1 element to end of vector (unspecified alignment). */
#define vec_add1(V,E)           vec_add1_ha(V,E,0,0)
/* Add 1 element to end of vector (alignment specified). */
#define vec_add1_aligned(V,E,A) vec_add1_ha(V,E,0,A)

/* Add N elements to end of vector V, return pointer to new elements in P. (general version) */
#define vec_add2_ha(V,P,N,H,A)						\
do {									\
  word _v(add_n) = (N);							\
  word _v(add_l) = vec_len (V);						\
  V = _vec_resize ((V), _v(add_n), _v(add_l), sizeof ((V)[0]), (H), (A)); \
  P = (V) + _v(add_l);							\
} while (0)

/* Add N elements to end of vector V, return pointer to new elements in P. (unspecified alignment) */
#define vec_add2(V,P,N)           vec_add2_ha(V,P,N,0,0)
/* Add N elements to end of vector V, return pointer to new elements in P. (alignment specified, no header) */
#define vec_add2_aligned(V,P,N,A) vec_add2_ha(V,P,N,0,A)

/* Add N elements to end of vector V (general version) */
#define vec_add_ha(V,E,N,H,A)						\
do {									\
  word _v(add_n) = (N);							\
  word _v(add_l) = vec_len (V);						\
  V = _vec_resize ((V), _v(add_n), _v(add_l), sizeof ((V)[0]), (H), (A));	\
  memcpy ((V) + _v(add_l), (E), _v(add_n) * sizeof ((V)[0]));			\
} while (0)

/* Add N elements to end of vector V (unspecified alignment) */
#define vec_add(V,E,N)           vec_add_ha(V,E,N,0,0)
/* Add N elements to end of vector V (alignment specified) */
#define vec_add_aligned(V,E,N,A) vec_add_ha(V,E,N,0,A)

/* Returns last element of a vector and decrements its length */
#define vec_pop(V)				\
({						\
  uword _v(pop_l) = vec_len (V);		\
  ASSERT (_v(pop_l) > 0);			\
  _v(pop_l) -= 1;				\
  _vec_len (V) = _v(pop_l);			\
  (V)[_v(pop_l)];				\
})

/* Set E to the last element of a vector, decrement vector length */
#define vec_pop2(V,E)				\
({						\
  uword _v(pop_l) = vec_len (V);		\
  if (_v(pop_l) > 0) (E) = vec_pop (V);		\
  _v(pop_l) > 0;				\
})

/* Resize vector by N elements starting from element M, initialize new elements (general version). */
#define vec_insert_init_empty_ha(V,N,M,INIT,H,A)			\
do {									\
  word _v(insert_l) = vec_len (V);					\
  word _v(insert_n) = (N);						\
  word _v(insert_m) = (M);						\
  vec_resize_ha ((V), _v(insert_n), (H), (A));				\
  ASSERT (_v(insert_m) <= _v(insert_l));				\
  memmove ((V) + _v(insert_m) + _v(insert_n),				\
	   (V) + _v(insert_m),						\
	   (_v(insert_l) - _v(insert_m)) * sizeof ((V)[0]));		\
  memset  ((V) + _v(insert_m), INIT, _v(insert_n) * sizeof ((V)[0]));	\
} while (0)

/* Resize vector by N elements starting from element M, initialize new elements to zero (general version). */
#define vec_insert_ha(V,N,M,H,A)    vec_insert_init_empty_ha(V,N,M,0,H,A)
/* Resize vector by N elements starting from element M, initialize new elements to zero (unspecified alignment, no header). */
#define vec_insert(V,N,M)           vec_insert_ha(V,N,M,0,0)
/* Resize vector by N elements starting from element M, initialize new elements to zero (alignment specified, no header). */
#define vec_insert_aligned(V,N,M,A) vec_insert_ha(V,N,M,0,A)

/* Resize vector by N elements starting from element M, initialize new elements to INIT (unspecified alignment, no header). */
#define vec_insert_init_empty(V,N,M,INIT) \
  vec_insert_init_empty_ha(V,N,M,INIT,0,0)
/* Resize vector by N elements starting from element M, initialize new elements to INIT (alignment specified, no header). */
#define vec_insert_init_empty_aligned(V,N,M,INIT,A) \
  vec_insert_init_empty_ha(V,N,M,INIT,0,A)

/* Resize vector by N elements starting from element M. Insert given elements (general version). */
#define vec_insert_elts_ha(V,E,N,M,H,A)					\
do {									\
  word _v(insert_l) = vec_len (V);					\
  word _v(insert_n) = (N);						\
  word _v(insert_m) = (M);						\
  vec_resize_ha ((V), _v(insert_n), (H), (A));				\
  ASSERT (_v(insert_m) <= _v(insert_l));				\
  memmove ((V) + _v(insert_m) + _v(insert_n),				\
	   (V) + _v(insert_m),						\
	   (_v(insert_l) - _v(insert_m)) * sizeof ((V)[0]));		\
  memcpy  ((V) + _v(insert_m), (E), _v(insert_n) * sizeof ((V)[0]));	\
} while (0)

/* Resize vector by N elements starting from element M. Insert given elements (unspecified alignment, no header). */
#define vec_insert_elts(V,E,N,M)           vec_insert_elts_ha(V,E,N,M,0,0)
/* Resize vector by N elements starting from element M. Insert given elements (alignment specified, no header). */
#define vec_insert_elts_aligned(V,E,N,M,A) vec_insert_elts_ha(V,E,N,M,0,A)

/* Delete N elements starting from element M */
#define vec_delete(V,N,M)						\
do {									\
  word _v(delete_l) = vec_len (V);					\
  word _v(delete_n) = (N);						\
  word _v(delete_m) = (M);						\
  /* Copy over deleted elements. */					\
  if (_v(delete_l) - _v(delete_n) - _v(delete_m) > 0)			\
    memmove ((V) + _v(delete_m), (V) + _v(delete_m) + _v(delete_n),	\
	     (_v(delete_l) - _v(delete_n) - _v(delete_m)) * sizeof ((V)[0])); \
  /* Zero empty space at end (for future re-allocation). */		\
  if (_v(delete_n) > 0)							\
    memset ((V) + _v(delete_l) - _v(delete_n), 0, _v(delete_n) * sizeof ((V)[0])); \
  _vec_len (V) -= _v(delete_n);						\
} while (0)

/* Delete a single element at index I. */
#define vec_del1(v,i)				\
do {						\
  uword _v (delete_l) = _vec_len (v) - 1;	\
  uword _v (delete_i) = (i);			\
  if (_v (delete_i) < _v (delete_l))		\
    (v)[_v (delete_i)] = (v)[_v (delete_l];	\
  _vec_len (v) = _v (delete_l);			\
} while (0)

/* Appends v2 after v1. Result in v1. */
#define vec_append(v1,v2)					\
do {								\
  uword _v(l1) = vec_len (v1);					\
  uword _v(l2) = vec_len (v2);					\
  vec_resize ((v1), _v(l2));					\
  memcpy ((v1) + _v(l1), (v2), _v(l2) * sizeof ((v2)[0]));	\
} while (0)

/* Prepends v2 in front of v1. Result in v1. */
#define vec_prepend(v1,v2)					\
do {								\
  uword _v(l1) = vec_len (v1);					\
  uword _v(l2) = vec_len (v2);					\
  vec_resize ((v1), _v(l2));					\
  memmove ((v1) + _v(l2), (v1), _v(l1) * sizeof ((v1)[0]));	\
  memcpy ((v1), (v2), _v(l2) * sizeof ((v2)[0]));		\
} while (0)

/* Zero all elements. */
#define vec_zero(var)						\
do {								\
  if (var)							\
    memset ((var), 0, vec_len (var) * sizeof ((var)[0]));	\
} while (0)

/* Set all elements to given value. */
#define vec_set(v,val)				\
do {						\
  word _v(i);					\
  __typeof__ ((v)[0]) _val = (val);		\
  for (_v(i) = 0; _v(i) < vec_len (v); _v(i)++)	\
    (v)[_v(i)] = _val;				\
} while (0)

#ifdef CLIB_UNIX
#include <stdlib.h>		/* for qsort */
#endif

/* Compare two vectors. */
#define vec_is_equal(v1,v2) \
  (vec_len (v1) == vec_len (v2) && ! memcmp ((v1), (v2), vec_len (v1) * sizeof ((v1)[0])))

/* Compare two vectors (only applicable to vectors of signed numbers).

   Used in qsort compare functions. */
#define vec_cmp(v1,v2)					\
({							\
  word _v(i), _v(cmp), _v(l);				\
  _v(l) = clib_min (vec_len (v1), vec_len (v2));	\
  _v(cmp) = 0;						\
  for (_v(i) = 0; _v(i) < _v(l); _v(i)++) {		\
    _v(cmp) = (v1)[_v(i)] - (v2)[_v(i)];		\
    if (_v(cmp))					\
      break;						\
  }							\
  if (_v(cmp) == 0 && _v(l) > 0)			\
    _v(cmp) = vec_len(v1) - vec_len(v2);		\
  (_v(cmp) < 0 ? -1 : (_v(cmp) > 0 ? +1 : 0));		\
})

/* Sort a vector with qsort via user's comparison body

   Example to sort an integer vector:
     int * int_vec = ...;
     vec_sort (int_vec, i0, i1, i0[0] - i1[0]);
*/
#define vec_sort(vec,cmp_function)				\
do {								\
 qsort (vec, vec_len (vec), sizeof (vec[0]), cmp_function);	\
} while (0)

/* Sort a vector using the supplied element comparison function

    A simple qsort wrapper */
#define vec_sort_with_function(vec,f)				\
do {								\
  qsort (vec, vec_len (vec), sizeof (vec[0]), (void *) (f));	\
} while (0)

#endif /* included_vec_h */

