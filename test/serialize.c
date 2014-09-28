#include <uclib/uclib.h>
#include <uclib/uclib.c>

typedef struct { u32 a, b; } foo_t;

always_inline int foo_cmp (foo_t * f0, foo_t * f1)
{
  int cmp_a = f0->a < f1->a ? -1 : (f0->a > f1->a ? +1 : 0);
  int cmp_b = f0->b < f1->b ? -1 : (f0->b > f1->b ? +1 : 0);
  return cmp_a ? cmp_a : cmp_b;
}

typedef struct {
  foo_t * foo_pool;
} foo_main_t;

static int foo_main_cmp (foo_main_t * fm0, foo_main_t * fm1)
{
  uword i;

  if (pool_elts (fm0->foo_pool) != pool_elts (fm1->foo_pool))
    return 1;

  for (i = 0; i < pool_len (fm0->foo_pool); i++)
    {
      if (pool_is_free_index (fm0->foo_pool, i)
          && ! pool_is_free_index (fm1->foo_pool, i))
        return 1;
      if (foo_cmp (fm0->foo_pool + i, fm1->foo_pool + i))
        return 1;
    }
  return 0;
}

static void serialize_multiple_foos (serialize_main_t * sm, va_list * va)
{
  foo_t * f = va_arg (*va, foo_t *);
  u32 n_foos = va_arg (*va, u32);
  u32 i;
  for (i = 0; i < n_foos; i++)
    {
      serialize_likely_small_unsigned_integer (sm, f[i].a);
      serialize_likely_small_unsigned_integer (sm, f[i].b);
    }
}

void unserialize_multiple_foos (serialize_main_t * sm, va_list * va)
{
  foo_t * f = va_arg (*va, foo_t *);
  u32 n_foos = va_arg (*va, u32);
  u32 i;
  for (i = 0; i < n_foos; i++)
    {
      f[i].a = unserialize_likely_small_unsigned_integer (sm);
      f[i].b = unserialize_likely_small_unsigned_integer (sm);
    }
}

static char * foo_main_magic = "foo_main_v1";

void serialize_foo_main (serialize_main_t * sm, va_list * va)
{
  foo_main_t * fm = va_arg (*va, foo_main_t *);
  serialize_magic (sm, foo_main_magic, strlen (foo_main_magic));
  pool_serialize (sm, fm->foo_pool, serialize_multiple_foos);
}

void unserialize_foo_main (serialize_main_t * sm, va_list * va)
{
  foo_main_t * fm = va_arg (*va, foo_main_t *);
  unserialize_check_magic (sm, foo_main_magic, strlen (foo_main_magic), "foo_main");
  pool_unserialize (sm, &fm->foo_pool, unserialize_multiple_foos);
}

static void serialize_add_foo (serialize_main_t * sm, va_list * va)
{
  CLIB_UNUSED (foo_main_t * fm) = va_arg (*va, foo_main_t *);
  foo_t * f = va_arg (*va, foo_t *);
  serialize (sm, serialize_multiple_foos, f, 1);
}

static void unserialize_add_foo (serialize_main_t * sm, va_list * va)
{
  foo_main_t * fm = va_arg (*va, foo_main_t *);
  foo_t * f;
  pool_get (fm->foo_pool, f);
  unserialize (sm, unserialize_multiple_foos, f, 1);
}

static serialize_diff_type_t add_foo = {
  .name = "add_foo",
  .serialize = serialize_add_foo,
  .unserialize = unserialize_add_foo,
};
CLIB_INIT_ADD (serialize_diff_type_t, add_foo);

static void serialize_change_foo (serialize_main_t * sm, va_list * va)
{
  foo_main_t * fm = va_arg (*va, foo_main_t *);
  foo_t * f = va_arg (*va, foo_t *);
  ASSERT (! pool_is_free (fm->foo_pool, f));
  serialize_likely_small_unsigned_integer (sm, f - fm->foo_pool);
  serialize (sm, serialize_multiple_foos, f, 1);
}

static void unserialize_change_foo (serialize_main_t * sm, va_list * va)
{
  foo_main_t * fm = va_arg (*va, foo_main_t *);
  u32 fi = unserialize_likely_small_unsigned_integer (sm);
  foo_t * f = pool_elt_at_index (fm->foo_pool, fi);
  unserialize (sm, unserialize_multiple_foos, f, 1);
}

static serialize_diff_type_t change_foo = {
  .name = "change_foo",
  .serialize = serialize_change_foo,
  .unserialize = unserialize_change_foo,
};
CLIB_INIT_ADD (serialize_diff_type_t, change_foo);

typedef struct {
  serialize_main_t serialize_main;
  foo_main_t foo_main[2];
} test_udb_main_t;

int test_udb_main (unformat_input_t * input)
{
  clib_error_t * error = 0;
  test_udb_main_t _tm, * tm = &_tm;
  serialize_main_t * sm = &tm->serialize_main;
  foo_main_t * fm = &tm->foo_main[0];
  foo_t * f;

  memset (tm, 0, sizeof (tm[0]));

  pool_get (fm->foo_pool, f);
  f->a = 1; f->b = 2;

  if ((error = serialize_open_unix_file_with_flags_and_mode (sm, "foos", O_SYNC, 0666)))
    goto done;
  serialize (sm, serialize_foo_main, fm);

  pool_get (fm->foo_pool, f);

  f->a = 3; f->b = 4;
  serialize_diff (sm, &add_foo, fm, f);

  f->a = 5; f->b = 6;
  serialize_diff (sm, &change_foo, fm, f);

  serialize_sync (sm);

  serialize_close (sm);

  {
    foo_main_t * fm1 = &tm->foo_main[1];

    if ((error = unserialize_open_unix_file (sm, "foos")))
      goto done;
    if ((error = unserialize (sm, unserialize_foo_main, fm1)))
      goto done;
    while (! unserialize_is_end_of_stream (sm))
      unserialize_diff (sm, fm1);

    unserialize_close (sm);

    if (foo_main_cmp (fm, fm1))
      os_panic ();
  }

 done:
  if (error)
    {
      clib_error_report (error);
      return 1;
    }

  return 0;
}

int main (int argc, char * argv[])
{
  unformat_input_t i;
  int ret;

  unformat_init_command_line (&i, argv);
  ret = test_udb_main (&i);
  unformat_free (&i);

  return ret;
}
