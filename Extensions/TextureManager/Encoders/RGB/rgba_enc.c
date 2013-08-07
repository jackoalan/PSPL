//
//  rgba_enc.c
//  PSPL
//
//  Created by Jack Andersen on 6/6/13.
//
//

#include <TMToolchain.h>
#include <PSPL/PSPLValue.h>

/* General Platforms - EASY! */
int RGBA_encode(pspl_malloc_context_t* mem_ctx, const uint8_t* image_in, unsigned chan_count,
                unsigned width, unsigned height, uint8_t** image_out, size_t* size_out) {
    *size_out = chan_count * width * height;
    *image_out = (uint8_t*)image_in;
    return 0;
}

#define FI_RGBA_RED                     0
#define FI_RGBA_GREEN                   1
#define FI_RGBA_BLUE                    2
#define FI_RGBA_ALPHA                   3

#define _SHIFTL(v, s, w)	\
((u32) (((u32)(v) & ((0x01 << (w)) - 1)) << (s)))
#define _SHIFTR(v, s, w)	\
((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))

typedef uint32_t u32;

/* Swapping loops below are derived libogc's zlib-licenced `gxtexconv`
 * http://sourceforge.net/p/devkitpro/libogc/ci/master/tree/libogc_license.txt */

/* GX requires that texels be arranged into localised tiles (not scanlines) */
int RGBAGX_encode(pspl_malloc_context_t* mem_ctx, const uint8_t* image_in, unsigned chan_count,
                  unsigned width, unsigned height, uint8_t** image_out, size_t* size_out) {
    
    /* Perform tiling for GX */
    void* tiled_data = pspl_malloc_malloc(mem_ctx, chan_count * width * height);
    void* td_cur = tiled_data;
    
    int x,y,ix,iy;
    uint16_t color;
    
    const uint8_t* bits = image_in;
    for(y=0;y<height;y+=4) {
        for(x=0;x<width;x+=4) {
            for(iy=0;iy<4;++iy) {
                for(ix=0;ix<4;++ix) {
                    color = _SHIFTL(bits[(((y+iy)*(width<<2))+((x+ix)<<2))+FI_RGBA_ALPHA],8,8)|(bits[(((y+iy)*(width<<2))+((x+ix)<<2))+FI_RGBA_RED]&0xff);
#                   if __LITTLE_ENDIAN__
                    color = swap_uint16(color);
#                   endif
                    *(uint16_t*)td_cur = color;
                    td_cur += 2;                    
                }
            }
            for(iy=0;iy<4;++iy) {
                for(ix=0;ix<4;++ix) {
                    color = _SHIFTL(bits[(((y+iy)*(width<<2))+((x+ix)<<2))+FI_RGBA_GREEN],8,8)|(bits[(((y+iy)*(width<<2))+((x+ix)<<2))+FI_RGBA_BLUE]&0xff);
#                   if __LITTLE_ENDIAN__
                    color = swap_uint16(color);
#                   endif
                    *(uint16_t*)td_cur = color;
                    td_cur += 2;
                }
            }
        }
    }
    
    *size_out = chan_count * width * height;
    *image_out = (uint8_t*)tiled_data;
    return 0;
    
}
