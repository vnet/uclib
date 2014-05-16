#ifndef included_uclib_os_h
#define included_uclib_os_h

#include <stdlib.h>

always_inline void os_panic ()
{ abort (); }

always_inline void os_exit (uword code)
{ exit (code); }

always_inline uword
os_get_cpu_number (void)
{ return 0; }

/* External function to print a line. */
void os_puts (u8 * string, uword length, uword is_error);

#endif /* included_uclib_os_h */
