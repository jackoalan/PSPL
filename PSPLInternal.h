//
//  PSPLInternal.h
//  PSPL
//
//  Created by Jack Andersen on 4/25/13.
//
//

#ifndef PSPL_PSPLInternal_h
#define PSPL_PSPLInternal_h

/* This file is only to be included in core PSPL runtime and toolchain
 * codebases. It is primarily concerned with the PSPLC and PSPLP file-formats */
#ifdef PSPL_INTERNAL

#include <PSPL/PSPL.h>

/* Internal enumerations */
extern const char* PSPL_MAGIC; // Set to PSPL_MAGIC_DEF
#define PSPL_MAGIC_DEF "PSPL"
#define PSPL_PSPLC 0
#define PSPL_PSPLP 1
#define PSPL_VERSION 1


#pragma mark Common Types

/* Byte-order-independent structure defining the PSPLC/PSPLP header */
typedef struct {
    uint8_t magic[4]; // "PSPL"
    uint8_t package_flag; // 0: PSPLC, 1: PSPLP
    uint8_t version;
    uint8_t endian_flags; // 1: Little, 2: Big, 3: Bi
    uint8_t padding;
        
    // Count and offset of extension name table
    DEF_BI_OBJ_TYPE(uint32_t) extension_name_table_c;
    DEF_BI_OBJ_TYPE(uint32_t) extension_name_table_off;
    
    // Count and offset of platform name table
    DEF_BI_OBJ_TYPE(uint32_t) platform_name_table_c;
    DEF_BI_OBJ_TYPE(uint32_t) platform_name_table_off;
    
} pspl_header_t;


#pragma mark PSPLC Types

/* PSPLC compiled object record 
 * (every PSPLC file has exactly one and PSPLP files may have many) */
typedef struct {
    
    // Hash of this PSPLC object (truncated 32-bit SHA1 of object name)
    // This actually occurs immediately before this header (not included in
    // type since it's endian-independent)
    //pspl_hash psplc_object_hash;
    
    // Tiered object indexing structure:
    //  * Per-extension object array count
    //  * Per-extension object array (marked by absolute offset for each byte order)
    //  * Total length of the sub-tables below this one (pointed to by previous member)
    //      * Extension index
    //      * Extension hash-indexed object array count
    //      * Extension integer-indexed object array count
    //      * Extension hash-indexed object array (relative to per-extension array start)
    //          * 32-bit Object hash
    //          * Platform availability bits (platform-dependent loading)
    //          * Absolute file offset of native byte-order object
    //          * Length of object
    //      * Extension integer-indexed object array (relative to per-extension array start)
    //          * 32-bit Object index
    //          * Platform availability bits (platform-dependent loading)
    //          * Absolute file offset of native byte-order object
    //          * Length of object
    
    // Count of extensions referenced by this psplc (count of elements in array below)
    uint32_t extension_count;
    
    // File-absolute offset to per-extension object subtables,
    // separate tables for each byte-order
    uint32_t extension_array_off;
    
    // Count of file object stubs
    uint32_t file_stub_count;
    
    // Offset to byte-order-specific file stub array
    uint32_t file_stub_array_off;
    
} pspl_psplc_header_t;
typedef DEF_BI_OBJ_TYPE(pspl_psplc_header_t) pspl_psplc_header_bi_t;


/* Tier 2 (per-extension) Extension object array table */
typedef struct {
    
    // Index of relevant extension
    // (as positioned in extension name table at bottom of PSPL)
    uint32_t tier2_extension_index;
    
    // Hash-indexed object count
    uint32_t tier2_hash_indexed_object_count;
    uint32_t tier2_hash_indexed_object_array_off;
    
    // Integer-indexed object count
    uint32_t tier2_int_indexed_object_count;
    uint32_t tier2_int_indexed_object_array_off;
    
    // Embedded object entry records (tier3) follow this one
    
} pspl_object_array_tier2_t;
typedef DEF_BI_OBJ_TYPE(pspl_object_array_tier2_t) pspl_object_array_tier2_bi_t;


/* Tier 3 (per-object) Extension object entry (stub) 
 * (actual resource not present). Used in PSPLC files to reference
 * hashes of files to be packaged. Also composed into concrete
 * object entry below. */
typedef struct {
    
    // Data hash of object file (before structure)
    //pspl_hash data_hash;
    
    // Platform availability bitfield
    // Bits indexed from LSB to MSB indicate whether or not this
    // object should be loaded into RAM given the current runtime host
    // (as indexed in the platform name table at bottom of PSPL)
    uint32_t platform_availability_bits;
    
    // Absolute file-offset to source file name this file stub
    // is derived from (used to perform internal dependency calculation
    // if the PSPLC is re-compiled over itself)
    uint32_t object_source_path_off;
    
    // In cases where the source file is provided with a tiered extension,
    // this will be non-zero; pointing to a string containing the extension
    // identifier
    uint32_t object_source_path_ext_off;
    
} pspl_object_stub_t;
typedef DEF_BI_OBJ_TYPE(pspl_object_stub_t) pspl_object_stub_bi_t;


/* Tier 3 (per-object) Extension object entry (concrete)
 * Used to index embedded objects and archived files. */
typedef struct {
    
    // Hash of object (just before structure)
    //pspl_hash object_hash;
    
    // Platform availability bitfield
    // Bits indexed from LSB to MSB indicate whether or not this
    // object should be loaded into RAM given the current runtime host
    // (as indexed in the platform name table at bottom of PSPL)
    uint32_t platform_availability_bits;
    
    // Absolute file-offset to embedded object
    uint32_t object_off;
    
    // Length of object
    uint32_t object_len;
    
} pspl_object_hash_record_t;
typedef DEF_BI_OBJ_TYPE(pspl_object_hash_record_t) pspl_object_hash_record_bi_t;
typedef struct {
    
    // Index of object
    uint32_t object_index;
    
    // Platform availability bitfield
    // Bits indexed from LSB to MSB indicate whether or not this
    // object should be loaded into RAM given the current runtime host
    // (as indexed in the platform name table at bottom of PSPL)
    uint32_t platform_availability_bits;
    
    // Absolute file-offset to embedded object
    uint32_t object_off;
    
    // Length of object
    uint32_t object_len;
    
} pspl_object_int_record_t;
typedef DEF_BI_OBJ_TYPE(pspl_object_int_record_t) pspl_object_int_record_bi_t;


#pragma mark PSPLP Types

/* Extended structure for PSPLP (occurs after `pspl_header_t`) */
typedef struct {
    
    // Count of packaged PSPLC records
    uint32_t psplc_array_count;
    
    // Absolute offset to byte-order native array of `pspl_psplc_header_t` objects
    //uint32_t psplc_array_off; // Occurs after this structure (therefore unnecessary)
    
    // Count of packaged files
    uint32_t packaged_file_array_count;
    
    // Absolute offset to byte-order native array of `pspl_object_record_t`
    // objects storing file hashes and absolute locations
    uint32_t packaged_file_array_off;
    
} pspl_psplp_header_t;
typedef DEF_BI_OBJ_TYPE(pspl_psplp_header_t) pspl_psplp_header_bi_t;


#endif // PSPL_INTERNAL
#endif
