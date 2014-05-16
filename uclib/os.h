#ifndef included_uclib_os_h
#define included_uclib_os_h

#include <stdlib.h>

void os_panic ();
void os_exit (int code);

/* External function to print a line. */
void os_puts (u8 * string, uword length, uword is_error);

always_inline uword
os_get_cpu_number (void)
{ return 0; }

#endif /* included_uclib_os_h */
