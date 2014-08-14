#include <uclib/uclib.h>
#include <uclib/uclib.c>

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

static int verbose = 0;
#define if_verbose(format,args...) \
  if (verbose) { clib_warning(format, ## args); }

int test_mheap_main (unformat_input_t * input)
{
  int i, j, k, n_iterations;
  void * h, * h_mem;
  uword * objects = 0;
  u32 objects_used, really_verbose, n_objects, max_object_size;
  u32 check_mask, seed, trace, use_vm;
  u32 print_every = 0;
  u32 * data;
  mheap_t * mh;

  /* Validation flags. */
  check_mask = 0;
#define CHECK_VALIDITY 1
#define CHECK_DATA     2
#define CHECK_ALIGN    4

  n_iterations = 10;
  seed = 0;
  max_object_size = 100;
  n_objects = 1000;
  trace = 0;
  really_verbose = 0;
  use_vm = 0;

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT)
    {
      if (0 == unformat (input, "iter %d", &n_iterations)
	  && 0 == unformat (input, "count %d", &n_objects)
	  && 0 == unformat (input, "size %d", &max_object_size)
	  && 0 == unformat (input, "seed %d", &seed)
	  && 0 == unformat (input, "print %d", &print_every)
	  && 0 == unformat (input, "validdata %|",
			    &check_mask, CHECK_DATA | CHECK_VALIDITY)
	  && 0 == unformat (input, "valid %|",
			    &check_mask, CHECK_VALIDITY)
	  && 0 == unformat (input, "verbose %=", &really_verbose, 1)
	  && 0 == unformat (input, "trace %=", &trace, 1)
	  && 0 == unformat (input, "vm %=", &use_vm, 1)
	  && 0 == unformat (input, "align %|", &check_mask, CHECK_ALIGN))
	{
	  clib_warning ("unknown input `%U'", format_unformat_error, input);
	  return 1;
	}
    }

  /* Zero seed means use default. */
  if (! seed)
    seed = random_default_seed ();

  if_verbose   ("testing %d iterations, %d %saligned objects, max. size %d, seed %d",
		n_iterations,
		n_objects,
		(check_mask & CHECK_ALIGN) ? "randomly " : "un",
		max_object_size,
		seed);

  vec_resize (objects, n_objects);
  memset (objects, ~0, vec_bytes (objects));
  objects_used = 0;

  /* Allocate initial heap. */
  {
    uword size = max_pow2 (2 * n_objects * max_object_size * sizeof (data[0]));

    if (use_vm)
      {
	h_mem = 0;
	h = mheap_alloc (h_mem, size);
      }
    else
      {
	h_mem = clib_mem_alloc (size);
	if (! h_mem)
	  return 0;
	h = mheap_alloc (h_mem, size);
      }
  }

  if (trace)
    mheap_trace (h, trace);

  mh = mheap_header (h);

  if (use_vm)
    mh->flags &= ~MHEAP_FLAG_DISABLE_VM;
  else
    mh->flags |= MHEAP_FLAG_DISABLE_VM;

  if (check_mask & CHECK_VALIDITY)
    mh->flags |= MHEAP_FLAG_VALIDATE;

  for (i = 0; i < n_iterations; i++)
    {
      while (1)
	{
	  j = random_u32 (&seed) % vec_len (objects);
	  if (objects[j] != ~0 || i + objects_used < n_iterations)
	    break;
	}

      if (objects[j] != ~0)
	{
	  mheap_put (h, objects[j]);
	  objects_used--;
	  objects[j] = ~0;
	}
      else
	{
	  uword size, align, align_offset;

	  size = (random_u32 (&seed) % max_object_size) * sizeof (data[0]);
	  align = align_offset = 0;
	  if (check_mask & CHECK_ALIGN)
	    {
	      align = 1 << (random_u32 (&seed) % 10);
	      align_offset = round_pow2 (random_u32 (&seed) & (align - 1),
					 sizeof (u32));
	    }
	  
	  h = mheap_get_aligned (h, size, align, align_offset, &objects[j]);

	  if (align > 0)
	    ASSERT (0 == ((objects[j] + align_offset) & (align - 1)));

	  ASSERT (objects[j] != ~0);
	  objects_used++;

	  /* Set newly allocated object with test data. */
	  if (check_mask & CHECK_DATA)
	    {
	      uword len;

	      data = (void *) h +  objects[j];
	      len = mheap_len (h, data);

	      ASSERT (size <= mheap_data_bytes (h, objects[j]));

	      data[0] = len;
	      for (k = 1; k < len; k++)
		data[k] = objects[j] + k;
	    }
	}

      /* Verify that all used objects have correct test data. */
      if (check_mask & 2)
	{
	  for (j = 0; j < vec_len (objects); j++)
	    if (objects[j] != ~0)
	      {
		u32 * data = h + objects[j];
		uword len = data[0];
		for (k = 1; k < len; k++)
		  ASSERT (data[k] == objects[j] + k);
	      }
	}
      if (print_every != 0 && i > 0 && (i % print_every) == 0)
	fformat (stderr, "iteration %d: %U\n", i, format_mheap, h, really_verbose);
    }

  if (verbose)
    fformat (stderr, "%U\n", format_mheap, h, really_verbose);
  mheap_free (h);
  if (h_mem)
    clib_mem_free (h_mem);
  vec_free (objects);

  return 0;
}

static void foo ()
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
}

static void test_sha1 ()
{
  u8 sum[20];

  static u8 sha1_test_sums[3][20] =
    {
      { 0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A, 0xBA, 0x3E,
        0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C, 0x9C, 0xD0, 0xD8, 0x9D },
      { 0x84, 0x98, 0x3E, 0x44, 0x1C, 0x3B, 0xD2, 0x6E, 0xBA, 0xAE,
        0x4A, 0xA1, 0xF9, 0x51, 0x29, 0xE5, 0xE5, 0x46, 0x70, 0xF1 },
      { 0x34, 0xAA, 0x97, 0x3C, 0xD4, 0xC4, 0xDA, 0xA4, 0xF6, 0x1E,
        0xEB, 0x2B, 0xDB, 0xAD, 0x27, 0x31, 0x65, 0x34, 0x01, 0x6F }
    };

  sha1 (sum, (u8 *) "abc", 3);
  ASSERT (! memcmp (sum, sha1_test_sums[0], sizeof (sum)));

  sha1 (sum, (u8 *) "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56);
  ASSERT (! memcmp (sum, sha1_test_sums[1], sizeof (sum)));

  {
    u8 * s = 0;
    int i;
    sha1_state_t sha1;

    for (i = 0; i < 1000; i++)
      vec_add1 (s, 'a');

    sha1_init (&sha1);
    for (i = 0; i < 1000; i++)
      sha1_update (&sha1, s, vec_len (s));

    sha1_finalize (&sha1, sum);
    ASSERT (! memcmp (sum, sha1_test_sums[2], sizeof (sum)));
    vec_free (s);
  }
}

static void test_sha256 ()
{
  u8 sum256[32], sum224[28];

  static u8 sha224_test_sums[3][32] =
    {
      { 0x23, 0x09, 0x7D, 0x22, 0x34, 0x05, 0xD8, 0x22,
        0x86, 0x42, 0xA4, 0x77, 0xBD, 0xA2, 0x55, 0xB3,
        0x2A, 0xAD, 0xBC, 0xE4, 0xBD, 0xA0, 0xB3, 0xF7,
        0xE3, 0x6C, 0x9D, 0xA7 },
      { 0x75, 0x38, 0x8B, 0x16, 0x51, 0x27, 0x76, 0xCC,
        0x5D, 0xBA, 0x5D, 0xA1, 0xFD, 0x89, 0x01, 0x50,
        0xB0, 0xC6, 0x45, 0x5C, 0xB4, 0xF5, 0x8B, 0x19,
        0x52, 0x52, 0x25, 0x25 },
      { 0x20, 0x79, 0x46, 0x55, 0x98, 0x0C, 0x91, 0xD8,
        0xBB, 0xB4, 0xC1, 0xEA, 0x97, 0x61, 0x8A, 0x4B,
        0xF0, 0x3F, 0x42, 0x58, 0x19, 0x48, 0xB2, 0xEE,
        0x4E, 0xE7, 0xAD, 0x67 },
    };

  static u8 sha256_test_sums[3][32] =
    {
      { 0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
        0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
        0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
        0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD },
      { 0x24, 0x8D, 0x6A, 0x61, 0xD2, 0x06, 0x38, 0xB8,
        0xE5, 0xC0, 0x26, 0x93, 0x0C, 0x3E, 0x60, 0x39,
        0xA3, 0x3C, 0xE4, 0x59, 0x64, 0xFF, 0x21, 0x67,
        0xF6, 0xEC, 0xED, 0xD4, 0x19, 0xDB, 0x06, 0xC1 },
      { 0xCD, 0xC7, 0x6E, 0x5C, 0x99, 0x14, 0xFB, 0x92,
        0x81, 0xA1, 0xC7, 0xE2, 0x84, 0xD7, 0x3E, 0x67,
        0xF1, 0x80, 0x9A, 0x48, 0xA4, 0x97, 0x20, 0x0E,
        0x04, 0x6D, 0x39, 0xCC, 0xC7, 0x11, 0x2C, 0xD0 }
    };

  sha256 (sum256, (u8 *) "abc", 3);
  ASSERT (! memcmp (sum256, sha256_test_sums[0], sizeof (sum256)));

  sha224 (sum224, (u8 *) "abc", 3);
  ASSERT (! memcmp (sum224, sha224_test_sums[0], sizeof (sum224)));

  sha256 (sum256, (u8 *) "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56);
  ASSERT (! memcmp (sum256, sha256_test_sums[1], sizeof (sum256)));

  sha224 (sum224, (u8 *) "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56);
  ASSERT (! memcmp (sum224, sha224_test_sums[1], sizeof (sum224)));

  {
    u8 * s = 0;
    int i;

    for (i = 0; i < 1000; i++)
      vec_add1 (s, 'a');

    {
      sha256_state_t st;
      sha256_init (&st);
      for (i = 0; i < 1000; i++)
        sha256_update (&st, s, vec_len (s));
      sha256_finalize (&st, sum256);
      ASSERT (! memcmp (sum256, sha256_test_sums[2], sizeof (sum256)));
    }

    {
      sha224_state_t st;
      sha224_init (&st);
      for (i = 0; i < 1000; i++)
        sha224_update (&st, s, vec_len (s));
      sha224_finalize (&st, sum224);
      ASSERT (! memcmp (sum224, sha224_test_sums[2], sizeof (sum224)));
    }

    vec_free (s);
  }
}

static void test_sha512 ()
{
  u8 sum512[512 / BITS (u8)], sum384[384 / BITS (u8)];

  static u8 sha384_test_sums[3][64] =
    {
      { 0xCB, 0x00, 0x75, 0x3F, 0x45, 0xA3, 0x5E, 0x8B,
        0xB5, 0xA0, 0x3D, 0x69, 0x9A, 0xC6, 0x50, 0x07,
        0x27, 0x2C, 0x32, 0xAB, 0x0E, 0xDE, 0xD1, 0x63,
        0x1A, 0x8B, 0x60, 0x5A, 0x43, 0xFF, 0x5B, 0xED,
        0x80, 0x86, 0x07, 0x2B, 0xA1, 0xE7, 0xCC, 0x23,
        0x58, 0xBA, 0xEC, 0xA1, 0x34, 0xC8, 0x25, 0xA7 },
      { 0x09, 0x33, 0x0C, 0x33, 0xF7, 0x11, 0x47, 0xE8,
        0x3D, 0x19, 0x2F, 0xC7, 0x82, 0xCD, 0x1B, 0x47,
        0x53, 0x11, 0x1B, 0x17, 0x3B, 0x3B, 0x05, 0xD2,
        0x2F, 0xA0, 0x80, 0x86, 0xE3, 0xB0, 0xF7, 0x12,
        0xFC, 0xC7, 0xC7, 0x1A, 0x55, 0x7E, 0x2D, 0xB9,
        0x66, 0xC3, 0xE9, 0xFA, 0x91, 0x74, 0x60, 0x39 },
      { 0x9D, 0x0E, 0x18, 0x09, 0x71, 0x64, 0x74, 0xCB,
        0x08, 0x6E, 0x83, 0x4E, 0x31, 0x0A, 0x4A, 0x1C,
        0xED, 0x14, 0x9E, 0x9C, 0x00, 0xF2, 0x48, 0x52,
        0x79, 0x72, 0xCE, 0xC5, 0x70, 0x4C, 0x2A, 0x5B,
        0x07, 0xB8, 0xB3, 0xDC, 0x38, 0xEC, 0xC4, 0xEB,
        0xAE, 0x97, 0xDD, 0xD8, 0x7F, 0x3D, 0x89, 0x85 },
    };

  static u8 sha512_test_sums[3][64] =
    {
      { 0xDD, 0xAF, 0x35, 0xA1, 0x93, 0x61, 0x7A, 0xBA,
        0xCC, 0x41, 0x73, 0x49, 0xAE, 0x20, 0x41, 0x31,
        0x12, 0xE6, 0xFA, 0x4E, 0x89, 0xA9, 0x7E, 0xA2,
        0x0A, 0x9E, 0xEE, 0xE6, 0x4B, 0x55, 0xD3, 0x9A,
        0x21, 0x92, 0x99, 0x2A, 0x27, 0x4F, 0xC1, 0xA8,
        0x36, 0xBA, 0x3C, 0x23, 0xA3, 0xFE, 0xEB, 0xBD,
        0x45, 0x4D, 0x44, 0x23, 0x64, 0x3C, 0xE8, 0x0E,
        0x2A, 0x9A, 0xC9, 0x4F, 0xA5, 0x4C, 0xA4, 0x9F },
      { 0x8E, 0x95, 0x9B, 0x75, 0xDA, 0xE3, 0x13, 0xDA,
        0x8C, 0xF4, 0xF7, 0x28, 0x14, 0xFC, 0x14, 0x3F,
        0x8F, 0x77, 0x79, 0xC6, 0xEB, 0x9F, 0x7F, 0xA1,
        0x72, 0x99, 0xAE, 0xAD, 0xB6, 0x88, 0x90, 0x18,
        0x50, 0x1D, 0x28, 0x9E, 0x49, 0x00, 0xF7, 0xE4,
        0x33, 0x1B, 0x99, 0xDE, 0xC4, 0xB5, 0x43, 0x3A,
        0xC7, 0xD3, 0x29, 0xEE, 0xB6, 0xDD, 0x26, 0x54,
        0x5E, 0x96, 0xE5, 0x5B, 0x87, 0x4B, 0xE9, 0x09 },
      { 0xE7, 0x18, 0x48, 0x3D, 0x0C, 0xE7, 0x69, 0x64,
        0x4E, 0x2E, 0x42, 0xC7, 0xBC, 0x15, 0xB4, 0x63,
        0x8E, 0x1F, 0x98, 0xB1, 0x3B, 0x20, 0x44, 0x28,
        0x56, 0x32, 0xA8, 0x03, 0xAF, 0xA9, 0x73, 0xEB,
        0xDE, 0x0F, 0xF2, 0x44, 0x87, 0x7E, 0xA6, 0x0A,
        0x4C, 0xB0, 0x43, 0x2C, 0xE5, 0x77, 0xC3, 0x1B,
        0xEB, 0x00, 0x9C, 0x5C, 0x2C, 0x49, 0xAA, 0x2E,
        0x4E, 0xAD, 0xB2, 0x17, 0xAD, 0x8C, 0xC0, 0x9B }
    };

  sha512 (sum512, (u8 *) "abc", 3);
  ASSERT (! memcmp (sum512, sha512_test_sums[0], sizeof (sum512)));

  sha384 (sum384, (u8 *) "abc", 3);
  ASSERT (! memcmp (sum384, sha384_test_sums[0], sizeof (sum384)));

  sha512 (sum512, (u8 *)
          "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
          "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 56*2);
  ASSERT (! memcmp (sum512, sha512_test_sums[1], sizeof (sum512)));

  sha384 (sum384, (u8 *)
          "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
          "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 56*2);
  ASSERT (! memcmp (sum384, sha384_test_sums[1], sizeof (sum384)));

  {
    u8 * s = 0;
    int i;

    for (i = 0; i < 1000; i++)
      vec_add1 (s, 'a');

    {
      sha512_state_t st;
      sha512_init (&st);
      for (i = 0; i < 1000; i++)
        sha512_update (&st, s, vec_len (s));
      sha512_finalize (&st, sum512);
      ASSERT (! memcmp (sum512, sha512_test_sums[2], sizeof (sum512)));
    }

    {
      sha384_state_t st;
      sha384_init (&st);
      for (i = 0; i < 1000; i++)
        sha384_update (&st, s, vec_len (s));
      sha384_finalize (&st, sum384);
      ASSERT (! memcmp (sum384, sha384_test_sums[2], sizeof (sum384)));
    }

    vec_free (s);
  }
}

int main (int argc, char * argv[])
{
  unformat_input_t i;
  int ret;

  if (1)
    {
      test_sha1 ();
      test_sha256 ();
      test_sha512 ();
    }

  if (0) foo ();

  verbose = (argc > 1);
  unformat_init_command_line (&i, argv);
  ret = test_mheap_main (&i);
  unformat_free (&i);

  return ret;
}

