#ifndef included_uclib_mem_h
#define included_uclib_mem_h

#include <malloc.h>

always_inline uword clib_mem_size (void * v)
{ return malloc_usable_size (v); }

always_inline void clib_mem_free (void * v)
{ free (v); }

always_inline void *
clib_mem_alloc_aligned_at_offset (uword size,
				  uword align,
				  uword align_offset)
{
  void * p;

  if (align_offset > align)
    {
      if (align > 0)
	align_offset %= align;
      else
	align_offset = align;
    }

  p = memalign (align, size);

  if (! p)
    os_panic ();

  return p;
}

#endif /* included_uclib_mem_h */
