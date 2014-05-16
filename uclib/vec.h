#ifndef included_uclib_vec_h
#define included_uclib_vec_h

#include <string.h>		/* for memset etc. */

/* Bookeeping header preceding vector elements in memory.
   User header information may preceed standard vec header. */
typedef struct {
  /* Number of elements in vector (NOT its allocated length). */
  u32 len;

  /* Vector data follows. */
  u8 vector_data[0];
} vec_header_t;

/* Given the user's pointer to a vector, find the corresponding vector header. */
#define _vec_get_header(v) ((vec_header_t *) (v) - 1)

#define _vec_round_size(s) \
  (((s) + sizeof (uword) - 1) &~ (sizeof (uword) - 1))

always_inline uword
vec_header_bytes (uword header_bytes)
{ return round_pow2 (header_bytes + sizeof (vec_header_t), sizeof (vec_header_t)); }

always_inline void *
vec_header (void * v, uword header_bytes)
{ return v - vec_header_bytes (header_bytes); }

always_inline void *
vec_header_end (void * v, uword header_bytes)
{ return v + vec_header_bytes (header_bytes); }

always_inline uword
vec_aligned_header_bytes (uword header_bytes, uword align)
{
  return round_pow2 (header_bytes + sizeof (vec_header_t), align);
}

always_inline void *
vec_aligned_header (void * v, uword header_bytes, uword align)
{ return v - vec_aligned_header_bytes (header_bytes, align); }

always_inline void *
vec_aligned_header_end (void * v, uword header_bytes, uword align)
{ return v + vec_aligned_header_bytes (header_bytes, align); }

/* Number of elements in vector (lvalue-capable).
   _vec_len (v) does not check for null, but can be used as a lvalue
   (e.g. _vec_len (v) = 99). */
#define _vec_len(v)	(_vec_get_header(v)->len)

/* Number of elements in vector (rvalue-only, NULL tolerant)
   vec_len (v) checks for NULL, but cannot be used as an lvalue.
   If in doubt, use vec_len... */
#define vec_len(v)	((v) ? _vec_len(v) : 0)

/* Number of data bytes in vector. */
#define vec_bytes(v) (vec_len (v) * sizeof (v[0]))

/* Total number of bytes that can fit in vector with current allocation. */
#define vec_capacity(v,b)							\
({										\
  void * _vec_capacity_v = (void *) (v);					\
  uword _vec_capacity_b = (b);							\
  _vec_capacity_b = sizeof (vec_header_t) + _vec_round_size (_vec_capacity_b);	\
  _vec_capacity_v ? clib_mem_size (_vec_capacity_v - _vec_capacity_b) : 0;	\
})

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

  vh = _vec_get_header (v);

  if (! v)
    goto slow_path;

  p = v - aligned_header_bytes;

  /* Typically we'll not need to resize. */
  if (new_data_bytes > clib_mem_size (p))
    goto slow_path;

  vh->len += length_increment;

  return v;

 slow_path:
  /* Slow path: call helper function. */
  data_align = data_align > sizeof (vec_header_t) ? data_align : sizeof (vec_header_t);
  return vec_resize_allocate_memory (v,
				     length_increment,
				     old_length,
				     n_bytes_per_elt,
				     header_bytes,
				     data_align);
}

/* Local variable naming macro (prevents collisions with other macro naming). */
#define _vec_var(var) _vec_##var

/* Resize a vector (general version).
   Add N elements to end of given vector V, return pointer to start of vector.
   Vector will have room for H header bytes and will have user's data aligned
   at alignment A (rounded to next power of 2). */
#define vec_resize_ha(V,N,H,A)						\
do {									\
  word _vec_var(resize_n) = (N);					\
  word _vec_var(resize_l) = vec_len (V);				\
  V = _vec_resize ((V), _vec_var(resize_n), _vec_var(resize_l), sizeof ((V)[0]), (H), (A)); \
} while (0)

/* Resize a vector (unspecified alignment). */
#define vec_resize(V,N)     vec_resize_ha(V,N,0,0)
/* Resize a vector (aligned). */
#define vec_resize_aligned(V,N,A) vec_resize_ha(V,N,0,A)

/* Allocate space for N more elements but keep size the same (general version). */
#define vec_alloc_ha(V,N,H,A)			\
do {						\
    uword _vec_var(alloc_l) = vec_len (V);		\
    vec_resize_ha (V, N, H, A);			\
    _vec_len (V) = _vec_var(alloc_l);			\
} while (0)

/* Allocate space for N more elements but keep size the same (unspecified alignment) */
#define vec_alloc(V,N) vec_alloc_ha(V,N,0,0)
/* Allocate space for N more elements but keep size the same (alignment specified but no header) */
#define vec_alloc_aligned(V,N,A) vec_alloc_ha(V,N,0,A)

/* Create new vector of given type and length (general version). */
#define vec_new_ha(T,N,H,A)					\
({								\
  word _vec_var(new_n) = (N);						\
  _vec_resize ((T *) 0, _vec_var(new_n), 0, sizeof (T), (H), (A));	\
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
  __typeof__ ((V)[0]) * _vec_var(dup_v) = 0;				\
  uword _vec_var(dup_l) = vec_len (V);				\
  if (_vec_var(dup_l) > 0)						\
    {								\
      vec_resize_ha (_vec_var(dup_v), _vec_var(dup_l), (H), (A));		\
      memcpy (_vec_var(dup_v), (V), _vec_var(dup_l) * sizeof ((V)[0]));	\
    }								\
  _vec_var(dup_v);							\
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
  word _vec_var(clone_l) = vec_len (OLD_V);		\
  (NEW_V) = 0;					\
  vec_resize ((NEW_V), _vec_var(clone_l));		\
} while (0)

/* Make sure vector is long enough for given index (general version). */ \
#define vec_validate_ha(V,I,H,A)					\
do {									\
  word _vec_var(validate_i) = (I);					\
  word _vec_var(validate_l) = vec_len (V);				\
  if (_vec_var(validate_i) >= _vec_var(validate_l))			\
    {									\
      vec_resize_ha ((V), 1 + (_vec_var(validate_i) - _vec_var(validate_l)), (H), (A)); \
      /* Must zero new space since user may have previously		\
	 used e.g. _vec_len (v) -= 10 */				\
      memset ((V) + _vec_var(validate_l), 0, (1 + (_vec_var(validate_i) - _vec_var(validate_l))) * sizeof ((V)[0])); \
    }									\
} while (0)

/* Make sure vector is long enough for given index (unspecified alignment). */
#define vec_validate(V,I)           vec_validate_ha(V,I,0,0)
/* Make sure vector is long enough for given index (alignment specified but no header). */
#define vec_validate_aligned(V,I,A) vec_validate_ha(V,I,0,A)

/* Make sure vector is long enough for given index and initialize empty space (general version). */
#define vec_validate_init_empty_ha(V,I,INIT,H,A)			\
do {									\
  word _vec_var(validate_i) = (I);					\
  word _vec_var(validate_l) = vec_len (V);				\
  if (_vec_var(validate_i) >= _vec_var(validate_l))			\
    {									\
      vec_resize_ha ((V), 1 + (_vec_var(validate_i) - _vec_var(validate_l)), (H), (A)); \
      while (_vec_var(validate_l) <= _vec_var(validate_i))		\
	{								\
	  (V)[_vec_var(validate_l)] = (INIT);				\
	  _vec_var(validate_l)++;					\
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
  word _vec_var(add_l) = vec_len (V);						\
  V = _vec_resize ((V), 1, _vec_var(add_l), sizeof ((V)[0]), (H), (A));	\
  (V)[_vec_var(add_l)] = (E);							\
} while (0)

/* Add 1 element to end of vector (unspecified alignment). */
#define vec_add1(V,E)           vec_add1_ha(V,E,0,0)
/* Add 1 element to end of vector (alignment specified). */
#define vec_add1_aligned(V,E,A) vec_add1_ha(V,E,0,A)

/* Add N elements to end of vector V, return pointer to new elements in P. (general version) */
#define vec_add2_ha(V,P,N,H,A)						\
do {									\
  word _vec_var(add_n) = (N);							\
  word _vec_var(add_l) = vec_len (V);						\
  V = _vec_resize ((V), _vec_var(add_n), _vec_var(add_l), sizeof ((V)[0]), (H), (A)); \
  P = (V) + _vec_var(add_l);							\
} while (0)

/* Add N elements to end of vector V, return pointer to new elements in P. (unspecified alignment) */
#define vec_add2(V,P,N)           vec_add2_ha(V,P,N,0,0)
/* Add N elements to end of vector V, return pointer to new elements in P. (alignment specified, no header) */
#define vec_add2_aligned(V,P,N,A) vec_add2_ha(V,P,N,0,A)

/* Add N elements to end of vector V (general version) */
#define vec_add_ha(V,E,N,H,A)						\
do {									\
  word _vec_var(add_n) = (N);							\
  word _vec_var(add_l) = vec_len (V);						\
  V = _vec_resize ((V), _vec_var(add_n), _vec_var(add_l), sizeof ((V)[0]), (H), (A));	\
  memcpy ((V) + _vec_var(add_l), (E), _vec_var(add_n) * sizeof ((V)[0]));			\
} while (0)

/* Add N elements to end of vector V (unspecified alignment) */
#define vec_add(V,E,N)           vec_add_ha(V,E,N,0,0)
/* Add N elements to end of vector V (alignment specified) */
#define vec_add_aligned(V,E,N,A) vec_add_ha(V,E,N,0,A)

/* End (last data address) of vector. */
#define vec_end(v)	((v) + vec_len (v))

/* True if given pointer is within given vector. */
#define vec_is_member(v,e) ((e) >= (v) && (e) < vec_end (v))

/* Get vector element at index i checking that i is in bounds. */
#define vec_elt_at_index(v,i)			\
({						\
  ASSERT ((i) < vec_len (v));			\
  (v) + (i);					\
})

/* Get vector value at index i */
#define vec_elt(v,i) (vec_elt_at_index(v,i))[0]

/* Vector iterator */
#define vec_foreach(var,vec) for (var = (vec); var < vec_end (vec); var++)

/* Iterate over vector indices. */
#define vec_foreach_index(var,v) for ((var) = 0; (var) < vec_len (v); (var)++)

/* Resize vector by N elements starting from element M, initialize new elements (general version). */
#define vec_insert_init_empty_ha(V,N,M,INIT,H,A)			\
do {									\
  word _vec_var(insert_l) = vec_len (V);					\
  word _vec_var(insert_n) = (N);						\
  word _vec_var(insert_m) = (M);						\
  vec_resize_ha ((V), _vec_var(insert_n), (H), (A));				\
  ASSERT (_vec_var(insert_m) <= _vec_var(insert_l));				\
  memmove ((V) + _vec_var(insert_m) + _vec_var(insert_n),				\
	   (V) + _vec_var(insert_m),						\
	   (_vec_var(insert_l) - _vec_var(insert_m)) * sizeof ((V)[0]));		\
  memset  ((V) + _vec_var(insert_m), INIT, _vec_var(insert_n) * sizeof ((V)[0]));	\
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
  word _vec_var(insert_l) = vec_len (V);					\
  word _vec_var(insert_n) = (N);						\
  word _vec_var(insert_m) = (M);						\
  vec_resize_ha ((V), _vec_var(insert_n), (H), (A));				\
  ASSERT (_vec_var(insert_m) <= _vec_var(insert_l));				\
  memmove ((V) + _vec_var(insert_m) + _vec_var(insert_n),				\
	   (V) + _vec_var(insert_m),						\
	   (_vec_var(insert_l) - _vec_var(insert_m)) * sizeof ((V)[0]));		\
  memcpy  ((V) + _vec_var(insert_m), (E), _vec_var(insert_n) * sizeof ((V)[0]));	\
} while (0)

/* Resize vector by N elements starting from element M. Insert given elements (unspecified alignment, no header). */
#define vec_insert_elts(V,E,N,M)           vec_insert_elts_ha(V,E,N,M,0,0)
/* Resize vector by N elements starting from element M. Insert given elements (alignment specified, no header). */
#define vec_insert_elts_aligned(V,E,N,M,A) vec_insert_elts_ha(V,E,N,M,0,A)

/* Delete N elements starting from element M */
#define vec_delete(V,N,M)						\
do {									\
  word _vec_var(delete_l) = vec_len (V);					\
  word _vec_var(delete_n) = (N);						\
  word _vec_var(delete_m) = (M);						\
  /* Copy over deleted elements. */					\
  if (_vec_var(delete_l) - _vec_var(delete_n) - _vec_var(delete_m) > 0)			\
    memmove ((V) + _vec_var(delete_m), (V) + _vec_var(delete_m) + _vec_var(delete_n),	\
	     (_vec_var(delete_l) - _vec_var(delete_n) - _vec_var(delete_m)) * sizeof ((V)[0])); \
  /* Zero empty space at end (for future re-allocation). */		\
  if (_vec_var(delete_n) > 0)							\
    memset ((V) + _vec_var(delete_l) - _vec_var(delete_n), 0, _vec_var(delete_n) * sizeof ((V)[0])); \
  _vec_len (V) -= _vec_var(delete_n);						\
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
  uword _vec_var(l1) = vec_len (v1);					\
  uword _vec_var(l2) = vec_len (v2);					\
  vec_resize ((v1), _vec_var(l2));					\
  memcpy ((v1) + _vec_var(l1), (v2), _vec_var(l2) * sizeof ((v2)[0]));	\
} while (0)

/* Prepends v2 in front of v1. Result in v1. */
#define vec_prepend(v1,v2)					\
do {								\
  uword _vec_var(l1) = vec_len (v1);					\
  uword _vec_var(l2) = vec_len (v2);					\
  vec_resize ((v1), _vec_var(l2));					\
  memmove ((v1) + _vec_var(l2), (v1), _vec_var(l1) * sizeof ((v1)[0]));	\
  memcpy ((v1), (v2), _vec_var(l2) * sizeof ((v2)[0]));		\
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
  word _vec_var(i);					\
  __typeof__ ((v)[0]) _val = (val);		\
  for (_vec_var(i) = 0; _vec_var(i) < vec_len (v); _vec_var(i)++)	\
    (v)[_vec_var(i)] = _val;				\
} while (0)

#endif /* included_uclib_vec_h */
