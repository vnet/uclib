#include <unistd.h>

void os_puts (u8 * string, uword length, uword is_error)
{
  write (1, string, length);
}
