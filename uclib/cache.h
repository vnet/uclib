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

#ifndef included_clib_cache_h
#define included_clib_cache_h

#ifdef __x86_64__
#define CLIB_LOG2_CACHE_LINE_BYTES 6
#endif

/* Default cache line size of 32 bytes. */
#ifndef CLIB_LOG2_CACHE_LINE_BYTES
#define CLIB_LOG2_CACHE_LINE_BYTES 5
#endif

#define CLIB_CACHE_LINE_BYTES (1 << CLIB_LOG2_CACHE_LINE_BYTES)

/* Read/write arguments to __builtin_prefetch. */
#define CLIB_PREFETCH_READ 0
#define CLIB_PREFETCH_LOAD 0	/* alias for read */
#define CLIB_PREFETCH_WRITE 1
#define CLIB_PREFETCH_STORE 1	/* alias for write */

#define _CLIB_PREFETCH(n,size,type)				\
  if ((size) > (n)*CLIB_CACHE_LINE_BYTES)			\
    __builtin_prefetch (_addr + (n)*CLIB_CACHE_LINE_BYTES,	\
			CLIB_PREFETCH_##type,			\
			/* locality */ 1);

#define CLIB_PREFETCH(addr,size,type)		\
do {						\
  void * _addr = (addr);			\
						\
  ASSERT ((size) <= 4*CLIB_CACHE_LINE_BYTES);	\
  _CLIB_PREFETCH (0, size, type);		\
  _CLIB_PREFETCH (1, size, type);		\
  _CLIB_PREFETCH (2, size, type);		\
  _CLIB_PREFETCH (3, size, type);		\
} while (0)

#undef _

#endif /* included_clib_cache_h */

