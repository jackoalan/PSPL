//
//  Buffer.h
//  PSPL
//
//  Created by Jack Andersen on 5/1/13.
//
//

#ifndef PSPL_Buffer_h
#define PSPL_Buffer_h

#include <stdlib.h>

/* This API provides a common way to maintain an externally "owned"
 * append-mutable string buffer (using a common struct to maintain buffer state). */

/* Any error conditions are handled by `pspl_error` 
 * (resulting in program termination) */

/* First, the buffer state structure */
typedef struct {
    
    size_t buf_cap;
    char* buf;
    char* buf_cur;
    
} pspl_buffer_t;

/* Init a buffer with specified size */
void pspl_buffer_init(pspl_buffer_t* buf, size_t cap);

/* Append a copied string */
void pspl_buffer_addstr(pspl_buffer_t* buf, const char* str);
void pspl_buffer_addstrn(pspl_buffer_t* buf, const char* str, size_t str_len);

/* Append a single character */
void pspl_buffer_addchar(pspl_buffer_t* buf, char ch);

/* Free memory used by the buffer */
void pspl_buffer_free(pspl_buffer_t* buf);

#endif

