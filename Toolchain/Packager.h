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

/* Main entry point for packaging an array of compiled PSPL sources
 * (whose structure contains the necessary object buffers) */
void _pspl_run_packager(pspl_toolchain_driver_source_t* sources,
                        pspl_toolchain_driver_opts_t* driver_opts);

#endif // PSPL_INTERNAL
#endif
