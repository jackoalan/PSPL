//
//  rgb_enc.c
//  PSPL
//
//  Created by Jack Andersen on 6/6/13.
//
//

#include <TMToolchain.h>

/* EASY! */
int RGB_encode(pspl_malloc_context_t* mem_ctx, const uint8_t* image_in, unsigned chan_count,
               unsigned width, unsigned height, uint8_t** image_out, size_t* size_out) {
    *size_out = chan_count * width * height;
    *image_out = (uint8_t*)image_in;
    return 0;
}
