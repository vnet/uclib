/*
  Copyright (c) 2001-2005 Eliot Dresselhaus

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

#ifndef included_clib_smp_bootstrap_h
#define included_clib_smp_bootstrap_h

/* Per-CPU state. */
typedef struct {
  /* Per-cpu local heap. */
  void * heap;

  /* Index into per_cpu_mains (equal to cpu number). */
  u32 index;

  u32 thread_id;

  /* OS specific stuff goes here (e.g. pthread_t). */
  uword opaque;
} clib_smp_per_cpu_main_t;

typedef struct {
  /* Number of CPUs (threads) used to model current computer.
     This never includes the main thread (cpu index == n_cpus). */
  u32 n_cpus;

  /* Log2 stack and vm (heap) size. */
  u8 log2_n_per_cpu_stack_bytes, log2_n_per_cpu_vm_bytes;

  /* Per cpus stacks/heaps start at these addresses. */
  void * vm_base;

  /* Thread-safe global heap.  Objects here can be allocated/freed by any cpu. */
  void * global_heap;

  clib_smp_per_cpu_main_t * per_cpu_mains;
} clib_smp_main_t;

extern clib_smp_main_t clib_smp_main;

#endif /* included_clib_smp_bootstrap_h */
