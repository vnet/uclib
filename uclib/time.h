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

#ifndef included_time_h
#define included_time_h

/* Return CPU time stamp as 64bit number. */
#if defined(__x86_64__) || defined(i386)
always_inline u64 clib_cpu_time_now (void)
{
  u32 a, d;
  asm volatile ("rdtsc"
		: "=a" (a), "=d" (d));
  return (u64) a + ((u64) d << (u64) 32);
}

#elif defined (__powerpc64__)

always_inline u64 clib_cpu_time_now (void)
{
  u64 t;
  asm volatile ("mftb %0" : "=r" (t));
  return t;
}

#elif defined (__SPU__)

always_inline u64 clib_cpu_time_now (void)
{
#ifdef _XLC
  return spu_rdch (0x8);
#else
  return 0 /* __builtin_si_rdch (0x8) FIXME */;
#endif
}

#elif defined (__powerpc__)

always_inline u64 clib_cpu_time_now (void)
{
  u32 hi1, hi2, lo;
  asm volatile (
    "1:\n"
    "mftbu %[hi1]\n"
    "mftb  %[lo]\n"
    "mftbu %[hi2]\n"
    "cmpw %[hi1],%[hi2]\n"
    "bne 1b\n"
    : [hi1] "=r" (hi1), [hi2] "=r" (hi2), [lo] "=r" (lo));
  return (u64) lo + ((u64) hi2 << (u64) 32);
}

#elif defined (__arm__)

always_inline u64 clib_cpu_time_now (void)
{
  u32 lo;
  asm volatile ("mrc p15, 0, %[lo], c15, c12, 1"
		: [lo] "=r" (lo));
  return (u64) lo;
}

#elif defined (__xtensa__)

/* Stub for now. */
always_inline u64 clib_cpu_time_now (void)
{ return 0; }

#elif defined (__TMS320C6X__)

always_inline u64 clib_cpu_time_now (void)
{
  u32 l, h;

  asm volatile (" dint\n"
		" mvc .s2 TSCL,%0\n"
		" mvc .s2 TSCH,%1\n"
		" rint\n"
		: "=b" (l), "=b" (h));

  return ((u64)h << 32) | l;
}

#else

#error "don't know how to read CPU time stamp"

#endif

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time

#include <mach/clock.h>
#include <mach/mach.h>

always_inline void
unix_time_now_timespec (struct timespec * ts)
{
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
}

#endif

#ifndef __MACH__

#include <sys/syscall.h>
always_inline void
unix_time_now_timespec (struct timespec * ts)
{
  /* clock_gettime without indirect syscall uses GLIBC wrappers which
     we don't want.  Just the bare metal, please. */
  syscall (SYS_clock_gettime, CLOCK_REALTIME, ts);
}
#endif

/* Use 64bit floating point to represent time offset from epoch. */
always_inline f64 unix_time_now (void)
{
  struct timespec ts;
  unix_time_now_timespec (&ts);
  return ts.tv_sec + 1e-9*ts.tv_nsec;
}

/* As above but integer number of nano-seconds. */
always_inline u64 unix_time_now_nsec (void)
{
  struct timespec ts;
  unix_time_now_timespec (&ts);
  return 1e9*ts.tv_sec + ts.tv_nsec;
}

always_inline f64 unix_usage_now (void)
{
  struct rusage u;
  getrusage (RUSAGE_SELF, &u);
  return u.ru_utime.tv_sec + 1e-6*u.ru_utime.tv_usec
    + u.ru_stime.tv_sec + 1e-6*u.ru_stime.tv_usec;
}

always_inline void unix_sleep (f64 dt)
{
  struct timespec t;
  t.tv_sec = dt;
  t.tv_nsec = 1e9 * dt;
  nanosleep (&t, 0);
}

typedef struct {
  /* Total run time in clock cycles
     since clib_time_init call. */
  u64 total_cpu_time;

  /* Last recorded time stamp. */
  u64 last_cpu_time;

  /* CPU clock frequency. */
  f64 clocks_per_second;

  /* 1 / cpu clock frequency: conversion factor
     from clock cycles into seconds. */
  f64 seconds_per_clock;

  /* Time stamp of call to clib_time_init call. */
  u64 init_cpu_time;
} clib_time_t;

always_inline f64
clib_time_now (clib_time_t * c)
{
  u64 n = clib_cpu_time_now ();
  u64 l = c->last_cpu_time;
  u64 t = c->total_cpu_time;
  t += n - l;
  c->total_cpu_time = t;
  c->last_cpu_time = n;
  return t * c->seconds_per_clock;
}

void clib_time_init (clib_time_t * c);

#endif /* included_time_h */
