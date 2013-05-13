//
//  hash_commoncrypto.h
//  PSPL
//
//  Created by Jack Andersen on 5/11/13.
//
//

#ifndef PSPL_hash_commoncrypto_h
#define PSPL_hash_commoncrypto_h

#include <CommonCrypto/CommonDigest.h>

#define PSPL_HASH_LENGTH CC_SHA1_DIGEST_LENGTH

/* PSPL defines */
typedef struct {
    CC_SHA1_CTX sha_ctx;
    unsigned char result[CC_SHA1_DIGEST_LENGTH];
} pspl_hash_ctx_t;
#define pspl_hash_init(ctx_ptr) CC_SHA1_Init(&(ctx_ptr)->sha_ctx)
#define pspl_hash_write(ctx_ptr, data_ptr, len) CC_SHA1_Update(&(ctx_ptr)->sha_ctx, (const void*)data_ptr, (CC_LONG)len)
#define pspl_hash_result(ctx_ptr, out_ptr) CC_SHA1_Final((ctx_ptr)->result, &(ctx_ptr)->sha_ctx); out_ptr = (pspl_hash*)(ctx_ptr)->result

#endif
