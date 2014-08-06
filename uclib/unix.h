/*
  Copyright (c) 2005 Eliot Dresselhaus

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

#ifndef included_clib_unix_h
#define included_clib_unix_h

#include <uclib/error.h>

/* Number of bytes in a Unix file. */
clib_error_t * unix_file_n_bytes (char * file, uword * result);

/* Read file contents into given buffer. */
clib_error_t *
unix_file_read_contents (char * file, u8 * result, uword n_bytes);

/* Read and return contents of Unix file. */
clib_error_t * unix_file_contents (char * file, u8 ** result);

/* As above but for /proc file system on Linux. */
clib_error_t * unix_proc_file_contents (char * file, u8 ** result);

/* Call function foreach regular file in directory. */
clib_error_t *
unix_foreach_directory_file (char * root_dir_name,
			     clib_error_t * (* f) (void * arg, u8 * path_name, u8 * file_name),
			     void * arg,
			     int recursive);

#endif /* included_clib_unix_h */
