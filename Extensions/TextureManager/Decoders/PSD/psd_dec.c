//
//  psd_dec.c
//  PSPL
//
//  Created by Jack Andersen on 6/3/13.
//
//

#include <errno.h>
#include <stdio.h>
#include <TMToolchain.h>
#include <PSPLExtension.h>

/* Colour mode enumeration */
enum PSD_COLOUR_MODE {
    PSD_COLOUR_BITMAP       = 0,
    PSD_COLOUR_GREYSCALE    = 1,
    PSD_COLOUR_INDEXED      = 2,
    PSD_COLOUR_RGB          = 3,
    PSD_COLOUR_CMYK         = 4,
    PSD_COLOUR_MULTICHANNEL = 7,
    PSD_COLOUR_DUOTONE      = 8,
    PSD_COLOUR_LAB          = 9
};

/* PSD Header Type */
typedef struct {
    uint8_t signature[4];
    uint16_t version;
    uint16_t pad1;
    uint32_t pad2;
    uint16_t num_channels;
    uint32_t height, width;
    uint16_t depth;
    uint16_t colour_mode;
} PSD_head_t;

/* Rectangle type */
typedef struct {
    uint32_t top, left, bottom, right;
} PSD_rect_t;

/* Layer-Channel type */
typedef struct {
    uint16_t chan_id;
    uint32_t chan_len;
} PSD_layer_channel;

/* Pascal string advance utility */
static void advance_past_pascal_string(void** ptr) {
    uint8_t** cptr = (uint8_t**)ptr;
    if (!(**cptr))
        *cptr += 2;
    else
        *cptr += **cptr + 1;
}

/* Routine to extract named layer from Photoshop document */
int PSD_decode(const char* file_path, const char* file_path_ext,
               pspl_tm_image_t* image_out) {
    int x,y,c;
    
    // Open file
    FILE* psdf = fopen(file_path, "r");
    if (!psdf)
        pspl_error(-1, "Unable to open PSD", "error opening PSD `%s` - errno: %d (%s)",
                   file_path, errno, strerror(errno));
    
    
#   pragma mark Read Header
    
    // Read header
    PSD_head_t psd_head;
    if(fread(&psd_head, 1, sizeof(PSD_head_t), psdf) != sizeof(PSD_head_t))
        pspl_error(-1, "Unable to read PSD header", "error reading header of PSD `%s`",
                   file_path);
    
    // Validate header
    if (memcmp(psd_head.signature, "8BPS", 4))
        pspl_error(-1, "Invalid PSD signature", "signature of PSD `%s` needs to be '8BPS'",
                   file_path);
    
    // Swap for little-endian platforms
#   ifdef __LITTLE_ENDIAN__
    psd_head.version = swap_uint16(psd_head.version);
    psd_head.num_channels = swap_uint16(psd_head.num_channels);
    psd_head.height = swap_uint32(psd_head.height);
    psd_head.width = swap_uint32(psd_head.width);
    psd_head.depth = swap_uint16(psd_head.depth);
    psd_head.colour_mode = swap_uint16(psd_head.colour_mode);
#   endif
    
    // Validate version
    if (psd_head.version != 1)
        pspl_error(-1, "Only PSD version 1 files supported", "unable to use `%s`; PSB 'BIG' files aren't supported",
                   file_path);
    
    // Validate depth
    if (psd_head.depth != 8)
        pspl_error(-1, "Only 8-bit-per-channel PSD files supported", "unable to use `%s`; PSD %d-bit files aren't supported",
                   file_path, psd_head.depth);
    
    // Validate colour mode
    if (psd_head.colour_mode != PSD_COLOUR_RGB)
        pspl_error(-1, "Only RGB PSD files supported", "unable to use `%s`",
                   file_path);

    
    
#   pragma mark Read Colour Mode Data
    
    uint32_t cm_len = 0;
    fread(&cm_len, 1, sizeof(uint32_t), psdf);
#   ifdef __LITTLE_ENDIAN__
    cm_len = swap_uint32(cm_len);
#   endif
    
    fseek(psdf, cm_len, SEEK_CUR);
    
    
#   pragma mark Read Image Resources Data
    
    uint32_t ir_len = 0;
    fread(&ir_len, 1, sizeof(uint32_t), psdf);
#   ifdef __LITTLE_ENDIAN__
    ir_len = swap_uint32(ir_len);
#   endif
    
    fseek(psdf, ir_len, SEEK_CUR);
    
    
#   pragma mark Read Layer and Mask Data
    
    uint32_t lm_len = 0;
    fread(&lm_len, 1, sizeof(uint32_t), psdf);
#   ifdef __LITTLE_ENDIAN__
    lm_len = swap_uint32(lm_len);
#   endif
    
    if (file_path_ext) {
        if (!lm_len)
            pspl_error(-1, "No layers in PSD", "requested layer '%s' from PSD `%s` which has no layers",
                       file_path_ext, file_path);
        
        // Find named layer and read in
        
        int16_t layer_count;
        fread(&layer_count, 1, sizeof(int16_t), psdf);
#       ifdef __LITTLE_ENDIAN__
        layer_count = swap_uint16(layer_count);
#       endif
        
        // If we found proposed layer
        uint8_t found_layer = 0;
        
        // Rectangle of proposed layer
        PSD_rect_t layer_rect;
        
        // Channel data of proposed layer
        uint16_t layer_chan_count;
        PSD_layer_channel layer_chan_arr[4];
        
        // Offset of proposed layer's image data
        size_t layer_chan_data_off = 0;
        
        // Navigate layer info data
        int i;
        for (i=0 ; i<layer_count ; ++i) {
            size_t next_layer_chan_data_off = 0;
            
            // Layer rectangle
            if (found_layer) {
                fseek(psdf, sizeof(PSD_rect_t), SEEK_CUR);
            } else {
                fread(&layer_rect, 1, sizeof(PSD_rect_t), psdf);
#               ifdef __LITTLE_ENDIAN__
                layer_rect.bottom = swap_uint32(layer_rect.bottom);
                layer_rect.left = swap_uint32(layer_rect.left);
                layer_rect.top = swap_uint32(layer_rect.top);
                layer_rect.right = swap_uint32(layer_rect.right);
#               endif
            }
            
            // Advance past channels
            uint16_t chan_count;
            fread(&chan_count, 1, sizeof(uint16_t), psdf);
#           ifdef __LITTLE_ENDIAN__
            chan_count = swap_uint16(chan_count);
#           endif
            if (!found_layer)
                layer_chan_count = (chan_count > 4)?4:chan_count;
            int j;
            for (j=0 ; j<chan_count ; ++j) {
                if (found_layer || j>3) {
                    fseek(psdf, sizeof(PSD_layer_channel), SEEK_CUR);
                } else {
                    PSD_layer_channel* chan = &layer_chan_arr[j];
                    fread(chan, 1, sizeof(PSD_layer_channel), psdf);
#                   ifdef __LITTLE_ENDIAN__
                    chan->chan_id = swap_uint16(chan->chan_id);
                    chan->chan_len = swap_uint32(chan->chan_len);
#                   endif
                    next_layer_chan_data_off += chan->chan_len;
                }
            }
            
            // Ensure '8BIM' occurs here
            char czech[4];
            fread(czech, 1, 4, psdf);
            if (memcmp(czech, "8BIM", 4))
                pspl_error(-1, "Inconsistent PSD detected", "unable to find expected blend-mode signature in `%s`",
                           file_path);
            
            // Advance to layer name
            fseek(psdf, 8, SEEK_CUR);
            uint32_t extra_len;
            fread(&extra_len, 1, sizeof(uint32_t), psdf);
#           ifdef __LITTLE_ENDIAN__
            extra_len = swap_uint32(extra_len);
#           endif
            fseek(psdf, extra_len, SEEK_CUR);
            
            // This should be the name
            uint8_t name_len;
            fread(&name_len, 1, sizeof(uint8_t), psdf);
            char name[256];
            fread(name, 1, name_len, psdf);
            name[name_len] = '\0';
            
            // Check against requested name
            if (!strcasecmp(name, file_path_ext))
                found_layer = 1;
            
            // Accumulate layer data offset
            if (!found_layer)
                layer_chan_data_off += next_layer_chan_data_off;
            
        }
        
        // Ensure layer was found
        if (!found_layer)
            pspl_error(-1, "Unable to find PSD layer", "PSD `%s` does not contain a layer named '%s'",
                       file_path, file_path_ext);
        
        // Read in layer image data
        fseek(psdf, layer_chan_data_off, SEEK_CUR);
        uint16_t im_comp_mode = 0;
        fread(&im_comp_mode, 1, sizeof(uint16_t), psdf);
#       ifdef __LITTLE_ENDIAN__
        im_comp_mode = swap_uint16(im_comp_mode);
#       endif
        if (im_comp_mode != 0)
            pspl_error(-1, "Compressed PSD formats unsupported", "PSD `%s` has compressed image data",
                       file_path);
        size_t im_len = 0;
        for (i=0 ; i<layer_chan_count ; ++i) {
            PSD_layer_channel* chan = &layer_chan_arr[i];
            im_len += chan->chan_len;
        }
        void* im_data = malloc(im_len);
        if (fread(im_data, 1, im_len, psdf) != im_len)
            pspl_error(-1, "Unexpected end of PSD", "unable to read layer '%s' image data from `%s`",
                       file_path_ext, file_path);
        
        // Rearrange data in scanline, channel-interleaved order
        uint8_t* im_dest = malloc(psd_head.width * psd_head.height * layer_chan_count);
        const uint8_t* im_cur_chan = im_data;
        for (c=0 ; c<layer_chan_count ; ++c) {
            for (y=0 ; y<psd_head.height ; ++y) {
                const uint8_t* im_cur_line = im_cur_chan + ((y - layer_rect.top) * (layer_rect.right - layer_rect.left));
                uint8_t line_limit = (layer_rect.top > y || layer_rect.bottom < y);
                unsigned line_base = psd_head.width * y * layer_chan_count;
                for (x=0 ; x<psd_head.width ; ++x) {
                    uint8_t limit = (line_limit)?1:(layer_rect.left > x || layer_rect.right < x);
                    unsigned col = x * layer_chan_count;
                    im_dest[line_base+col+c] = (limit)?0:*(im_cur_line + (x - layer_rect.left));
                }
            }
            im_cur_chan += layer_chan_arr[c].chan_len;
        }
        
        // Done with read data
        free(im_data);
        
        // Populate TM structure
        image_out->image_type = layer_chan_count;
        image_out->height = psd_head.height;
        image_out->width = psd_head.width;
        image_out->image_buffer = im_dest;
        image_out->index_buffer = NULL;
        
        fclose(psdf);
        return 0;
        
    }
    
    // Skip to next section otherwise
    fseek(psdf, lm_len, SEEK_CUR);
    
    
#   pragma mark Read Merged Image Data (if layer not specified)
    
    if (!file_path_ext) {
        
        uint16_t im_comp_mode = 0;
        if (!fread(&im_comp_mode, 1, sizeof(uint16_t), psdf))
            pspl_error(-1, "No merged image data in PSD", "PSD `%s` was not saved with 'Maximise Compatibility' checked",
                       file_path);
#       ifdef __LITTLE_ENDIAN__
        im_comp_mode = swap_uint16(im_comp_mode);
#       endif
        if (im_comp_mode != 0)
            pspl_error(-1, "Compressed PSD formats unsupported", "PSD `%s` has compressed merged image data",
                       file_path);
        size_t im_len = psd_head.width * psd_head.height * psd_head.num_channels;
        void* im_data = malloc(im_len);
        if (fread(im_data, 1, im_len, psdf) != im_len)
            pspl_error(-1, "Unexpected end of PSD", "unable to read all image data from `%s`",
                       file_path);
        
        // Rearrange merged data in scanline, channel-interleaved order
        const uint8_t* im_cur = im_data;
        unsigned out_num_channels = (psd_head.num_channels > 4)?4:psd_head.num_channels;
        uint8_t* im_dest = malloc(psd_head.width * psd_head.height * out_num_channels);
        for (c=0 ; c<out_num_channels ; ++c) {
            for (y=0 ; y<psd_head.height ; ++y) {
                unsigned line_base = psd_head.width * y * out_num_channels;
                for (x=0 ; x<psd_head.width ; ++x) {
                    unsigned col = x * out_num_channels;
                    im_dest[line_base+col+c] = *(im_cur++);
                }
            }
        }
        
        // Done with read data
        free(im_data);
        
        // Populate TM structure
        image_out->image_type = out_num_channels;
        image_out->height = psd_head.height;
        image_out->width = psd_head.width;
        image_out->image_buffer = im_dest;
        image_out->index_buffer = NULL;
        
    }
    
    fclose(psdf);
    return 0;
    
}

