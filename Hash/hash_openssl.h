//
//  hash_openssl.h
//  PSPL
//
//  Created by Jack Andersen on 5/11/13.
//
//

#ifndef PSPL_hash_openssl_h
#define PSPL_hash_openssl_h

#include <openssl/sha.h>

#define PSPL_HASH_LENGTH SHA_DIGEST_LENGTH

/* PSPL defines */
typedef struct {
    SHA_CTX sha_ctx;
    unsigned char result[SHA_DIGEST_LENGTH];
} pspl_hash_ctx_t;
#define pspl_hash_init(ctx_ptr) SHA1_Init(&(ctx_ptr)->sha_ctx)
#define pspl_hash_write(ctx_ptr, data_ptr, len) SHA1_Update(&(ctx_ptr)->sha_ctx, (const void*)data_ptr, len)
#define pspl_hash_result(ctx_ptr, out_ptr) SHA1_Final((ctx_ptr)->result, &(ctx_ptr)->sha_ctx); out_ptr = (ctx_ptr)->result

#endif
