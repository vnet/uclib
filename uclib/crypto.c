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
sha1_update_block (u32 * state, u8 * data)
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

#undef R
#undef P

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
        sha1_update_block (s->state, s->partial_block_buffer);
    }

  while (n_left >= 64)
    {
      sha1_update_block (s->state, d);
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

  sha1_update_block (s->state, last_block + 0);
  if (n_last_block > 64)
    sha1_update_block (s->state, last_block + 64);

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

static void
sha256_update_block (u32 * state, u8 * data)
{
  u32 temp1, temp2, W[64];
  u32 A, B, C, D, E, F, G, H;

#define _(i) W[i] = clib_big_to_host_mem_u32 ((u32 *) (data + sizeof(u32)*(i)))
  _ (0x0); _ (0x1); _ (0x2); _ (0x3); _ (0x4); _ (0x5); _ (0x6); _ (0x7);
  _ (0x8); _ (0x9); _ (0xa); _ (0xb); _ (0xc); _ (0xd); _ (0xe); _ (0xf);
#undef _ 

#define  SHR(x,n) ((x & 0xFFFFFFFF) >> n)
#define ROTR(x,n) (SHR(x,n) | (x << (32 - n)))

#define S0(x) (ROTR(x, 7) ^ ROTR(x,18) ^  SHR(x, 3))
#define S1(x) (ROTR(x,17) ^ ROTR(x,19) ^  SHR(x,10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x,13) ^ ROTR(x,22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x,11) ^ ROTR(x,25))

#define F0(x,y,z) ((x & y) | (z & (x | y)))
#define F1(x,y,z) (z ^ (x & (y ^ z)))

#define R(t)                                            \
  (                                                     \
   W[t] = S1(W[t -  2]) + W[t -  7] +                   \
   S0(W[t - 15]) + W[t - 16]                            \
                                                  )

#define P(a,b,c,d,e,f,g,h,x,K)                  \
  {                                             \
    temp1 = h + S3(e) + F1(e,f,g) + K + x;      \
    temp2 = S2(a) + F0(a,b,c);                  \
    d += temp1; h = temp1 + temp2;              \
  }

  A = state[0];
  B = state[1];
  C = state[2];
  D = state[3];
  E = state[4];
  F = state[5];
  G = state[6];
  H = state[7];

  P( A, B, C, D, E, F, G, H, W[ 0], 0x428A2F98 );
  P( H, A, B, C, D, E, F, G, W[ 1], 0x71374491 );
  P( G, H, A, B, C, D, E, F, W[ 2], 0xB5C0FBCF );
  P( F, G, H, A, B, C, D, E, W[ 3], 0xE9B5DBA5 );
  P( E, F, G, H, A, B, C, D, W[ 4], 0x3956C25B );
  P( D, E, F, G, H, A, B, C, W[ 5], 0x59F111F1 );
  P( C, D, E, F, G, H, A, B, W[ 6], 0x923F82A4 );
  P( B, C, D, E, F, G, H, A, W[ 7], 0xAB1C5ED5 );
  P( A, B, C, D, E, F, G, H, W[ 8], 0xD807AA98 );
  P( H, A, B, C, D, E, F, G, W[ 9], 0x12835B01 );
  P( G, H, A, B, C, D, E, F, W[10], 0x243185BE );
  P( F, G, H, A, B, C, D, E, W[11], 0x550C7DC3 );
  P( E, F, G, H, A, B, C, D, W[12], 0x72BE5D74 );
  P( D, E, F, G, H, A, B, C, W[13], 0x80DEB1FE );
  P( C, D, E, F, G, H, A, B, W[14], 0x9BDC06A7 );
  P( B, C, D, E, F, G, H, A, W[15], 0xC19BF174 );
  P( A, B, C, D, E, F, G, H, R(16), 0xE49B69C1 );
  P( H, A, B, C, D, E, F, G, R(17), 0xEFBE4786 );
  P( G, H, A, B, C, D, E, F, R(18), 0x0FC19DC6 );
  P( F, G, H, A, B, C, D, E, R(19), 0x240CA1CC );
  P( E, F, G, H, A, B, C, D, R(20), 0x2DE92C6F );
  P( D, E, F, G, H, A, B, C, R(21), 0x4A7484AA );
  P( C, D, E, F, G, H, A, B, R(22), 0x5CB0A9DC );
  P( B, C, D, E, F, G, H, A, R(23), 0x76F988DA );
  P( A, B, C, D, E, F, G, H, R(24), 0x983E5152 );
  P( H, A, B, C, D, E, F, G, R(25), 0xA831C66D );
  P( G, H, A, B, C, D, E, F, R(26), 0xB00327C8 );
  P( F, G, H, A, B, C, D, E, R(27), 0xBF597FC7 );
  P( E, F, G, H, A, B, C, D, R(28), 0xC6E00BF3 );
  P( D, E, F, G, H, A, B, C, R(29), 0xD5A79147 );
  P( C, D, E, F, G, H, A, B, R(30), 0x06CA6351 );
  P( B, C, D, E, F, G, H, A, R(31), 0x14292967 );
  P( A, B, C, D, E, F, G, H, R(32), 0x27B70A85 );
  P( H, A, B, C, D, E, F, G, R(33), 0x2E1B2138 );
  P( G, H, A, B, C, D, E, F, R(34), 0x4D2C6DFC );
  P( F, G, H, A, B, C, D, E, R(35), 0x53380D13 );
  P( E, F, G, H, A, B, C, D, R(36), 0x650A7354 );
  P( D, E, F, G, H, A, B, C, R(37), 0x766A0ABB );
  P( C, D, E, F, G, H, A, B, R(38), 0x81C2C92E );
  P( B, C, D, E, F, G, H, A, R(39), 0x92722C85 );
  P( A, B, C, D, E, F, G, H, R(40), 0xA2BFE8A1 );
  P( H, A, B, C, D, E, F, G, R(41), 0xA81A664B );
  P( G, H, A, B, C, D, E, F, R(42), 0xC24B8B70 );
  P( F, G, H, A, B, C, D, E, R(43), 0xC76C51A3 );
  P( E, F, G, H, A, B, C, D, R(44), 0xD192E819 );
  P( D, E, F, G, H, A, B, C, R(45), 0xD6990624 );
  P( C, D, E, F, G, H, A, B, R(46), 0xF40E3585 );
  P( B, C, D, E, F, G, H, A, R(47), 0x106AA070 );
  P( A, B, C, D, E, F, G, H, R(48), 0x19A4C116 );
  P( H, A, B, C, D, E, F, G, R(49), 0x1E376C08 );
  P( G, H, A, B, C, D, E, F, R(50), 0x2748774C );
  P( F, G, H, A, B, C, D, E, R(51), 0x34B0BCB5 );
  P( E, F, G, H, A, B, C, D, R(52), 0x391C0CB3 );
  P( D, E, F, G, H, A, B, C, R(53), 0x4ED8AA4A );
  P( C, D, E, F, G, H, A, B, R(54), 0x5B9CCA4F );
  P( B, C, D, E, F, G, H, A, R(55), 0x682E6FF3 );
  P( A, B, C, D, E, F, G, H, R(56), 0x748F82EE );
  P( H, A, B, C, D, E, F, G, R(57), 0x78A5636F );
  P( G, H, A, B, C, D, E, F, R(58), 0x84C87814 );
  P( F, G, H, A, B, C, D, E, R(59), 0x8CC70208 );
  P( E, F, G, H, A, B, C, D, R(60), 0x90BEFFFA );
  P( D, E, F, G, H, A, B, C, R(61), 0xA4506CEB );
  P( C, D, E, F, G, H, A, B, R(62), 0xBEF9A3F7 );
  P( B, C, D, E, F, G, H, A, R(63), 0xC67178F2 );

#undef SHR
#undef ROTR
#undef S0
#undef S1
#undef S2
#undef S3
#undef F0
#undef F1
#undef R
#undef P

  state[0] += A;
  state[1] += B;
  state[2] += C;
  state[3] += D;
  state[4] += E;
  state[5] += F;
  state[6] += G;
  state[7] += H;
}

void sha256_update (sha256_state_t * s, u8 * data, uword n_bytes)
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
        sha256_update_block (s->state, s->partial_block_buffer);
    }

  while (n_left >= 64)
    {
      sha256_update_block (s->state, d);
      n_left -= 64;
      d += 64;
    }

  memcpy (s->partial_block_buffer, d, n_left);
  s->n_bytes_processed += n_bytes;
}

static void sha256_sha224_init (sha256_state_t * s, u32 is_sha224)
{
  s->n_bytes_processed = 0;
  s->is_sha224 = is_sha224;
  if (is_sha224)
    {
      s->state[0] = 0xC1059ED8;
      s->state[1] = 0x367CD507;
      s->state[2] = 0x3070DD17;
      s->state[3] = 0xF70E5939;
      s->state[4] = 0xFFC00B31;
      s->state[5] = 0x68581511;
      s->state[6] = 0x64F98FA7;
      s->state[7] = 0xBEFA4FA4;
    }
  else
    {
      s->state[0] = 0x6A09E667;
      s->state[1] = 0xBB67AE85;
      s->state[2] = 0x3C6EF372;
      s->state[3] = 0xA54FF53A;
      s->state[4] = 0x510E527F;
      s->state[5] = 0x9B05688C;
      s->state[6] = 0x1F83D9AB;
      s->state[7] = 0x5BE0CD19;
    }
}

void sha256_init (sha256_state_t * s)
{ sha256_sha224_init (s, /* is_sha224 */ 0); }

void sha256_finalize (sha256_state_t * s, u8 * result)
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

  sha256_update_block (s->state, last_block + 0);
  if (n_last_block > 64)
    sha256_update_block (s->state, last_block + 64);

  /* Remove traces of message stored in stack. */
  crypto_zero_memory (last_block, n_left);
  crypto_zero_memory (s->partial_block_buffer, n_left);

  /* Return result. */
#define _(i) ((u32 *) result)[i] = clib_host_to_big_u32 (s->state[i]);
  _ (0); _ (1); _ (2); _ (3); _ (4); _ (5); _ (6);
  if (! s->is_sha224) _ (7);
#undef _
}

void sha256 (u8 * result, u8 * data, uword n_bytes)
{
  sha256_state_t s;
  sha256_init (&s);
  sha256_update (&s, data, n_bytes);
  sha256_finalize (&s, result);
}

void sha224_init (sha256_state_t * s)
{ sha256_sha224_init (s, /* is_sha224 */ 1); }

void sha224_update (sha224_state_t * s, u8 * data, uword n_bytes)
{ sha256_update (s, data, n_bytes); }

void sha224_finalize (sha224_state_t * s, u8 * result)
{ sha256_finalize (s, result); }

void sha224 (u8 * result, u8 * data, uword n_bytes)
{
  sha224_state_t s;
  sha224_init (&s);
  sha224_update (&s, data, n_bytes);
  sha224_finalize (&s, result);
}

static void
sha512_update_block (u64 * state, u8 * data)
{
  int i;
  u64 temp1, temp2, W[80];
  u64 A, B, C, D, E, F, G, H;

#define UL64(x) x##ULL

  static u64 K[80] = {
    UL64(0x428A2F98D728AE22),  UL64(0x7137449123EF65CD),
    UL64(0xB5C0FBCFEC4D3B2F),  UL64(0xE9B5DBA58189DBBC),
    UL64(0x3956C25BF348B538),  UL64(0x59F111F1B605D019),
    UL64(0x923F82A4AF194F9B),  UL64(0xAB1C5ED5DA6D8118),
    UL64(0xD807AA98A3030242),  UL64(0x12835B0145706FBE),
    UL64(0x243185BE4EE4B28C),  UL64(0x550C7DC3D5FFB4E2),
    UL64(0x72BE5D74F27B896F),  UL64(0x80DEB1FE3B1696B1),
    UL64(0x9BDC06A725C71235),  UL64(0xC19BF174CF692694),
    UL64(0xE49B69C19EF14AD2),  UL64(0xEFBE4786384F25E3),
    UL64(0x0FC19DC68B8CD5B5),  UL64(0x240CA1CC77AC9C65),
    UL64(0x2DE92C6F592B0275),  UL64(0x4A7484AA6EA6E483),
    UL64(0x5CB0A9DCBD41FBD4),  UL64(0x76F988DA831153B5),
    UL64(0x983E5152EE66DFAB),  UL64(0xA831C66D2DB43210),
    UL64(0xB00327C898FB213F),  UL64(0xBF597FC7BEEF0EE4),
    UL64(0xC6E00BF33DA88FC2),  UL64(0xD5A79147930AA725),
    UL64(0x06CA6351E003826F),  UL64(0x142929670A0E6E70),
    UL64(0x27B70A8546D22FFC),  UL64(0x2E1B21385C26C926),
    UL64(0x4D2C6DFC5AC42AED),  UL64(0x53380D139D95B3DF),
    UL64(0x650A73548BAF63DE),  UL64(0x766A0ABB3C77B2A8),
    UL64(0x81C2C92E47EDAEE6),  UL64(0x92722C851482353B),
    UL64(0xA2BFE8A14CF10364),  UL64(0xA81A664BBC423001),
    UL64(0xC24B8B70D0F89791),  UL64(0xC76C51A30654BE30),
    UL64(0xD192E819D6EF5218),  UL64(0xD69906245565A910),
    UL64(0xF40E35855771202A),  UL64(0x106AA07032BBD1B8),
    UL64(0x19A4C116B8D2D0C8),  UL64(0x1E376C085141AB53),
    UL64(0x2748774CDF8EEB99),  UL64(0x34B0BCB5E19B48A8),
    UL64(0x391C0CB3C5C95A63),  UL64(0x4ED8AA4AE3418ACB),
    UL64(0x5B9CCA4F7763E373),  UL64(0x682E6FF3D6B2B8A3),
    UL64(0x748F82EE5DEFB2FC),  UL64(0x78A5636F43172F60),
    UL64(0x84C87814A1F0AB72),  UL64(0x8CC702081A6439EC),
    UL64(0x90BEFFFA23631E28),  UL64(0xA4506CEBDE82BDE9),
    UL64(0xBEF9A3F7B2C67915),  UL64(0xC67178F2E372532B),
    UL64(0xCA273ECEEA26619C),  UL64(0xD186B8C721C0C207),
    UL64(0xEADA7DD6CDE0EB1E),  UL64(0xF57D4F7FEE6ED178),
    UL64(0x06F067AA72176FBA),  UL64(0x0A637DC5A2C898A6),
    UL64(0x113F9804BEF90DAE),  UL64(0x1B710B35131C471B),
    UL64(0x28DB77F523047D84),  UL64(0x32CAAB7B40C72493),
    UL64(0x3C9EBE0A15C9BEBC),  UL64(0x431D67C49C100D4C),
    UL64(0x4CC5D4BECB3E42B6),  UL64(0x597F299CFC657E2A),
    UL64(0x5FCB6FAB3AD6FAEC),  UL64(0x6C44198C4A475817),
  };

#define  SHR(x,n) (x >> n)
#define ROTR(x,n) (SHR(x,n) | (x << (64 - n)))

#define S0(x) (ROTR(x, 1) ^ ROTR(x, 8) ^  SHR(x, 7))
#define S1(x) (ROTR(x,19) ^ ROTR(x,61) ^  SHR(x, 6))

#define S2(x) (ROTR(x,28) ^ ROTR(x,34) ^ ROTR(x,39))
#define S3(x) (ROTR(x,14) ^ ROTR(x,18) ^ ROTR(x,41))

#define F0(x,y,z) ((x & y) | (z & (x | y)))
#define F1(x,y,z) (z ^ (x & (y ^ z)))

#define P(a,b,c,d,e,f,g,h,x,K)                  \
{                                               \
    temp1 = h + S3(e) + F1(e,f,g) + K + x;      \
    temp2 = S2(a) + F0(a,b,c);                  \
    d += temp1; h = temp1 + temp2;              \
}

#define _(i) W[i] = clib_big_to_host_mem_u64 ((u64 *) (data + sizeof(u64)*(i)))
  _ (0x0); _ (0x1); _ (0x2); _ (0x3); _ (0x4); _ (0x5); _ (0x6); _ (0x7);
  _ (0x8); _ (0x9); _ (0xa); _ (0xb); _ (0xc); _ (0xd); _ (0xe); _ (0xf);
#undef _ 

    for (i = 16; i < 80; i++)
      W[i] = S1(W[i -  2]) + W[i -  7] + S0(W[i - 15]) + W[i - 16];

    A = state[0];
    B = state[1];
    C = state[2];
    D = state[3];
    E = state[4];
    F = state[5];
    G = state[6];
    H = state[7];
    i = 0;

    do {
      P( A, B, C, D, E, F, G, H, W[i], K[i] ); i++;
      P( H, A, B, C, D, E, F, G, W[i], K[i] ); i++;
      P( G, H, A, B, C, D, E, F, W[i], K[i] ); i++;
      P( F, G, H, A, B, C, D, E, W[i], K[i] ); i++;
      P( E, F, G, H, A, B, C, D, W[i], K[i] ); i++;
      P( D, E, F, G, H, A, B, C, W[i], K[i] ); i++;
      P( C, D, E, F, G, H, A, B, W[i], K[i] ); i++;
      P( B, C, D, E, F, G, H, A, W[i], K[i] ); i++;
    } while( i < 80 );

    state[0] += A;
    state[1] += B;
    state[2] += C;
    state[3] += D;
    state[4] += E;
    state[5] += F;
    state[6] += G;
    state[7] += H;

#undef SHR
#undef ROTR
#undef S0
#undef S1
#undef S2
#undef S3
#undef F0
#undef F1
#undef P
}

void sha512_update (sha512_state_t * s, u8 * data, uword n_bytes)
{
  uword n_left = n_bytes;
  uword n_bytes_in_partial_block_buffer;
  u8 * d = data;
  u32 n_bytes_per_block = sizeof (s->partial_block_buffer);

  n_bytes_in_partial_block_buffer = s->n_bytes_processed % n_bytes_per_block;
  if (n_bytes_in_partial_block_buffer > 0)
    {
      uword n_bytes_to_partial_block_buffer = clib_min (n_bytes_per_block - n_bytes_in_partial_block_buffer,
                                                        n_bytes);
      memcpy (s->partial_block_buffer + n_bytes_in_partial_block_buffer, d, n_bytes_to_partial_block_buffer);
      n_left -= n_bytes_to_partial_block_buffer;
      d += n_bytes_to_partial_block_buffer;
      n_bytes_in_partial_block_buffer += n_bytes_to_partial_block_buffer;
      if (n_bytes_in_partial_block_buffer == n_bytes_per_block)
        sha512_update_block (s->state, s->partial_block_buffer);
    }

  while (n_left >= n_bytes_per_block)
    {
      sha512_update_block (s->state, d);
      n_left -= n_bytes_per_block;
      d += n_bytes_per_block;
    }

  memcpy (s->partial_block_buffer, d, n_left);
  s->n_bytes_processed += n_bytes;
}

static void sha512_sha384_init (sha512_state_t * s, u32 is_sha384)
{
  s->n_bytes_processed = 0;
  s->is_sha384 = is_sha384;
  if (is_sha384)
    {
      s->state[0] = UL64(0xCBBB9D5DC1059ED8);
      s->state[1] = UL64(0x629A292A367CD507);
      s->state[2] = UL64(0x9159015A3070DD17);
      s->state[3] = UL64(0x152FECD8F70E5939);
      s->state[4] = UL64(0x67332667FFC00B31);
      s->state[5] = UL64(0x8EB44A8768581511);
      s->state[6] = UL64(0xDB0C2E0D64F98FA7);
      s->state[7] = UL64(0x47B5481DBEFA4FA4);
    }
  else
    {
      s->state[0] = UL64(0x6A09E667F3BCC908);
      s->state[1] = UL64(0xBB67AE8584CAA73B);
      s->state[2] = UL64(0x3C6EF372FE94F82B);
      s->state[3] = UL64(0xA54FF53A5F1D36F1);
      s->state[4] = UL64(0x510E527FADE682D1);
      s->state[5] = UL64(0x9B05688C2B3E6C1F);
      s->state[6] = UL64(0x1F83D9ABFB41BD6B);
      s->state[7] = UL64(0x5BE0CD19137E2179);
    }
}

void sha512_init (sha512_state_t * s)
{ sha512_sha384_init (s, /* is_sha384 */ 0); }

void sha512_finalize (sha512_state_t * s, u8 * result)
{
  u8 last_block[256];
  u32 n_last_block;
  u32 n_left = s->n_bytes_processed % sizeof (s->partial_block_buffer);

  n_last_block = 256 - 128*(n_left < 112);
    
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

  sha512_update_block (s->state, last_block + 0);
  if (n_last_block > 128)
    sha512_update_block (s->state, last_block + 128);

  /* Remove traces of message stored in stack. */
  crypto_zero_memory (last_block, n_left);
  crypto_zero_memory (s->partial_block_buffer, n_left);

  /* Return result. */
#define _(i) ((u64 *) result)[i] = clib_host_to_big_u64 (s->state[i]);

  _ (0); _ (1); _ (2); _ (3); _ (4); _ (5);

  if (! s->is_sha384) 
    {
      _ (6);
      _ (7);
    }
#undef _
}

void sha512 (u8 * result, u8 * data, uword n_bytes)
{
  sha512_state_t s;
  sha512_init (&s);
  sha512_update (&s, data, n_bytes);
  sha512_finalize (&s, result);
}

void sha384_init (sha384_state_t * s)
{ sha512_sha384_init (s, /* is_sha384 */ 1); }

void sha384_update (sha384_state_t * s, u8 * data, uword n_bytes)
{ sha512_update (s, data, n_bytes); }

void sha384_finalize (sha384_state_t * s, u8 * result)
{ sha512_finalize (s, result); }

void sha384 (u8 * result, u8 * data, uword n_bytes)
{
  sha384_state_t s;
  sha384_init (&s);
  sha384_update (&s, data, n_bytes);
  sha384_finalize (&s, result);
}
