//
//  SMRuntime.c
//  PSPL
//
//  Created by Jack Andersen on 6/24/13.
//
//

#include <stdio.h>
#include <PSPLExtension.h>

static int init_hook(const pspl_extension_t* extension) {
    return 0;
}

pspl_runtime_extension_t SceneManager_runext = {
    .init_hook = init_hook
};
