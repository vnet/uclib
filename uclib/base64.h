#ifndef included_clib_base64_h
#define included_clib_base64_h

/* Formats given byte stream and length as base64 string.
   Args: u8 * data, u32 n_data_bytes. */
format_function_t format_base64_data;

/* Inverse of above. Result put in vector: u8 ** result. */
unformat_function_t unformat_base64_data;

#endif /* included_clib_base64_h */
