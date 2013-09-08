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
 *
 *   Copyright (C) 2004 - 2009
 *    Michael Wiedenbauer (shagkur)
 *    Dave Murphy (WinterMute)
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any
 *  damages arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any
 *  purpose, including commercial applications, and to alter it and
 *  redistribute it freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you
 *     must not claim that you wrote the original software. If you use
 *     this software in a product, an acknowledgment in the product
 *     documentation would be appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and
 *     must not be misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source
 *     distribution.
 */

/* GX requires that texels be arranged into localised tiles (not scanlines) */
int RGBAGX_encode(pspl_malloc_context_t* mem_ctx, const uint8_t* image_in, unsigned chan_count,
                  unsigned width, unsigned height, uint8_t** image_out, size_t* size_out) {
    
    uint8_t* bits = (uint8_t*)image_in;
    size_t alloc_sz = 4 * width * height;
    if (alloc_sz < 64)
        alloc_sz = 64;
    
    /* Perform tiling for GX */
    void* tiled_data = pspl_malloc(mem_ctx, alloc_sz);
    void* td_cur = tiled_data;
    
    int x,y,ix,iy;
    uint16_t color;
    
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
    
    *size_out = 4 * width * height;
    *image_out = (uint8_t*)tiled_data;
    return 0;
    
}
