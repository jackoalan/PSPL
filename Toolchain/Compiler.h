//
//  Compiler.h
//  PSPL
//
//  Created by Jack Andersen on 4/28/13.
//
//

#ifndef PSPL_Compiler_h
#define PSPL_Compiler_h
#ifdef PSPL_INTERNAL

#include "Driver.h"

/* Main entry point for compiling a single PSPL source */
void _pspl_run_compiler(pspl_toolchain_driver_source_t* source,
                        pspl_toolchain_driver_opts_t* driver_opts);

#endif // PSPL_INTERNAL
#endif

