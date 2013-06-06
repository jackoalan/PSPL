//
//  PreprocessorBuiltins.h
//  PSPL
//
//  Created by Jack Andersen on 5/3/13.
//
//

#ifndef PSPL_PreprocessorBuiltins_h
#define PSPL_PreprocessorBuiltins_h

void BUILTINS_pp_hook(const pspl_toolchain_context_t* driver_context,
                      const char* directive_name,
                      unsigned int directive_argc,
                      const char** directive_argv);

#endif
