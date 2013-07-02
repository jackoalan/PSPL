//
//  PMDLToolchain.c
//  PSPL
//
//  Created by Jack Andersen on 7/1/13.
//
//

#include <stdio.h>
#include <PSPLExtension.h>
#include "PMDLBlenderInterface.h"

static void copyright_hook() {
    
    pspl_toolchain_provide_copyright("PMDL (PSPL-native 3D model format)",
                                     "Copyright (c) 2013 Jack Andersen <jackoalan@gmail.com>",
                                     "[See licence for \"PSPL\" above]");
    
}

static void command_call_hook(const pspl_toolchain_context_t* driver_context,
                              const pspl_toolchain_heading_context_t* current_heading,
                              const char* command_name,
                              unsigned int command_argc,
                              const char** command_argv) {
    
    if (!strcasecmp(command_name, "ADD_BLENDER_OBJECT")) {
        
        pspl_pmdl_blender_connection blender;
        
        
    }
    
}

static const char* claimed_global_command_names[] = {
    "ADD_BLENDER_OBJECT",
    NULL};

pspl_toolchain_extension_t PMDL_toolext = {
    .copyright_hook = copyright_hook,
    .claimed_global_command_names = claimed_global_command_names,
    .command_call_hook = command_call_hook
};
