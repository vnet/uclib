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

#ifndef included_uclib_crypto_h
#define included_uclib_crypto_h

typedef struct {
  u64 n_bytes_processed;
  u8 partial_block_buffer[64];
  u32 state[5];
} sha1_state_t;

void sha1_init (sha1_state_t * s);
void sha1_update (sha1_state_t * s, u8 * data, uword n_bytes);
void sha1_finalize (sha1_state_t * s, u8 * result);
void sha1 (u8 * result, u8 * data, uword n_bytes);

typedef struct {
  u64 n_bytes_processed;
  u8 partial_block_buffer[64];
  u32 state[8];
  u32 is_sha224;
} sha256_state_t;

void sha256_init (sha256_state_t * s);
void sha256_update (sha256_state_t * s, u8 * data, uword n_bytes);
void sha256_finalize (sha256_state_t * s, u8 * result);
void sha256 (u8 * result, u8 * data, uword n_bytes);

typedef sha256_state_t sha224_state_t;

void sha224_init (sha224_state_t * s);
void sha224_update (sha224_state_t * s, u8 * data, uword n_bytes);
void sha224_finalize (sha224_state_t * s, u8 * result);
void sha224 (u8 * result, u8 * data, uword n_bytes);

always_inline void crypto_zero_memory (void * v, uword n)
{ volatile unsigned char *p = v; while( n-- ) *p++ = 0; }

#endif /* included_uclib_crypto_h */
