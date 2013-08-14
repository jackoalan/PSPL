//
//  TMToolchain.c
//  PSPL
//
//  Created by Jack Andersen on 6/3/13.
//
//

#include <string.h>
#include <stdio.h>
#include <PSPLExtension.h>
#include "TMToolchain.h"
#include "TMCommon.h"

/* General PMDL platforms */
extern pspl_platform_t GL2_platform;
extern pspl_platform_t D3D11_platform;
static const pspl_platform_t* general_plats[] = {
    &GL2_platform,
    &D3D11_platform,
    NULL};

/* GX PMDL platforms */
extern pspl_platform_t GX_platform;
static const pspl_platform_t* gx_plats[] = {
    &GX_platform,
    NULL};


extern pspl_tm_decoder_t* pspl_tm_available_decoders[];
extern pspl_tm_encoder_t* pspl_tm_available_encoders[];

/* Malloc-context for tracking converted file-names */
static pspl_malloc_context_t converted_names;

/* Routine to count set bits (for mipmap validation) */
static unsigned count_bits(unsigned int word, unsigned* index) {
    int i;
    unsigned accum = 0;
    for (i=0 ; i<32 ; ++i)
        if ((word >> i) & 1) {
            *index = i;
            ++accum;
        }
    return accum;
}

typedef struct {
    const char* name;
    const char* name_ext;
    const char* name_fext;
    uint8_t mipmap;
    uint8_t gx;
} pspl_tm_convert_t;

/* Box filter algorithm (for mipmapping) */
static void box_filter(const uint8_t* input, unsigned chan_count,
                       unsigned in_width, unsigned in_height, uint8_t* output) {
    unsigned mip_width = 1;
    unsigned mip_height = 1;
    if (in_width > 1)
        mip_width = in_width / 2;
    if (in_height > 1)
        mip_height = in_height / 2;
    
    int y,x,c;
    for (y=0 ; y<mip_height ; ++y) {
        unsigned mip_line_base = mip_width * y;
        unsigned in1_line_base = in_width * (y*2);
        unsigned in2_line_base = in_width * (y*2+1);
        for (x=0 ; x<mip_width ; ++x) {
            uint8_t* out = &output[(mip_line_base+x)*chan_count];
            for (c=0 ; c<chan_count ; ++c) {
                out[c] = 0;
                out[c] += input[(in1_line_base+(x*2))*chan_count+c] / 4;
                out[c] += input[(in1_line_base+(x*2+1))*chan_count+c] / 4;
                out[c] += input[(in2_line_base+(x*2))*chan_count+c] / 4;
                out[c] += input[(in2_line_base+(x*2+1))*chan_count+c] / 4;
            }
        }
    }
}

/* Converter hook */
static int sample_converter(void** buf_out, size_t* len_out, const char* path_in, pspl_tm_convert_t* conv) {
    int i;
    
    // Determine decoder
    pspl_tm_decoder_t** dec_arr = pspl_tm_available_decoders;
    pspl_tm_decoder_t* dec;
    uint8_t found_dec = 0;
    while ((dec = *dec_arr)) {
        const char** ext_arr = dec->extensions;
        const char* ext;
        while ((ext = *ext_arr)) {
            if (!strcasecmp(ext, conv->name_fext)) {
                found_dec = 1;
                break;
            }
            ++ext_arr;
        }
        if (found_dec)
            break;
        ++dec_arr;
    }
    if (!found_dec)
        pspl_error(-1, "No decoder to handle image", "no available decoder "
                   "to handle images with '.%s' extension",
                   conv->name_fext);
    
    // Decode image
    if (!dec->decoder_hook)
        pspl_error(-1, "Unimplemented decoder hook", "decoder '%s' doesn't implement decoder hook",
                   dec->name);
    pspl_tm_image_t image;
    int err;
    pspl_converter_progress_update(0.05);
    if ((err = dec->decoder_hook(path_in, conv->name_ext, &image)))
        pspl_error(-1, "Decoder returned error", "decoder '%s' returned error %d while processing `%s`",
                   dec->name, err, conv->name);
    pspl_converter_progress_update(0.5);
    
    // GX doesn't support 3-component textures; insert full alpha
    uint8_t* image_buffer = (uint8_t*)image.image_buffer;
    if (conv->gx && image.image_type == 3) {
        uint8_t* image_cur = (uint8_t*)image.image_buffer;
        image_buffer = malloc(image.width * image.height * 4);
        int i,j;
        for (i=0 ; i<image.width*image.height ; ++i) {
            for (j=0 ; j<3 ; ++j)
                image_buffer[i*4+j] = *(image_cur++);
            image_buffer[i*4+3] = 0xff; // Full Alpha
        }
        image.image_type = 4;
    }
    
    // Image series (for mipmapping)
    unsigned series_count = 1;
    unsigned series_pixel_count = image.width * image.height;
    
    // Validate and mipmap (if requested)
    if (conv->mipmap) {
        // Validate dimensions
        unsigned w_idx, h_idx;
        if (count_bits(image.width, &w_idx) != 1)
            pspl_error(-1, "Invalid mipmap dimensions", "image `%s` has width of %u pixels; it must "
                       "be an exponential with base 2 in order to be mipmapped", conv->name, image.width);
        if (count_bits(image.height, &h_idx) != 1)
            pspl_error(-1, "Invalid mipmap dimensions", "image `%s` has height of %u pixels; it must "
                       "be an exponential with base 2 in order to be mipmapped", conv->name, image.height);
        
        // Accumulate up buffer size
        if (image.width > image.height)
            series_count = w_idx + 1;
        else
            series_count = h_idx + 1;
        series_pixel_count = 0;
        for (i=0 ; i<series_count ; ++i) {
            series_pixel_count += (1 << w_idx) * (1 << h_idx);
            if (w_idx)
                --w_idx;
            if (h_idx)
                --h_idx;
        }
    }
    
    // Perform mipmap
    void* final_buf = malloc(series_pixel_count * image.image_type);
    void* final_cur = final_buf;
    memcpy(final_cur, image_buffer, image.width * image.height * image.image_type);
    unsigned mip_width = image.width;
    unsigned mip_height = image.height;
    for (i=1 ; i<series_count ; ++i) {
        void* next_cur = final_cur + (mip_width * mip_height * image.image_type);
        box_filter(final_cur, image.image_type, mip_width, mip_height, next_cur);
        if (mip_width > 1)
            mip_width /= 2;
        if (mip_height > 1)
            mip_height /= 2;
        final_cur = next_cur;
    }
    pspl_converter_progress_update(0.75);
    
    // Perform encode
    pspl_tm_encoder_t* enc = pspl_tm_available_encoders[0];
    if (conv->gx)
        enc = pspl_tm_available_encoders[1];
    if (!enc->encoder_hook)
        pspl_error(-1, "Unimplemented encoder hook", "encoder '%s' doesn't implement encoder hook",
                   enc->name);
    
    // Buffer array
    uint8_t** enc_bufs = calloc(series_count, sizeof(uint8_t*));
    size_t* enc_sizes = calloc(series_count, sizeof(size_t));
    size_t total_size = 0;
    
    // Malloc context for encoder
    pspl_malloc_context_t enc_ctx;
    pspl_malloc_context_init(&enc_ctx);
    
    // Iterate
    final_cur = final_buf;
    mip_width = image.width;
    mip_height = image.height;
    for (i=0 ; i<series_count ; ++i) {
        void* next_cur = final_cur + (mip_width * mip_height * image.image_type);
        int err;
        if ((err = enc->encoder_hook(&enc_ctx, final_cur, image.image_type, mip_width, mip_height, &enc_bufs[i], &enc_sizes[i])))
            pspl_error(-1, "Encoder returned error", "encoder '%s' returned error %d while processing `%s`",
                       enc->name, err, conv->name);
        total_size += enc_sizes[i];
        if (mip_width > 1)
            mip_width /= 2;
        if (mip_height > 1)
            mip_height /= 2;
        final_cur = next_cur;
    }
    
    // Populate header
    size_t data_off = ROUND_UP_32(sizeof(pspl_tm_texture_head_t) + strlen(enc->name) + 1);
    void* output_buf = malloc(data_off + ROUND_UP_32(total_size));
    memset(output_buf, 0, data_off + ROUND_UP_32(total_size));
    pspl_tm_texture_head_t* head = output_buf;
    head->key1 = 'T';
    head->chan_count = image.image_type;
    head->num_mips = series_count;
    head->data_off = data_off;
    SET_BI_U16(head->size, width, image.width);
    SET_BI_U16(head->size, height, image.height);
    strcpy(output_buf+sizeof(pspl_tm_texture_head_t), enc->name);
    
    // Copy encoded mipmap chain
    void* output_cur = output_buf + data_off;
    for (i=0 ; i<series_count ; ++i) {
        memcpy(output_cur, enc_bufs[i], enc_sizes[i]);
        output_cur += enc_sizes[i];
    }
    
    if (conv->gx && image.image_type == 3) {
        uint8_t* image_cur = (uint8_t*)image.image_buffer;
        uint8_t* new_image_cur = malloc(image.width * image.height * 4);
        int i,j;
        for (i=0 ; i<image.width*image.height ; ++i) {
            for (j=0 ; j<3 ; ++j)
                new_image_cur[i*4+j] = *(image_cur++);
            new_image_cur[i*4+3] = 0xff; // Full Alpha
        }
    }
    
    // Done with malloc context
    free(final_buf);
    free(enc_bufs);
    free(enc_sizes);
    pspl_malloc_context_destroy(&enc_ctx);
    
    // Return buffer
    *buf_out = output_buf;
    *len_out = data_off + ROUND_UP_32(total_size);
    
    
    return 0;
}

/* SAMPLE preprocessor directive handling */
static void sample_direc(const pspl_toolchain_context_t* driver_context,
                         unsigned int argc, const char** argv) {
    if (!argc)
        pspl_error(-1, "Incomplete use of [SAMPLE] directive",
                   "must follow `SAMPLE <tex_file> [LAYER <layer_name>] "
                   "[MIPMAP] UV <uv_key>` syntax");
    
    int i;
    
    // Platforms to convert for
    uint8_t make_general = 0;
    uint8_t make_gx = 0;
    for (i=0 ; i<driver_context->target_runtime_platforms_c; ++i) {
        const pspl_platform_t* plat = driver_context->target_runtime_platforms[i];
        if (plat == &GL2_platform || plat == &D3D11_platform)
            make_general = 1;
        else if (plat == &GX_platform)
            make_gx = 1;
    }
    
    // Converter state
    pspl_tm_convert_t convert_state;
    
    // Name forms
    convert_state.name = argv[0];
    convert_state.name_ext = NULL;
    
    // UV Arg
    const char* uv = NULL;
    
    // Mipmap arg
    convert_state.mipmap = 0;
    
    // Ensure name has an extension
    convert_state.name_fext = strrchr(convert_state.name, '.');
    if (!convert_state.name_fext)
        pspl_error(-1, "Image files must have a file extension", "`%s` doesn't have an extension",
                   convert_state.name);
    ++convert_state.name_fext;
    
    // Iterate remaining arguments
    for (i=1 ; i<argc ; ++i) {
        if (!strcasecmp(argv[i], "LAYER") && argc >= i) {
            convert_state.name_ext = argv[i+1];
            ++i;
        } else if (!strcasecmp(argv[i], "UV") && argc >= i) {
            uv = argv[i+1];
            ++i;
        } else if (!strcasecmp(argv[i], "MIPMAP")) {
            convert_state.mipmap = 1;
        }
    }
    
    // We need UV
    if (!uv)
        pspl_error(-1, "Incomplete use of [SAMPLE] directive",
                   "missing required 'UV' argument in `SAMPLE <tex_file> "
                   "[LAYER <layer_name>] [MIPMAP] UV <TEXGEN_IDX>` syntax");
    
    // Determine if file was already cached for this PSPLC (allows shared bindings)
    size_t name_len = strlen(convert_state.name);
    unsigned tex_idx = converted_names.object_num;
    for (i=0 ; i<converted_names.object_num ; ++i) {
        const char* prev_name = converted_names.object_arr[i];
        if (!memcmp(convert_state.name, prev_name, name_len)) {
            if (convert_state.name_ext && strcmp(convert_state.name_ext, prev_name+name_len))
                continue;
            tex_idx = i;
            break;
        }
    }
    
    // Add index as sample parameter
    char tex_idx_str[32];
    snprintf(tex_idx_str, 32, "%u", tex_idx);
    pspl_preprocessor_add_command_call("SAMPLE", tex_idx_str, uv);
    
    // Add to name cache and convert if needed
    if (tex_idx == converted_names.object_num) {
        if (!convert_state.name_ext) {
            void* name_buf = pspl_malloc_malloc(&converted_names, strlen(convert_state.name) + 1);
            sprintf(name_buf, "%s", convert_state.name);
        } else {
            void* name_buf = pspl_malloc_malloc(&converted_names, strlen(convert_state.name) +
                                                strlen(convert_state.name_ext) + 1);
            sprintf(name_buf, "%s%s", convert_state.name, convert_state.name_ext);
        }
        
        pspl_hash* hash;
        
        if (make_general) {
            convert_state.gx = 0;
            pspl_package_membuf_augment(general_plats, convert_state.name, convert_state.name_ext,
                                        (pspl_converter_membuf_hook)sample_converter, &convert_state, &hash);
            pspl_embed_integer_keyed_object(general_plats, tex_idx, hash, hash, sizeof(pspl_hash));
        }
        
        if (make_gx) {
            convert_state.gx = 1;
            pspl_package_membuf_augment(gx_plats, convert_state.name, convert_state.name_ext,
                                        (pspl_converter_membuf_hook)sample_converter, &convert_state, &hash);
            pspl_embed_integer_keyed_object(gx_plats, tex_idx, hash, hash, sizeof(pspl_hash));
        }

    }
    
}

static void subext_hook() {
    
    pspl_tm_decoder_t** dec_arr = pspl_tm_available_decoders;
    if (dec_arr[0])
        pspl_toolchain_provide_subext("Decoders", NULL, 1);
    pspl_tm_decoder_t* dec;
    while ((dec = *dec_arr)) {
        pspl_toolchain_provide_subext(dec->name, dec->desc, 2);
        ++dec_arr;
    }
    
    pspl_tm_encoder_t** enc_arr = pspl_tm_available_encoders;
    if (enc_arr[0])
        pspl_toolchain_provide_subext("Encoders", NULL, 1);
    pspl_tm_encoder_t* enc;
    while ((enc = *enc_arr)) {
        pspl_toolchain_provide_subext(enc->name, enc->desc, 2);
        ++enc_arr;
    }
    
}

static void copyright_hook() {
    
    pspl_toolchain_provide_copyright("TextureManager (Toolchain and Runtime)",
                                     "Copyright (c) 2013 Jack Andersen <jackoalan@gmail.com>",
                                     "[See licence for \"PSPL\" above]");
    
    const char* tiff_licence =
    "Permission to use, copy, modify, distribute, and sell this software and\n"
    "its documentation for any purpose is hereby granted without fee, provided\n"
    "that (i) the above copyright notices and this permission notice appear in\n"
    "all copies of the software and related documentation, and (ii) the names of\n"
    "Sam Leffler and Silicon Graphics may not be used in any advertising or\n"
    "publicity relating to the software without the specific, prior written\n"
    "permission of Sam Leffler and Silicon Graphics.\n"
    "\n"
    "THE SOFTWARE IS PROVIDED \"AS-IS\" AND WITHOUT WARRANTY OF ANY KIND,\n"
    "EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY\n"
    "WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.\n"
    "\n"
    "IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR\n"
    "ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,\n"
    "OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,\n"
    "WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF\n"
    "LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE\n"
    "OF THIS SOFTWARE.";
    
    pspl_toolchain_provide_copyright("libtiff (TIFF decoding, PackBits decompression algorithm for PSD decoding)",
                                     "Copyright (c) 1988-1997 Sam Leffler\n"
                                     "Copyright (c) 1991-1997 Silicon Graphics, Inc.",
                                     tiff_licence);
    
}

static void PP_hook(const pspl_toolchain_context_t* driver_context,
                    const char* directive_name,
                    unsigned int directive_argc,
                    const char** directive_argv) {
    
    if (!strcasecmp(directive_name, "SAMPLE"))
        sample_direc(driver_context, directive_argc, directive_argv);
    
}

static int init_hook(const pspl_toolchain_context_t* driver_context) {
    pspl_malloc_context_init(&converted_names);
    return 0;
}

static void finish_hook(const pspl_toolchain_context_t* driver_context) {
    pspl_malloc_context_destroy(&converted_names);
}

/* Preprocessor directives */
static const char* claimed_pp_direc[] = {
    "SAMPLE",
    NULL};

pspl_toolchain_extension_t TextureManager_toolext = {
    .claimed_global_preprocessor_directives = claimed_pp_direc,
    .init_hook = init_hook,
    .finish_hook = finish_hook,
    .subext_hook = subext_hook,
    .copyright_hook = copyright_hook,
    .line_preprocessor_hook = PP_hook
};
