/* Vector resize operator.  Called as needed by various macros such as
   vec_add1() when we need to allocate memory. */
void * vec_resize_allocate_memory (void * v,
				   word length_increment,
				   uword old_length,
				   uword n_bytes_per_elt,
				   uword header_bytes,
				   uword data_align)
{
  vec_header_t * vh = _vec_find (v);
  uword old_alloc_bytes, new_alloc_bytes;
  uword old_data_bytes, new_data_bytes;
  void * old, * new;

  header_bytes = vec_header_bytes (header_bytes);

  old_data_bytes = old_length * n_bytes_per_elt;
  new_data_bytes = (old_length + length_increment) * n_bytes_per_elt;

  old_data_bytes += header_bytes;
  new_data_bytes += header_bytes;

  /* Check for overflow (security problems). */
  if ((length_increment >= 0) != (new_data_bytes >= old_data_bytes))
    os_panic ();

  /* Zero length (null pointer) vector resize. */
  if (! v)
    {
      new = clib_mem_alloc_aligned_at_offset (new_data_bytes, data_align, header_bytes);
      new_data_bytes = clib_mem_size (new);
      memset (new, 0, new_data_bytes);
      v = new + header_bytes;
      _vec_len (v) = length_increment;
      return v;
    }

  vh->len += length_increment;
  old = v - header_bytes;

  old_alloc_bytes = clib_mem_size (old);

  /* Need to resize? */
  if (new_data_bytes <= old_alloc_bytes)
    return v;

  new_alloc_bytes = (old_alloc_bytes * 3) / 2;
  if (new_alloc_bytes < new_data_bytes)
    new_alloc_bytes = new_data_bytes;

  new = clib_mem_alloc_aligned_at_offset (new_alloc_bytes, data_align, header_bytes);

  /* FIXME fail gracefully. */
  if (! new)
    os_panic ();

  memcpy (new, old, old_alloc_bytes);
  clib_mem_free (old);
  v = new;

  /* Allocator may give a bit of extra room. */
  new_alloc_bytes = clib_mem_size (v);

  /* Zero new memory. */
  memset (v + old_alloc_bytes, 0, new_alloc_bytes - old_alloc_bytes);

  return v + header_bytes;
} 
