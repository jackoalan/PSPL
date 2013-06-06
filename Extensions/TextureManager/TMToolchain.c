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

extern pspl_tm_decoder_t* pspl_tm_available_decoders[];
extern pspl_tm_encoder_t* pspl_tm_available_encoders[];

/* Malloc-context for tracking converted file-names */
static pspl_malloc_context_t converted_names;

/* SAMPLE preprocessor directive handling */
static void sample_direc(unsigned int argc, const char** argv) {
    if (!argc)
        pspl_error(-1, "Incomplete use of [SAMPLE] directive",
                   "must follow `SAMPLE <tex_file> [LAYER <layer_name>] UV <uv_key>` syntax");
    
    int i;
    
    // Name forms
    const char* name = argv[0];
    const char* name_ext = NULL;
    
    // UV Arg
    const char* uv = NULL;
    
    // Iterate remaining arguments
    for (i=1 ; i<argc ; ++i) {
        if (!strcasecmp(argv[i], "LAYER") && argc >= i) {
            name_ext = argv[i+1];
            ++i;
        } else if (!strcasecmp(argv[i], "UV") && argc >= i) {
            uv = argv[i+1];
            ++i;
        }
    }
    
    // We need UV
    if (!uv)
        pspl_error(-1, "Incomplete use of [SAMPLE] directive",
                   "missing required 'UV' argument in `SAMPLE <tex_file> [LAYER <layer_name>] UV <uv_key>` syntax");
    
    // Determine if file was already converted for this PSPLC (allows shared bindings)
    size_t name_len = strlen(name);
    unsigned tex_idx = converted_names.object_num;
    for (i=0 ; i<converted_names.object_num ; ++i) {
        const char* prev_name = converted_names.object_arr[i];
        if (!memcmp(name, prev_name, name_len)) {
            if (name_ext && strcmp(name_ext, prev_name+name_len))
                continue;
            tex_idx = i;
            break;
        }
    }
    
    char tex_idx_str[32];
    snprintf(tex_idx_str, 32, "%u", tex_idx);
    pspl_preprocessor_add_command_call("PSPL_SAMPLE_TEXTURE_2D", tex_idx_str, uv);
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

static void PP_hook(const pspl_toolchain_context_t* driver_context,
                    const char* directive_name,
                    unsigned int directive_argc,
                    const char** directive_argv) {
    
    if (!strcasecmp(directive_name, "SAMPLE"))
        sample_direc(directive_argc, directive_argv);
    
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
    .line_preprocessor_hook = PP_hook
};
