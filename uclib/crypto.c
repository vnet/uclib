/*
  Copyright (c) 2014 Eliot Dresselhaus

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

#include <uclib/uclib.h>

static void
sha1_update_64 (u32 * state, u8 * data)
{
  u32 temp, W[16], A, B, C, D, E;

#define _(i) W[i] = clib_big_to_host_mem_u32 ((u32 *) (data + sizeof(u32)*(i)))
  _ (0x0); _ (0x1); _ (0x2); _ (0x3); _ (0x4); _ (0x5); _ (0x6); _ (0x7);
  _ (0x8); _ (0x9); _ (0xa); _ (0xb); _ (0xc); _ (0xd); _ (0xe); _ (0xf);
#undef _ 

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define R(t)                                            \
(                                                       \
    temp = W[( t -  3 ) & 0x0F] ^ W[( t - 8 ) & 0x0F] ^ \
           W[( t - 14 ) & 0x0F] ^ W[  t       & 0x0F],  \
    ( W[t & 0x0F] = S(temp,1) )                         \
)

#define P(a,b,c,d,e,x)                                  \
{                                                       \
    e += S(a,5) + F(b,c,d) + K + x; b = S(b,30);        \
}

    A = state[0];
    B = state[1];
    C = state[2];
    D = state[3];
    E = state[4];

#define F(x,y,z) (z ^ (x & (y ^ z)))
#define K 0x5A827999

    P( A, B, C, D, E, W[0]  );
    P( E, A, B, C, D, W[1]  );
    P( D, E, A, B, C, W[2]  );
    P( C, D, E, A, B, W[3]  );
    P( B, C, D, E, A, W[4]  );
    P( A, B, C, D, E, W[5]  );
    P( E, A, B, C, D, W[6]  );
    P( D, E, A, B, C, W[7]  );
    P( C, D, E, A, B, W[8]  );
    P( B, C, D, E, A, W[9]  );
    P( A, B, C, D, E, W[10] );
    P( E, A, B, C, D, W[11] );
    P( D, E, A, B, C, W[12] );
    P( C, D, E, A, B, W[13] );
    P( B, C, D, E, A, W[14] );
    P( A, B, C, D, E, W[15] );
    P( E, A, B, C, D, R(16) );
    P( D, E, A, B, C, R(17) );
    P( C, D, E, A, B, R(18) );
    P( B, C, D, E, A, R(19) );

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0x6ED9EBA1

    P( A, B, C, D, E, R(20) );
    P( E, A, B, C, D, R(21) );
    P( D, E, A, B, C, R(22) );
    P( C, D, E, A, B, R(23) );
    P( B, C, D, E, A, R(24) );
    P( A, B, C, D, E, R(25) );
    P( E, A, B, C, D, R(26) );
    P( D, E, A, B, C, R(27) );
    P( C, D, E, A, B, R(28) );
    P( B, C, D, E, A, R(29) );
    P( A, B, C, D, E, R(30) );
    P( E, A, B, C, D, R(31) );
    P( D, E, A, B, C, R(32) );
    P( C, D, E, A, B, R(33) );
    P( B, C, D, E, A, R(34) );
    P( A, B, C, D, E, R(35) );
    P( E, A, B, C, D, R(36) );
    P( D, E, A, B, C, R(37) );
    P( C, D, E, A, B, R(38) );
    P( B, C, D, E, A, R(39) );

#undef K
#undef F

#define F(x,y,z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC

    P( A, B, C, D, E, R(40) );
    P( E, A, B, C, D, R(41) );
    P( D, E, A, B, C, R(42) );
    P( C, D, E, A, B, R(43) );
    P( B, C, D, E, A, R(44) );
    P( A, B, C, D, E, R(45) );
    P( E, A, B, C, D, R(46) );
    P( D, E, A, B, C, R(47) );
    P( C, D, E, A, B, R(48) );
    P( B, C, D, E, A, R(49) );
    P( A, B, C, D, E, R(50) );
    P( E, A, B, C, D, R(51) );
    P( D, E, A, B, C, R(52) );
    P( C, D, E, A, B, R(53) );
    P( B, C, D, E, A, R(54) );
    P( A, B, C, D, E, R(55) );
    P( E, A, B, C, D, R(56) );
    P( D, E, A, B, C, R(57) );
    P( C, D, E, A, B, R(58) );
    P( B, C, D, E, A, R(59) );

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0xCA62C1D6

    P( A, B, C, D, E, R(60) );
    P( E, A, B, C, D, R(61) );
    P( D, E, A, B, C, R(62) );
    P( C, D, E, A, B, R(63) );
    P( B, C, D, E, A, R(64) );
    P( A, B, C, D, E, R(65) );
    P( E, A, B, C, D, R(66) );
    P( D, E, A, B, C, R(67) );
    P( C, D, E, A, B, R(68) );
    P( B, C, D, E, A, R(69) );
    P( A, B, C, D, E, R(70) );
    P( E, A, B, C, D, R(71) );
    P( D, E, A, B, C, R(72) );
    P( C, D, E, A, B, R(73) );
    P( B, C, D, E, A, R(74) );
    P( A, B, C, D, E, R(75) );
    P( E, A, B, C, D, R(76) );
    P( D, E, A, B, C, R(77) );
    P( C, D, E, A, B, R(78) );
    P( B, C, D, E, A, R(79) );

#undef K
#undef F

    state[0] += A;
    state[1] += B;
    state[2] += C;
    state[3] += D;
    state[4] += E;
}

void sha1_update (sha1_state_t * s, u8 * data, uword n_bytes)
{
  uword n_left = n_bytes;
  uword n_bytes_in_partial_block_buffer;
  u8 * d = data;

  n_bytes_in_partial_block_buffer = s->n_bytes_processed % sizeof (s->partial_block_buffer);
  if (n_bytes_in_partial_block_buffer > 0)
    {
      uword n_bytes_to_partial_block_buffer = clib_min (sizeof (s->partial_block_buffer) - n_bytes_in_partial_block_buffer,
                                                        n_bytes);
      memcpy (s->partial_block_buffer + n_bytes_in_partial_block_buffer,
              d,
              n_bytes_to_partial_block_buffer);
      n_left -= n_bytes_to_partial_block_buffer;
      d += n_bytes_to_partial_block_buffer;
      n_bytes_in_partial_block_buffer += n_bytes_to_partial_block_buffer;
      if (n_bytes_in_partial_block_buffer == sizeof (s->partial_block_buffer))
        sha1_update_64 (s->state, s->partial_block_buffer);
    }

  while (n_left >= 64)
    {
      sha1_update_64 (s->state, d);
      n_left -= 64;
      d += 64;
    }

  memcpy (s->partial_block_buffer, d, n_left);
  s->n_bytes_processed += n_bytes;
}

void sha1_init (sha1_state_t * s)
{
  s->n_bytes_processed = 0;
  s->state[0] = 0x67452301;
  s->state[1] = 0xEFCDAB89;
  s->state[2] = 0x98BADCFE;
  s->state[3] = 0x10325476;
  s->state[4] = 0xC3D2E1F0;
}

void sha1_finalize (sha1_state_t * s, u8 * result)
{
  u8 last_block[128];
  u32 n_last_block;
  u32 n_left = s->n_bytes_processed % sizeof (s->partial_block_buffer);

  n_last_block = 128 - 64*(n_left < 56);
    
  memset (last_block, 0, sizeof (last_block));

  /* Copy in remaining message. */
  memcpy (last_block, s->partial_block_buffer, n_left);

  /* Terminate with 1 bit to mark end of message. */
  last_block[n_left] = 0x80;

  /* Add number of bits in message at end. */
  {
    u64 n64 = s->n_bytes_processed;
    *(u64 *) (last_block + n_last_block - 8) = clib_host_to_big_u64 (n64 << 3);
  }

  sha1_update_64 (s->state, last_block + 0);
  if (n_last_block > 64)
    sha1_update_64 (s->state, last_block + 64);

  /* Remove traces of message stored in stack. */
  crypto_zero_memory (last_block, n_left);
  crypto_zero_memory (s->partial_block_buffer, n_left);

  /* Return result. */
#define _(i) ((u32 *) result)[i] = clib_host_to_big_u32 (s->state[i]);
  _ (0); _ (1); _ (2); _ (3); _ (4);
#undef _
}

void sha1 (u8 * result, u8 * data, uword n_bytes)
{
  sha1_state_t s;
  sha1_init (&s);
  sha1_update (&s, data, n_bytes);
  sha1_finalize (&s, result);
}
