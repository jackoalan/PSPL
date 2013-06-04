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
        pspl_error(-1, "Only 8-bit-per-channel PSD files supported", "unable to use `%s`; PSB %d-bit files aren't supported",
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
        
        // Find named layer and read in
        
    } else
        fseek(psdf, lm_len, SEEK_CUR);
    
    
#   pragma mark Read Merged Image Data (if layer not specified)
    
    if (!file_path_ext) {
        
        uint16_t im_comp_mode = 0;
        if (!fread(&im_comp_mode, 1, sizeof(uint16_t), psdf))
            pspl_error(-1, "No merged image data in PSD", "PSD `%s` was not saved with 'Maximise Compatibility' checked",
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
    
    return 0;
    
}

