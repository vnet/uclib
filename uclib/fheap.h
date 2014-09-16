#ifndef included_clib_fheap_h
#define included_clib_fheap_h

/* Fibonacci Heaps Fredman, M. L.; Tarjan (1987).
   "Fibonacci heaps and their uses in improved network optimization algorithms" */

typedef struct {
  /* Node index of parent. */
  u32 parent;

  /* Node index of first child. */
  u32 first_child;

  /* Next and previous nodes in doubly linked list of siblings. */
  u32 next_sibling, prev_sibling;

  /* Key (distance) for this node.
     Min. node has smallest key.
     Parent always has key <= than keys of children. */
  f64 key;

  /* Number of children for this node (as opposed to descendents). */
  u32 rank;

  u16 is_marked;

  /* Set to one when node is inserted; zero when deleted. */
  u16 is_valid;
} fheap_node_t;

typedef struct {
  /* Vector of nodes. */
  fheap_node_t * nodes;

  u32 * root_list_by_rank;

  /* Node index of minimum element.  ~0 for none. */
  u32 min_root;

  u32 enable_validate;

  u32 validate_serial;
} fheap_t;

/* Initialize empty heap. */
always_inline void
fheap_init (fheap_t * f)
{
  memset (f, 0, sizeof (f[0]));
  f->min_root = ~0;
}

always_inline void
fheap_free (fheap_t * f)
{
  vec_free (f->nodes);
  vec_free (f->root_list_by_rank);
}

/* Return index with minimal key. */
always_inline u32
fheap_find_min (fheap_t * f)
{ return f->min_root; }

always_inline u32
fheap_is_empty (fheap_t * f)
{ return f->min_root == ~0; }

/* Add/delete nodes. */
void fheap_add (fheap_t * f, u32 ni, f64 key);
void fheap_del (fheap_t * f, u32 ni);

/* Delete and return minimum. */
u32 fheap_del_min (fheap_t * f, f64 * min_key);

/* Decrease key value. */
void fheap_decrease_key (fheap_t * f, u32 ni, f64 new_key);

#endif /* included_clib_fheap_h */
