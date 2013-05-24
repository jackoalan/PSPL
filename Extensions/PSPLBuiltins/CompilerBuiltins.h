//
//  CompilerBuiltins.h
//  PSPL
//
//  Created by Jack Andersen on 5/3/13.
//
//

#ifndef PSPL_CompilerBuiltins_h
#define PSPL_CompilerBuiltins_h

int BUILTINS_init(const pspl_toolchain_context_t* driver_context);

int BUILTINS_command_call_hook(const pspl_toolchain_context_t* driver_context,
                               const pspl_toolchain_heading_context_t* current_heading,
                               const char* command_name,
                               unsigned int command_argc,
                               const char** command_argv);

#endif
