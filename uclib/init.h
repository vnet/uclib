#ifndef included_clib_init_h
#define included_clib_init_h

#define CLIB_INIT_VAR(TYPE,TAG) __clib_init_##TAG##_##TYPE

#define CLIB_INIT_LINK_TYPE(TYPE) CLIB_INIT_VAR(TYPE,link_type)
#define CLIB_INIT_LINK_TYPEDEF(TYPE)                    \
  typedef struct CLIB_INIT_VAR(TYPE,_link_type) {       \
    TYPE * this;                                        \
    struct CLIB_INIT_VAR(TYPE,_link_type) * next;       \
  } CLIB_INIT_VAR(TYPE,link_type)

/* Adds an instance of an init type. */
#define CLIB_INIT_ADD(TYPE,SYM_TO_ADD)                  \
  __attribute__ ((constructor))                         \
  static void CLIB_INIT_VAR (TYPE, add_##SYM_TO_ADD) () \
  {                                                     \
    static CLIB_INIT_LINK_TYPE (TYPE) this;             \
    this.this = &SYM_TO_ADD;                            \
    this.next = CLIB_INIT_VAR (TYPE, head);             \
    CLIB_INIT_VAR (TYPE, head) = &this;                 \
  }

/* Iterate through all defined init variables of given type. */
#define foreach_clib_init_with_type(VAR,TYPE,BODY)                      \
do {                                                                    \
  CLIB_INIT_LINK_TYPE (TYPE) * CLIB_INIT_VAR (TYPE, link) = CLIB_INIT_VAR (TYPE, head); \
  while (CLIB_INIT_VAR (TYPE, link))                                    \
    {                                                                   \
      (VAR) = CLIB_INIT_VAR (TYPE, link)->this;                         \
      do { BODY; } while (0);                                           \
      CLIB_INIT_VAR (TYPE, link) = CLIB_INIT_VAR (TYPE, link)->next;    \
    }                                                                   \
} while (0)

/* Add new init type. */
#define CLIB_INIT_ADD_TYPE(TYPE)                                \
  CLIB_INIT_LINK_TYPEDEF (TYPE);                                \
  CLIB_INIT_LINK_TYPE (TYPE) * CLIB_INIT_VAR (TYPE, head);

#define CLIB_INIT_TYPE_IS_NON_EMPTY(TYPE) (CLIB_INIT_VAR (TYPE, head) != 0)

#endif /* included_clib_init_h */
