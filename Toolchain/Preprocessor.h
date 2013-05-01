//
//  Preprocessor.h
//  PSPL
//
//  Created by Jack Andersen on 4/28/13.
//
//

#ifndef PSPL_Preprocessor_h
#define PSPL_Preprocessor_h
#ifdef PSPL_INTERNAL

#include "Driver.h"

/* Main entry point for preprocessing a single PSPL source */
void _pspl_run_preprocessor(pspl_toolchain_driver_source_t* source,
                            pspl_toolchain_context_t* ext_driver_ctx,
                            pspl_toolchain_driver_opts_t* driver_opts);

#endif // PSPL_INTERNAL
#endif

