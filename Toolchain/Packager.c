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

/* Main entry point for packaging an array of compiled PSPL sources 
 * (whose structure contains the necessary object buffers) */
void _pspl_run_packager(pspl_toolchain_driver_source_t* sources,
                        pspl_toolchain_driver_opts_t* driver_opts,
                        const void** psplp_data_out,
                        size_t* psplp_data_len_out) {
    
}


