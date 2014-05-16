#include <uclib/uclib.h>
#include <uclib/uclib.c>

int main (int argc, char * argv[])
{
  typedef struct {
    int a, b;
  } foo_t;
  foo_t * pool = 0, * p;

  {
    clib_time_t ct;
    clib_time_init (&ct);
    clib_warning ("cpu freq %.6e", ct.clocks_per_second);
  }

  pool_get (pool, p);

  p->a = p->b = 1;

  pool_get (pool, p);
  p->a = p->b = 2;

  p = pool_elt_at_index (pool, 0);

  pool_put_index (pool, 0);

  clib_warning ("ok pool %p", pool);

  return 0;
}
