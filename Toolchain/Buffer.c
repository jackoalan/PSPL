//
//  Buffer.c
//  PSPL
//
//  Created by Jack Andersen on 5/1/13.
//
//

#define PSPL_INTERNAL
#define PSPL_TOOLCHAIN

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "Driver.h"
#include "Buffer.h"

/* Check buffer capacity (realloc if needed) */
static void pspl_buffer_check_cap(pspl_buffer_t* buf,
                                  size_t need_sz) {
    
    // Check the current buffer
    size_t cur_diff = buf->buf_cur - buf->buf;
    if ((cur_diff+need_sz) > buf->buf_cap) {
        
        // Make a larger buffer
        void* new_buf = realloc(buf->buf, (buf->buf_cap)*2);
        if (!new_buf)
            pspl_error(-1, "Unable to reallocate data buffer",
                       "Attempted to reallocate %u bytes; errno %d: %s",
                       (buf->buf_cap)*2, errno, strerror(errno));
        
        // Update buffer state with larger buffer
        buf->buf = new_buf;
        buf->buf_cur = new_buf + cur_diff;
        
    }
    
}

/* Init a buffer with specified initial capacity */
void pspl_buffer_init(pspl_buffer_t* buf, size_t cap) {
    buf->buf = malloc(cap);
    if (!buf->buf)
        pspl_error(-1, "Unable to allocate data buffer",
                   "Attempted to allocate %u bytes; errno %d: %s",
                   cap, errno, strerror(errno));
    buf->buf_cur = buf->buf;
    buf->buf_cap = cap;
    *buf->buf_cur = '\0';
}

/* Append a copied string */
void pspl_buffer_addstr(pspl_buffer_t* buf, const char* str) {
    size_t str_len = strlen(str);
    pspl_buffer_check_cap(buf, str_len+1);
    strncpy(buf->buf_cur, str, str_len);
    buf->buf_cur += str_len;
    *buf->buf_cur = '\0';
}
void pspl_buffer_addstrn(pspl_buffer_t* buf, const char* str, size_t str_len) {
    if (!str_len)
        return;
    str_len = strnlen(str, str_len);
    pspl_buffer_check_cap(buf, str_len+1);
    strncpy(buf->buf_cur, str, str_len);
    buf->buf_cur += str_len;
    *buf->buf_cur = '\0';
}

/* Append a single character */
void pspl_buffer_addchar(pspl_buffer_t* buf, char ch) {
    pspl_buffer_check_cap(buf, 2);
    *buf->buf_cur = ch;
    ++buf->buf_cur;
    *buf->buf_cur = '\0';
}

/* Free memory used by the buffer */
void pspl_buffer_free(pspl_buffer_t* buf) {
    free(buf->buf);
    buf->buf = NULL;
    buf->buf_cur = NULL;
    buf->buf_cap = 0;
}


