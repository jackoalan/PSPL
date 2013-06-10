//
//  gl_toolchain.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <PSPLExtension.h>



static int init_hook(const pspl_toolchain_context_t* driver_context) {
    
}

static void copyright_hook() {
    
    pspl_toolchain_provide_copyright("OpenGL[ES] 2.0 Platform (GLSL Generator and Runtime Loader)",
                                     "Copyright (c) 2013 Jack Andersen <jackoalan@gmail.com>",
                                     "[See licence for \"PSPL\" above]");
    
}

static void receive_hook(const pspl_toolchain_context_t* driver_context,
                         const pspl_extension_t* source_ext,
                         const char* operation,
                         const void* data) {
    
}

static void generate_hook(const pspl_toolchain_context_t* driver_context) {
    
}

/* Toolchain platform definition */
pspl_toolchain_platform_t GL2_toolplat = {
    .init_hook = init_hook,
    .copyright_hook = copyright_hook,
    .receive_hook = receive_hook,
    .generate_hook = generate_hook
};
