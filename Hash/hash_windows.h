//
//  hash_windows.h
//  PSPL
//
//  Created by Jack Andersen on 5/11/13.
//
//

#ifndef PSPL_hash_windows_h
#define PSPL_hash_windows_h

#ifdef _WIN32

#include <windows.h>
#include <wincrypt.h>

typedef struct {
    HCRYPTPROV crypt_ctx;
    HCRYPTHASH hash_ctx;
    DWORD result[20];
} pspl_hash_ctx_t;

static inline void init_sha1_win(pspl_hash_ctx_t* ctx) {
    
}

static inline void finish_sha1_win(pspl_hash_ctx_t* ctx) {
    
}

/* PSPL defines */

#define pspl_hash_init(ctx_ptr) init_sha1_win(ctx_ptr)
#define pspl_hash_write(ctx_ptr, data_ptr, len) CryptHashData((ctx_ptr)->hash_ctx, (PBYTE)(data_ptr), len, 0)
#define pspl_hash_result(ctx_ptr, out_ptr) finish_sha1_win(ctx_ptr); out_ptr = (pspl_hash*)(ctx_ptr)->result

#else
#  error Unable to use windows hashing algorithm; not targeting windows
#endif // _WIN32

#endif
