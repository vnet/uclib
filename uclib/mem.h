#ifndef included_uclib_mem_h
#define included_uclib_mem_h

/* Can be used as either {r,l}value, e.g. these both work
     clib_mem_unaligned (p, u64) = 99
     clib_mem_unaligned (p, u64) += 99 */

#define clib_mem_unaligned(pointer,type) \
  (((struct { CLIB_PACKED (type _data); } *) (pointer))->_data)

/* Access memory with specified alignment depending on align argument.
   As with clib_mem_unaligned, may be used as {r,l}value. */
#define clib_mem_aligned(addr,type,align)		\
  (((struct {						\
       type _data					\
       __attribute__ ((aligned (align), packed));	\
    } *) (addr))->_data)

#include <string.h>

/* Per CPU heaps. */
extern void * clib_per_cpu_mheaps[32];

always_inline void * clib_mem_get_per_cpu_heap (void)
{
  int cpu = os_get_cpu_number ();
  return clib_per_cpu_mheaps[cpu];
}

always_inline void * clib_mem_set_per_cpu_heap (u8 * new)
{
  int cpu = os_get_cpu_number ();
  void * old = clib_per_cpu_mheaps[cpu];
  clib_per_cpu_mheaps[cpu] = new;
  return old;
}

/* Memory allocator which returns null when it fails. */
always_inline void *
clib_mem_alloc_aligned_at_offset (uword size,
				  uword align,
				  uword align_offset)
{
  void * heap;
  uword offset, cpu;

  if (align_offset > align)
    {
      if (align > 0)
	align_offset %= align;
      else
	align_offset = align;
    }

  cpu = os_get_cpu_number ();
  heap = clib_per_cpu_mheaps[cpu];
  heap = mheap_get_aligned (heap,
			    size, align, align_offset,
			    &offset);
  clib_per_cpu_mheaps[cpu] = heap;

  if (offset != ~0)
    {
      return heap + offset;
    }
  else
    {
      os_panic ();
      return 0;
    }
}

/* Memory allocator which returns null when it fails. */
always_inline void *
clib_mem_alloc (uword size)
{ return clib_mem_alloc_aligned_at_offset (size, /* align */ 1, /* align_offset */ 0); }

always_inline void *
clib_mem_alloc_aligned (uword size, uword align)
{ return clib_mem_alloc_aligned_at_offset (size, align, /* align_offset */ 0); }

/* Memory allocator which panics when it fails.
   Use macro so that clib_panic macro can expand __FUNCTION__ and __LINE__. */
#define clib_mem_alloc_aligned_no_fail(size,align)				\
({										\
  uword _clib_mem_alloc_size = (size);						\
  void * _clib_mem_alloc_p;							\
  _clib_mem_alloc_p = clib_mem_alloc_aligned (_clib_mem_alloc_size, (align));	\
  if (! _clib_mem_alloc_p)							\
    clib_panic ("failed to allocate %d bytes", _clib_mem_alloc_size);		\
  _clib_mem_alloc_p;								\
})

#define clib_mem_alloc_no_fail(size) clib_mem_alloc_aligned_no_fail(size,1)

/* Alias to stack allocator for naming consistency. */
#define clib_mem_alloc_stack(bytes) __builtin_alloca(bytes)

always_inline uword clib_mem_is_heap_object (void * p)
{
  void * heap = clib_mem_get_per_cpu_heap ();
  uword offset = p - heap;
  mheap_elt_t * e, * n;

  if (offset >= vec_len (heap))
    return 0;

  e = mheap_elt_at_uoffset (heap, offset);
  n = mheap_next_elt (e);
  
  /* Check that heap forward and reverse pointers agree. */
  return e->n_user_data == n->prev_n_user_data;
}

always_inline void clib_mem_free (void * p)
{
  u8 * heap = clib_mem_get_per_cpu_heap ();

  /* Make sure object is in the correct heap. */
  ASSERT (clib_mem_is_heap_object (p));

  mheap_put (heap, (u8 *) p - heap);
}

always_inline void * clib_mem_realloc (void * p, uword new_size, uword old_size)
{
  /* By default use alloc, copy and free to emulate realloc. */
  void * q = clib_mem_alloc (new_size);
  if (q)
    {
      uword copy_size;
      if (old_size < new_size)
	copy_size = old_size;
      else
	copy_size = new_size;
      memcpy (q, p, copy_size);
      clib_mem_free (p);
    }
  return q;
}

always_inline uword clib_mem_size (void * p)
{
  void * heap = clib_mem_get_per_cpu_heap ();
  ASSERT (clib_mem_is_heap_object (p));
  return mheap_data_bytes (heap, p - heap);
}

always_inline void * clib_mem_get_heap (void)
{ return clib_mem_get_per_cpu_heap (); }

always_inline void * clib_mem_set_heap (void * heap)
{ return clib_mem_set_per_cpu_heap (heap); }

void * clib_mem_init (void * heap, uword size);

void clib_mem_exit (void);

always_inline uword clib_mem_get_page_size (void);

void clib_mem_validate (void);

void clib_mem_trace (int enable);

typedef struct {
  /* Total number of objects allocated. */
  uword object_count;

  /* Total allocated bytes.  Bytes used and free.
     used + free = total */
  uword bytes_total, bytes_used, bytes_free;

  /* Number of bytes used by mheap data structure overhead
     (e.g. free lists, mheap header). */
  uword bytes_overhead;

  /* Amount of free space returned to operating system. */
  uword bytes_free_reclaimed;
  
  /* For malloc which puts small objects in sbrk region and
     large objects in mmap'ed regions. */
  uword bytes_used_sbrk;
  uword bytes_used_mmap;

  /* Max. number of bytes in this heap. */
  uword bytes_max;
} clib_mem_usage_t;

void clib_mem_usage (clib_mem_usage_t * usage);

#endif /* included_uclib_mem_h */
