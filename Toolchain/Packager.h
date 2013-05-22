//
//  Packager.h
//  PSPL
//
//  Created by Jack Andersen on 4/28/13.
//
//

#ifndef PSPL_Packager_h
#define PSPL_Packager_h
#ifdef PSPL_INTERNAL

#include "Driver.h"

/* Package Indexer context type */
typedef struct {
    
    // Using which extensions
    unsigned int ext_count;
    const pspl_extension_t** ext_array;
    
    // Using which platforms
    unsigned int plat_count;
    const pspl_platform_t** plat_array;
    
    // PSPLC indexers
    unsigned int indexer_count;
    unsigned int indexer_cap;
    pspl_indexer_context_t** indexer_array;
    
    // Embedded PSPLC File stubs
    unsigned int stubs_count;
    unsigned int stubs_cap;
    pspl_indexer_entry_t** stubs_array;
    
} pspl_packager_context_t;

/* Package indexer initialiser */
void pspl_packager_init(pspl_packager_context_t* ctx,
                        unsigned int max_extension_count,
                        unsigned int max_platform_count,
                        unsigned int psplc_count);

/* Augment with index */
void pspl_packager_indexer_augment(pspl_packager_context_t* ctx,
                                   pspl_indexer_context_t* indexer);

/* Write out to PSPLP file */
void pspl_packager_write_psplp(pspl_packager_context_t* ctx,
                               uint8_t psplp_endianness,
                               FILE* psplp_file_out);

#endif // PSPL_INTERNAL
#endif

