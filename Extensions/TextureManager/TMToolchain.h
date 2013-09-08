//
//  TMToolchain.h
//  PSPL
//
//  Created by Jack Andersen on 6/3/13.
//
//

#ifndef PSPL_TMToolchain_h
#define PSPL_TMToolchain_h

#include <stdlib.h>
#include <stdint.h>
#include <PSPL/PSPLCommon.h>

/* Image type enumeration */
enum PSPL_TM_IMAGE_TYPE {
    PSPL_TM_IMAGE_NONE = 0,
    PSPL_TM_IMAGE_R    = 1,
    PSPL_TM_IMAGE_RG   = 2,
    PSPL_TM_IMAGE_RGB  = 3,
    PSPL_TM_IMAGE_RGBA = 4,
    PSPL_TM_IMAGE_IDX  = 8
};

/* PSPL TextureManager common image type */
typedef struct {
    enum PSPL_TM_IMAGE_TYPE image_type;
    unsigned int width, height;
    uint8_t* image_buffer;
    uint8_t* index_buffer;
} pspl_tm_image_t;



/* Decoder hook type */
typedef int(*pspl_tm_decoder_hook)(const char* file_path, const char* file_path_ext,
                                   pspl_tm_image_t* image_out);

/* Decoder type */
typedef struct {
    const char* name;
    const char* desc;
    const char** extensions;
    pspl_tm_decoder_hook decoder_hook;
} pspl_tm_decoder_t;



/* Encoder hook type */
typedef int(*pspl_tm_encoder_hook)(pspl_malloc_context_t* mem_ctx, const uint8_t* image_in,
unsigned chan_count, unsigned width, unsigned height, uint8_t** image_out, size_t* size_out);

/* Encoder type */
typedef struct {
    const char* name;
    const char* desc;
    pspl_tm_encoder_hook encoder_hook;
} pspl_tm_encoder_t;


#endif
