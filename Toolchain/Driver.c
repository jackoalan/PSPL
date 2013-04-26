//
//  Driver.c
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//
#define PSPL_INTERNAL
#define PSPL_TOOLCHAIN

#include <stdio.h>
#include <string.h>
#include <PSPL/PSPL.h>
#include <PSPL/PSPLExtension.h>
#include "../Extensions/GXSupport/gx_offline.h"

#include <PSPLInternal.h>

int main(int argc, char** argv) {
    
    pspl_gx_offline_begin_transaction();
    GX_SetNumTevStages(2);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
    GX_SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP1, GX_COLORNULL);
    GX_SetTevOp(GX_TEVSTAGE1, GX_BLEND);
    GX_SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GX_Flush();
    void* buf;
    size_t buf_sz = pspl_gx_offline_end_transaction(&buf);
    FILE* outf = fopen("output.psplc", "w");
    fwrite(buf, 1, buf_sz, outf);
    fclose(outf);
    printf("Done %lu\n", buf_sz);
    return 0;
}