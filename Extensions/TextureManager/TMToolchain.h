//
//  TMToolchain.h
//  PSPL
//
//  Created by Jack Andersen on 6/3/13.
//
//

#ifndef PSPL_TMToolchain_h
#define PSPL_TMToolchain_h

/* Image type enumeration */
enum PSPL_TM_IMAGE_TYPE {
    PSPL_TM_IMAGE_NONE = 0,
    PSPL_TM_IMAGE_RGBA = 1,
    PSPL_TM_IMAGE_RGB  = 2,
    PSPL_TM_IMAGE_RG   = 3,
    PSPL_TM_IMAGE_R    = 4,
    PSPL_TM_IMAGE_IDX  = 5
};

/* PSPL TextureManager common image type */
#define PSPL_FLOAT_DEPTH 0xffffffff
typedef struct {
    enum PSPL_TM_IMAGE_TYPE image_type;
    unsigned int width, height;
    union {
        struct {
            uint32_t r_depth, g_depth, b_depth, a_depth;
        };
        struct {
            uint32_t idx_depth;
        };
    } depth;
    const void* scanline_image_data;
} pspl_tm_image_t;



/* Decoder hook type */
typedef int(*pspl_tm_decoder_hook)(const char* file_path, const char* file_path_ext,
                                   pspl_tm_image_t* image_out);

/* Decoder type */
typedef struct {
    const char* name;
    const char* desc;
    pspl_tm_decoder_hook decoder_hook;
} pspl_tm_decoder_t;



/* Encoder hook type */
typedef int(*pspl_tm_encoder_hook)(const char* file_path, const char* file_path_ext,
                                   const pspl_tm_image_t* image_in);

/* Encoder type */
typedef struct {
    const char* name;
    const char* desc;
    pspl_tm_encoder_hook encoder_hook;
} pspl_tm_encoder_t;


#endif
