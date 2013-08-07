//
//  IRRuntime.c
//  PSPL
//
//  Created by Jack Andersen on 6/9/13.
//
//

#include <stdio.h>
#include <PSPLRuntime.h>
#include <PSPL/PSPL_IR.h>

static void bind_object(pspl_runtime_psplc_t* object) {
    pspl_calc_chain_bind(object);
}

pspl_runtime_extension_t PSPL_IR_runext = {
    .bind_object_hook = bind_object
};
