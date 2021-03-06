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

#ifndef included_vm_unix_h
#define included_vm_unix_h

#include <unistd.h>
#include <sys/mman.h>

always_inline uword
clib_mem_vm_mmap_base_flags ()
{
  uword flags = MAP_PRIVATE;

#if defined (MAP_ANONYMOUS)
  flags |= MAP_ANONYMOUS;
#elif defined (MAP_ANON)
  flags |= MAP_ANON;
#endif

#if defined (MAP_NORESERVE)
  flags |= MAP_NORESERVE;
#endif

  return flags;
}

/* Allocate virtual address space. */
always_inline void * clib_mem_vm_alloc (uword size)
{
  void * mmap_addr;
  uword flags = clib_mem_vm_mmap_base_flags ();

  mmap_addr = mmap (0, size, PROT_READ | PROT_WRITE, flags, -1, 0);
  if (mmap_addr == (void *) -1)
    mmap_addr = 0;

  return mmap_addr;
}

always_inline void clib_mem_vm_free (void * addr, uword size)
{ munmap (addr, size); }

always_inline void * clib_mem_vm_unmap (void * addr, uword size)
{
  void * mmap_addr;
  uword flags = clib_mem_vm_mmap_base_flags () | MAP_FIXED;

  /* To unmap we "map" with no protection.  If we actually called
     munmap then other callers could steal the address space.  By
     changing to PROT_NONE the kernel can free up the pages which is
     really what we want "unmap" to mean. */
  mmap_addr = mmap (addr, size, PROT_NONE, flags, -1, 0);
  if (mmap_addr == (void *) -1)
    mmap_addr = 0;

  return mmap_addr;
}

always_inline void * clib_mem_vm_map (void * addr, uword size)
{
  void * mmap_addr;
  uword flags = clib_mem_vm_mmap_base_flags () | MAP_FIXED;

  mmap_addr = mmap (addr, size, (PROT_READ | PROT_WRITE), flags, -1, 0);
  if (mmap_addr == (void *) -1)
    mmap_addr = 0;

  return mmap_addr;
}

always_inline uword clib_mem_get_page_size (void)
{ return getpagesize (); }

#endif /* included_vm_unix_h */
