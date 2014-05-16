/*
  Copyright (c) 2004 Eliot Dresselhaus

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

typedef struct clib_generic_stack_frame_t {
  struct clib_generic_stack_frame_t * prev;
  void * return_address;
} clib_generic_stack_frame_t;

/* This will only work if we have a frame pointer.
   Without a frame pointer we have to parse the machine code to
   parse the stack frames. */
uword
clib_backtrace (uword * callers,
		uword max_callers,
		uword n_frames_to_skip)
{
  clib_generic_stack_frame_t * f;
  uword i;

  f = __builtin_frame_address (0);

  /* Also skip current frame. */
  n_frames_to_skip += 1;

  for (i = 0; i < max_callers + n_frames_to_skip; i++)
    {
      f = f->prev;
      if (! f)
	goto backtrace_done;
      if (uclib_abs ((void *) f - (void *) f->prev) > (64*1024))
	goto backtrace_done;
      if (i >= n_frames_to_skip)
	callers[i - n_frames_to_skip] = pointer_to_uword (f->return_address);
    }

 backtrace_done:
  if (i < n_frames_to_skip)
      return 0;
  else
      return i - n_frames_to_skip;
}
