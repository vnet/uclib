#include <uclib/uclib.h>

typedef struct {
  f64 time;
} test_fheap_elt_t;

typedef struct {
  fheap_t fheap;
  
  test_fheap_elt_t * elt_pool;

  u32 verbose;

  u32 seed;

  u32 n_iter;

  u32 max_elts;
} test_fheap_main_t;

int test_fheap_main (unformat_input_t * input)
{
  test_fheap_main_t tm;
  clib_error_t * error = 0;

  memset (&tm, 0, sizeof (tm));
  tm.verbose = 0;
  tm.seed = 1;
  tm.n_iter = 10;
  tm.max_elts = 10;

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat (input, "seed %d", &tm.seed))
        ;
      else if (unformat (input, "iter %d", &tm.n_iter))
        ;
      else if (unformat (input, "max-elts %d", &tm.max_elts))
        ;
      else if (unformat (input, "verbose"))
        tm.verbose = 1;
      else
        {
          error = clib_error_return (0, "unknown input `%U'", format_unformat_error, input);
          goto done;
        }
    }

  fheap_init (&tm.fheap);
  tm.fheap.enable_validate = 0;

  if (! tm.seed)
    tm.seed = getpid ();

  {
    int i, j, k;
    u64 stats[2] = {0};
    
    for (i = 0; i < tm.n_iter; i++)
      {
        test_fheap_elt_t * te;

        while (pool_elts (tm.elt_pool) < tm.max_elts)
          {
            pool_get (tm.elt_pool, te);
            te->time = 100 * random_f64 (&tm.seed);
            fheap_add (&tm.fheap, te - tm.elt_pool, te->time);
            stats[0] += 1;
          }

        k = random_u32 (&tm.seed) % (tm.max_elts / 2);
        for (j = 0; j < k; j++)
          {
            f64 min_time;
            u32 min_i = fheap_del_min (&tm.fheap, &min_time);
            test_fheap_elt_t * te_min = pool_elt_at_index (tm.elt_pool, min_i);

            ASSERT (te_min->time == min_time);

            pool_foreach (te, tm.elt_pool, ({
              ASSERT (te->time >= te_min->time);
            }));

            pool_put (tm.elt_pool, te_min);
            stats[1] += 1;
          }
      }

    clib_warning ("%Ld adds %Ld del_mins", stats[0], stats[1]);
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
  ret = test_fheap_main (&i);
  unformat_free (&i);

  return ret;
}

