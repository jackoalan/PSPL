//
//  Packager.c
//  PSPL
//
//  Created by Jack Andersen on 4/28/13.
//
//

#define PSPL_INTERNAL
#define PSPL_TOOLCHAIN

#include <PSPL/PSPLExtension.h>
#include <PSPLInternal.h>

#include "Packager.h"


#pragma mark Packager Implementation

/* Write out to PSPLP file */
void pspl_packager_write_psplp(pspl_indexer_context_t* ctx,
                               FILE* psplp_file_out) {
    
    // Determine endianness
    unsigned int psplp_endianness = 0;
    int i;
    for (i=0 ; i<ctx->plat_count ; ++i)
        psplp_endianness |= ctx->plat_array[i]->byte_order;
    
    
    
}