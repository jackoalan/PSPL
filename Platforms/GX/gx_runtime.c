//
//  gx_runtime.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <ogc/gx.h>
#include <PSPLExtension.h>

static void load_object(pspl_runtime_psplc_t* object) {
    pspl_data_object_t datao;
    pspl_runtime_get_embedded_data_object_from_integer(object, 0, &datao);
    object->native_shader.disp_list = datao.object_data;
    object->native_shader.disp_list_len = (u32)datao.object_len;
}

static void bind_object(pspl_runtime_psplc_t* object) {
    GX_CallDispList((void*)object->native_shader.disp_list,
                    object->native_shader.disp_list_len);
}

/* PSPL-IR routines */
static int cur_mtx = 0;
void pspl_ir_load_pos_mtx(pspl_matrix34_t* mtx) {
    GX_LoadPosMtxImm(mtx->matrix, GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);
}
void pspl_ir_load_norm_mtx(pspl_matrix34_t* mtx) {
    GX_LoadNrmMtxImm(mtx->matrix, GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);
}
void pspl_ir_load_uv_mtx(pspl_matrix34_t* mtx) {
    GX_LoadTexMtxImm(mtx->matrix, GX_TEXMTX0+cur_mtx, GX_MTX3x4);
    ++cur_mtx;
}
void pspl_ir_load_finish() {
    cur_mtx = 0;
}

pspl_runtime_platform_t GX_runplat = {
    .load_object_hook = load_object,
    .bind_object_hook = bind_object
};
