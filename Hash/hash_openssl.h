//
//  hash_openssl.h
//  PSPL
//
//  Created by Jack Andersen on 5/11/13.
//
//

#ifndef PSPL_hash_openssl_h
#define PSPL_hash_openssl_h

#include <openssl/evp.h>
#include <openssl/sha.h>

#define PSPL_HASH_LENGTH SHA_DIGEST_LENGTH

/* PSPL defines */
typedef struct {
    EVP_MD_CTX sha_ctx;
    unsigned char result[SHA_DIGEST_LENGTH];
} pspl_hash_ctx_t;
#define pspl_hash_init(ctx_ptr) EVP_DigestInit(&(ctx_ptr)->sha_ctx, EVP_sha1())
#define pspl_hash_write(ctx_ptr, data_ptr, len) EVP_DigestUpdate(&(ctx_ptr)->sha_ctx, (const void*)data_ptr, len)
#define pspl_hash_result(ctx_ptr, out_ptr) EVP_DigestFinal(&(ctx_ptr)->sha_ctx, (ctx_ptr)->result, NULL); out_ptr = (ctx_ptr)->result

#endif
