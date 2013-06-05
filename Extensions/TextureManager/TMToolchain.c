//
//  TMToolchain.c
//  PSPL
//
//  Created by Jack Andersen on 6/3/13.
//
//

#include <PSPLExtension.h>
#include "TMToolchain.h"

extern pspl_tm_decoder_t* pspl_tm_available_decoders[];
extern pspl_tm_encoder_t* pspl_tm_available_encoders[];

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

pspl_toolchain_extension_t TextureManager_toolext = {
    .subext_hook = subext_hook
};
