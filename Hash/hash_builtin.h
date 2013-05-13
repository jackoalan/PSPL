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

union _buffer {
	uint8_t b[BLOCK_LENGTH];
	uint32_t w[BLOCK_LENGTH/4];
};

union _state {
	uint8_t b[HASH_LENGTH];
	uint32_t w[HASH_LENGTH/4];
};

typedef struct sha1nfo {
	union _buffer buffer;
	uint8_t bufferOffset;
	union _state state;
	uint32_t byteCount;
	uint8_t keyBuffer[BLOCK_LENGTH];
	uint8_t innerHash[HASH_LENGTH];
} sha1nfo;

/**
 */
void sha1_init(sha1nfo *s);
/**
 */
void sha1_writebyte(sha1nfo *s, uint8_t data);
/**
 */
void sha1_write(sha1nfo *s, const char *data, size_t len);
/**
 */
uint8_t* sha1_result(sha1nfo *s);

#define PSPL_HASH_LENGTH HASH_LENGTH

/* PSPL defines */
typedef sha1nfo pspl_hash_ctx_t;
#define pspl_hash_init(ctx_ptr) sha1_init(ctx_ptr)
#define pspl_hash_write(ctx_ptr, data_ptr, len) sha1_write(ctx_ptr, (const char*)data_ptr, len)
#define pspl_hash_result(ctx_ptr, out_ptr) out_ptr = (pspl_hash*)sha1_result(ctx_ptr)

#endif
