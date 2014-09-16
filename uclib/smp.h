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

#ifndef included_clib_smp_h
#define included_clib_smp_h

always_inline void *
clib_smp_vm_base_for_cpu (clib_smp_main_t * m, uword cpu)
{
  return m->vm_base + (cpu << m->log2_n_per_cpu_vm_bytes);
}

always_inline void *
clib_smp_stack_start_for_cpu (clib_smp_main_t * m, uword cpu)
{
  /* Stack is at top of per cpu VM area. */
  return clib_smp_vm_base_for_cpu (m, cpu + 1) - ((uword) 1 << m->log2_n_per_cpu_stack_bytes);
}

always_inline clib_smp_per_cpu_main_t *
clib_smp_get_per_cpu_main_for_cpu (uword for_cpu)
{ return clib_smp_main.per_cpu_mains + for_cpu; }

always_inline clib_smp_per_cpu_main_t *
clib_smp_get_per_cpu_main (uword for_cpu)
{ return clib_smp_main.per_cpu_mains + os_get_cpu_number (); }

#define clib_smp_compare_and_swap(addr,new,old) __sync_val_compare_and_swap(addr,old,new)
#define clib_smp_swap(addr,new) __sync_lock_test_and_set(addr,new)
#define clib_smp_atomic_add(addr,increment) __sync_fetch_and_add(addr,increment)

#if defined (i386) || defined (__x86_64__)
#define clib_smp_pause() do { asm volatile ("pause"); } while (0)
#endif

#ifndef clib_smp_pause
#define clib_smp_pause() do { } while (0)
#endif

#ifdef CLIB_UNIX
#include <sched.h>

always_inline void
os_sched_yield (void)
{ sched_yield (); }
#else
always_inline void
os_sched_yield (void)
{ clib_smp_pause (); }
#endif

typedef enum {
  CLIB_SMP_LOCK_TYPE_READER,
  CLIB_SMP_LOCK_TYPE_WRITER,
  CLIB_SMP_LOCK_TYPE_SPIN,
} clib_smp_lock_type_t;

typedef enum {
  CLIB_SMP_LOCK_WAIT_EMPTY,
  CLIB_SMP_LOCK_WAIT_DONE,
  CLIB_SMP_LOCK_WAIT_READER,
  CLIB_SMP_LOCK_WAIT_WRITER,
} clib_smp_lock_wait_type_t;

#if uword_bits == 64
typedef u16 clib_smp_quarter_word_t;
typedef u32 clib_smp_half_word_t;
#else
typedef u8 clib_smp_quarter_word_t;
typedef u16 clib_smp_half_word_t;
#endif

typedef union {
  struct {
    /* FIFO of CPUs (threads) waiting for lock. */
    struct {
      clib_smp_quarter_word_t head_index, n_elts;
    } waiting_fifo;

    /* Requesting CPU for atomic compare_and_swap instructions.
       This makes CPUs requesting same header change unique. */
    clib_smp_quarter_word_t request_cpu;

    /* Count of readers who have been given read lock.
       Not applicable for spin locks. */
    clib_smp_quarter_word_t n_readers_with_lock : BITS (clib_smp_quarter_word_t) - 1;

    /* Set when writer has been given write lock.  Only one of
       these can happen at a time. */
    clib_smp_quarter_word_t writer_has_lock : 1;
  };

  uword as_uword;
} clib_smp_lock_header_t;

always_inline uword
clib_smp_lock_header_is_equal (clib_smp_lock_header_t h0, clib_smp_lock_header_t h1)
{ return h0.as_uword == h1.as_uword; }

typedef struct {
  volatile clib_smp_lock_wait_type_t wait_type;
  u8 pad[CLIB_CACHE_LINE_BYTES - 1 * sizeof (clib_smp_lock_wait_type_t)];
} clib_smp_lock_waiting_fifo_elt_t;

/* Cache aligned. */
typedef struct {
  clib_smp_lock_header_t header;

  /* Size of waiting FIFO; equal to max number of threads less one. */
  u32 n_waiting_fifo_elts;

  u8 pad[CLIB_CACHE_LINE_BYTES - sizeof (clib_smp_lock_header_t) - sizeof (u32)];

  clib_smp_lock_waiting_fifo_elt_t waiting_fifo[0];
} clib_smp_lock_t;

always_inline clib_smp_lock_header_t
clib_smp_lock_set_header (clib_smp_lock_t * l, clib_smp_lock_header_t new, clib_smp_lock_header_t old)
{
  clib_smp_lock_header_t cmp;
  cmp.as_uword = clib_smp_compare_and_swap (&l->header.as_uword, new.as_uword, old.as_uword);
  return cmp;
}

void clib_smp_lock_init (clib_smp_lock_t ** l);
void clib_smp_lock_free (clib_smp_lock_t ** l);
void clib_smp_lock_slow_path (clib_smp_lock_t * l, uword my_cpu, clib_smp_lock_header_t h, clib_smp_lock_type_t type);
void clib_smp_unlock_slow_path (clib_smp_lock_t * l, uword my_cpu, clib_smp_lock_header_t h, clib_smp_lock_type_t type);

always_inline void
clib_smp_lock_inline (clib_smp_lock_t * l, clib_smp_lock_type_t type)
{
  clib_smp_lock_header_t h0, h1, h2;
  uword is_reader = type == CLIB_SMP_LOCK_TYPE_READER;
  uword my_cpu;

  /* Null lock means n_cpus <= 1: nothing to lock. */
  if (! l)
    return;

  my_cpu = os_get_cpu_number ();
  h0 = l->header;
  while (! h0.writer_has_lock)
    {
      /* Want to write but there are still readers with lock? */
      if (type == CLIB_SMP_LOCK_TYPE_WRITER && h0.n_readers_with_lock != 0)
	break;

      if (type == CLIB_SMP_LOCK_TYPE_SPIN)
	ASSERT_AND_PANIC (h0.waiting_fifo.n_elts == 0);

      /* Read/write can't proceed when waiting fifo is non-empty. */
      else if (h0.waiting_fifo.n_elts != 0)
	break;

      h1 = h0;
      h1.request_cpu = my_cpu;
      h1.writer_has_lock = ! is_reader;
      h1.n_readers_with_lock += is_reader;

      /* Try to set head and tail to zero and thereby get the lock. */
      h2 = clib_smp_lock_set_header (l, h1, h0);

      /* Compare and swap succeeded?  If so, we got the lock. */
      if (clib_smp_lock_header_is_equal (h2, h0))
	return;

      /* Header for slow path. */
      h0 = h2;
    }

  clib_smp_lock_slow_path (l, my_cpu, h0, type);
}

always_inline void
clib_smp_unlock_inline (clib_smp_lock_t * l, clib_smp_lock_type_t type)
{
  clib_smp_lock_header_t h0, h1;
  uword is_reader = type == CLIB_SMP_LOCK_TYPE_READER;
  uword my_cpu;
  
  /* Null means no locking is necessary. */
  if (! l)
    return;

  my_cpu = os_get_cpu_number ();
  h0 = l->header;

  /* Should be locked. */
  if (is_reader)
    {
      ASSERT_AND_PANIC (h0.n_readers_with_lock != 0);
      ASSERT_AND_PANIC (h0.writer_has_lock == 0);
    }
  else
    {
      ASSERT_AND_PANIC (h0.n_readers_with_lock == 0);
      ASSERT_AND_PANIC (h0.writer_has_lock);
    }

  /* Locked but empty waiting fifo? */
  while (h0.waiting_fifo.n_elts == 0)
    {
      /* Try to mark it unlocked. */
      h1 = h0;
      if (is_reader)
	h1.n_readers_with_lock -= 1;
      else
	h1.writer_has_lock = 0;
      h1.request_cpu = my_cpu;
      h1 = clib_smp_lock_set_header (l, h1, h0);
      if (clib_smp_lock_header_is_equal (h1, h0))
	return;
      h0 = h1;
    }

  /* Other cpus are waiting. */
  clib_smp_unlock_slow_path (l, my_cpu, h0, type);
}

always_inline void
clib_smp_lock (clib_smp_lock_t * l)
{ clib_smp_lock_inline (l, CLIB_SMP_LOCK_TYPE_SPIN); }

always_inline void
clib_smp_lock_for_writer (clib_smp_lock_t * l)
{ clib_smp_lock_inline (l, CLIB_SMP_LOCK_TYPE_WRITER); }

always_inline void
clib_smp_lock_for_reader (clib_smp_lock_t * l)
{ clib_smp_lock_inline (l, CLIB_SMP_LOCK_TYPE_READER); }

always_inline void
clib_smp_unlock (clib_smp_lock_t * l)
{ clib_smp_unlock_inline (l, CLIB_SMP_LOCK_TYPE_SPIN); }

always_inline void
clib_smp_unlock_for_writer (clib_smp_lock_t * l)
{ clib_smp_unlock_inline (l, CLIB_SMP_LOCK_TYPE_WRITER); }

always_inline void
clib_smp_unlock_for_reader (clib_smp_lock_t * l)
{ clib_smp_unlock_inline (l, CLIB_SMP_LOCK_TYPE_READER); }

#define clib_exec_on_global_heap(body)					\
do {									\
  void * __clib_exec_on_global_heap_saved_heap;				\
									\
  /* Switch to global (thread-safe) heap. */				\
  __clib_exec_on_global_heap_saved_heap = clib_mem_set_heap (clib_smp_main.global_heap); \
									\
  /* Execute body. */							\
  body;									\
									\
  /* Switch back to previous heap. */					\
  clib_mem_set_heap (__clib_exec_on_global_heap_saved_heap);		\
} while (0)

uword os_smp_bootstrap (uword n_cpus,
			void * bootstrap_function,
			uword bootstrap_function_arg);

void clib_smp_init (void);

#endif /* included_clib_smp_h */
