//
//  PMDLRuntimeProcessing.c
//  PSPL
//
//  Created by Jack Andersen on 7/22/13.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <PSPLExtension.h>

/* Expected draw format for this runtime build */
#if PSPL_RUNTIME_PLATFORM_GL2 || PSPL_RUNTIME_PLATFORM_D3D11
    static const char* DRAW_FMT = "_GEN";
#elif PSPL_RUNTIME_PLATFORM_GX
    static const char* DRAW_FMT = "__GX";
#endif


/* PAR Enum */
enum pmdl_par {
    PMDL_PAR0 = 0,
    PMDL_PAR1 = 1,
    PMDL_PAR2 = 2
};

/* PMDL Header */
typedef struct __attribute__ ((__packed__)) {
    
    char magic[4];
    char endianness[4];
    uint32_t pointer_size;
    char sub_type_prefix[3];
    char sub_type_num;
    char draw_format[4];
    float master_aabb[2][3];
    
    uint32_t collection_offset;
    uint32_t collection_count;
    uint32_t shader_table_offset;
    uint32_t bone_table_offset;
    
    uint32_t padding;
    
} pmdl_header;

/* PMDL Collection Header */
typedef struct __attribute__ ((__packed__)) {
    
    uint16_t uv_count;
    uint16_t bone_count;
    uint32_t vert_buf_off;
    uint32_t vert_buf_len;
    uint32_t elem_buf_off;
    uint32_t elem_buf_len;
    uint32_t draw_idx_off;
    
} pmdl_col_header;


#pragma mark PMDL Loading / Unloading

/* This routine will validate and load PMDL data into GPU */
int pmdl_init(pspl_runtime_arc_file_t* pmdl_file) {
    char hash[PSPL_HASH_STRING_LEN];
    
    // First, validate header members
    pmdl_header* header = pmdl_file->file_data;
    
    // Magic
    if (memcmp(header->magic, "PMDL", 4)) {
        pspl_hash_fmt(hash, &pmdl_file->hash);
        pspl_warn("Unable to init PMDL", "file `%s` has invalid magic; skipping", hash);
        return -1;
    }
    
    // Endianness
#   if __LITTLE_ENDIAN__
        char* endian_str = "_LIT";
#   elif __BIG_ENDIAN__
        char* endian_str = "_BIG";
#   endif
    if (memcmp(header->endianness, endian_str, 4)) {
        pspl_hash_fmt(hash, &pmdl_file->hash);
        pspl_warn("Unable to init PMDL", "file `%s` has invalid endianness; skipping", hash);
        return -1;
    }
    
    // Pointer size
    if (header->pointer_size < sizeof(void*)) {
        pspl_hash_fmt(hash, &pmdl_file->hash);
        pspl_warn("Unable to init PMDL", "file `%s` pointer size too small (%d < %lu); skipping",
                  hash, header->pointer_size, (unsigned long)sizeof(void*));
        return -1;
    }
    
    // Draw Format
    if (memcmp(header->draw_format, DRAW_FMT, 4)) {
        pspl_hash_fmt(hash, &pmdl_file->hash);
        pspl_warn("Unable to init PMDL", "file `%s` has `%.4s` draw format, expected `%.4s` format; skipping",
                  hash, header->draw_format, DRAW_FMT);
        return -1;
    }
    
    
    // Decode PAR sub-type
    if (memcmp(header->sub_type_prefix, "PAR", 3)) {
        pspl_hash_fmt(hash, &pmdl_file->hash);
        pspl_warn("Unable to init PMDL", "invalid PAR type specified in `%s`; skipping", hash);
        return -1;
    }
    enum pmdl_par sub_type;
    if (header->sub_type_num == '0')
        sub_type = PMDL_PAR0;
    else if (header->sub_type_num == '1')
        sub_type = PMDL_PAR1;
    else if (header->sub_type_num == '2')
        sub_type = PMDL_PAR2;
    else {
        pspl_hash_fmt(hash, &pmdl_file->hash);
        pspl_warn("Unable to init PMDL", "invalid PAR type specified in `%s`; skipping", hash);
        return -1;
    }
    
    
    // Init collections
    pmdl_col_header* collection_array = pmdl_file->file_data + header->collection_offset;
    int i;
    for (i=0 ; i< header->collection_count; ++i) {
        pmdl_col_header* col_head = &collection_array[i];
        
    }
    
    return 0;
    
}

/* This routine will unload data from GPU */
void pmdl_destroy(void* pmdl_data) {
    
}


#pragma mark PMDL Drawing
