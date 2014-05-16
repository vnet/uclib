#ifndef included_uclib_error_h
#define included_uclib_error_h

enum {
  CLIB_ERROR_FATAL	= 1 << 0,
  CLIB_ERROR_ABORT	= 1 << 1,
  CLIB_ERROR_WARNING	= 1 << 2,
  CLIB_ERROR_ERRNO_VALID = 1 << 16,
  CLIB_ERROR_NO_RATE_LIMIT = 1 << 17,
};

/* Current function name.  Need (char *) cast to silence gcc4 pointer signedness warning. */
#define clib_error_function ((char *) __FUNCTION__)

#ifndef CLIB_ASSERT_ENABLE
#define CLIB_ASSERT_ENABLE (CLIB_DEBUG > 0)
#endif

/* Low level error reporting function.
   Code specifies whether to call exit, abort or nothing at
   all (for non-fatal warnings). */
extern void _clib_error (int code,
			 char * function_name,
			 uword line_number,
			 char * format, ...);

#define ASSERT(truth)					\
do {							\
  if (CLIB_ASSERT_ENABLE && ! (truth))			\
    {							\
      _clib_error (CLIB_ERROR_ABORT, 0, 0,		\
		   "%s:%d (%s) assertion `%s' fails",	\
		   __FILE__,				\
		   (uword) __LINE__,			\
		   clib_error_function,			\
		   # truth);				\
    }							\
} while (0)

/* Assert without allocating memory. */
#define ASSERT_AND_PANIC(truth)			\
do {						\
  if (CLIB_ASSERT_ENABLE && ! (truth))		\
    os_panic ();				\
} while (0)

typedef struct {
  /* Error message. */
  u8 * what;

  /* Where error occurred (e.g. __FUNCTION__ __LINE__) */
  const u8 * where;

  uword flags;

  /* Error code (e.g. errno for Unix errors). */
  uword code;
} clib_error_t;

#endif /* included_uclib_error_h */
