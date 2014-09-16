#ifndef included_uclib_os_h
#define included_uclib_os_h

#include <stdlib.h>

void os_panic ();
void os_exit (int code);

/* External function to print a line. */
void os_puts (u8 * string, uword length, uword is_error);

always_inline uword
os_get_cpu_number (void)
{
  u8 variable_on_stack;
  uword offset = pointer_to_uword (&variable_on_stack) - pointer_to_uword (clib_smp_main.vm_base);
  uword my_cpu = (offset >> clib_smp_main.log2_n_per_cpu_vm_bytes);
  uword n_cpus = clib_smp_main.n_cpus;
  return my_cpu < n_cpus ? my_cpu : n_cpus;
}

#endif /* included_uclib_os_h */
