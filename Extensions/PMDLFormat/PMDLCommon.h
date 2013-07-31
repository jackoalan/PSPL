//
//  PMDLCommon.h
//  PSPL
//
//  Created by Jack Andersen on 7/3/13.
//
//

#ifndef PSPL_PMDLCommon_h
#define PSPL_PMDLCommon_h

/* Pointer decl */
#if PSPL_TOOLCHAIN
#define P_DECL char ptr_buf[8]
#elif PSPL_RUNTIME
#define P_DECL union {const pspl_runtime_arc_file_t* file_ptr; char ptr_buf[8];}
#endif

/* Bi-uint32 type */
typedef DEF_BI_OBJ_TYPE(struct {
    uint32_t integer;
}) pmdl_bi_integer;

/* PMDL Reference Entry */
typedef struct {
    pspl_hash name_hash, pmdl_file_hash;
    P_DECL;
} pmdl_ref_entry;

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

#endif
