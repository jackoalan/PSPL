//
//  hash_builtin.h
//  PSPL
//
//  Created by Jack Andersen on 5/11/13.
//
//

#ifndef PSPL_hash_builtin_h
#define PSPL_hash_builtin_h

#define HASH_LENGTH 20
#define BLOCK_LENGTH 64

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t  buffer[64];
    uint8_t  result[20];
} SHA1_CTX;

#define SHA1_DIGEST_SIZE 20

void SHA1_Init(SHA1_CTX* context);
void SHA1_Update(SHA1_CTX* context, const uint8_t* data, const size_t len);
void SHA1_Final(SHA1_CTX* context, uint8_t digest[SHA1_DIGEST_SIZE]);


#define PSPL_HASH_LENGTH HASH_LENGTH

/* PSPL defines */
typedef SHA1_CTX pspl_hash_ctx_t;
#define pspl_hash_init(ctx_ptr) SHA1_Init((SHA1_CTX*)(ctx_ptr))
#define pspl_hash_write(ctx_ptr, data_ptr, len) SHA1_Update((SHA1_CTX*)(ctx_ptr), (const uint8_t*)(data_ptr), (const size_t)(len))
#define pspl_hash_result(ctx_ptr, out_ptr) SHA1_Final((SHA1_CTX*)(ctx_ptr), ((SHA1_CTX*)ctx_ptr)->result); out_ptr = (pspl_hash*)((SHA1_CTX*)ctx_ptr)->result

#endif
