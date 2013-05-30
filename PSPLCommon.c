//
//  PSPLCommon.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <PSPL/PSPL.h>

#pragma mark Hash Manipulation

/* Putting these here */
void pspl_hash_fmt(char* out, const pspl_hash* hash) {
    int i;
    for (i=0 ; i<sizeof(pspl_hash) ; ++i) {
        sprintf(out, "%02X", (unsigned int)(hash->b[i]));
        out += 2;
    }
}
void pspl_hash_parse(pspl_hash* out, const char* hash_str) {
    int i;
    char byte_str[3];
    byte_str[2] = '\0';
    for (i=0 ; i<sizeof(pspl_hash) ; ++i) {
        strncpy(byte_str, hash_str, 2);
        out->b[i] = (uint8_t)strtol(byte_str, NULL, 16);
        hash_str += 2;
    }
}

#pragma mark Malloc Zone Management

void pspl_malloc_context_init(pspl_malloc_context_t* context) {
    context->object_num = 0;
    context->object_cap = 50;
    context->object_arr = calloc(50, sizeof(void*));
}

void pspl_malloc_context_destroy(pspl_malloc_context_t* context) {
    int i;
    for (i=0 ; i<context->object_num ; ++i)
        if (context->object_arr[i])
            free((void*)context->object_arr[i]);
    context->object_num = 0;
    context->object_cap = 0;
    free(context->object_arr);
}

void* pspl_malloc_malloc(pspl_malloc_context_t* context, size_t size) {
    int i;
    for (i=0 ; i<context->object_num ; ++i)
        if (!context->object_arr[i])
            return (context->object_arr[i] = malloc(size));
    if (context->object_num >= context->object_cap) {
        context->object_cap *= 2;
        context->object_arr = realloc(context->object_arr, context->object_cap*sizeof(const void*));
    }
    return (context->object_arr[context->object_num++] = malloc(size));
}

void pspl_malloc_free(pspl_malloc_context_t* context, void* ptr) {
    int i;
    for (i=0 ; i<context->object_num ; ++i)
        if (context->object_arr[i] == ptr) {
            free((void*)context->object_arr[i]);
            context->object_arr[i] = NULL;
        }
}


#pragma mark Windows Support Stuff

#ifdef _WIN32

/* The following standard implementations are copied from
 * FreeBSD's codebase. */


/* Copyright (c) 1998 Softweyr LLC.  All rights reserved.
 *
 * strtok_r, from Berkeley strtok
 * Oct 13, 1998 by Wes Peters <wes@softweyr.com>
 *
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notices, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notices, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SOFTWEYR LLC, THE REGENTS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL SOFTWEYR LLC, THE
 * REGENTS, OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

char *
strtok_r(char *s, const char *delim, char **last)
{
	char *spanp, *tok;
	int c, sc;
    
	if (s == NULL && (s = *last) == NULL)
		return (NULL);
    
	/*
	 * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
	 */
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}
    
	if (c == 0) {		/* no non-delimiter characters */
		*last = NULL;
		return (NULL);
	}
	tok = s - 1;
    
	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = '\0';
				*last = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

size_t
strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;
    
	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;
    
	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';
    
	return(dlen + (s - src));	/* count does not include NUL */
}

size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
    
	if (!dst || !src)
		return 0;
    
	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}
    
	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}
    
	return(s - src - 1);	/* count does not include NUL */
}

#endif

