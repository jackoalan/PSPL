//
//  gx_runtime.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <malloc.h>
#include <ogc/cache.h>
#include <ogc/gx.h>
#include <PSPLExtension.h>

static void load_object(pspl_runtime_psplc_t* object) {
    
    // GX display lists *must* be 32-byte aligned
    // Therefore, this copies the list into an aligned region
    // and forces the cache to store into physical memory
    pspl_data_object_t datao;
    pspl_runtime_get_embedded_data_object_from_integer(object, 0, &datao);
    object->native_shader.disp_list = memalign(32, datao.object_len);
    memcpy(object->native_shader.disp_list, datao.object_data, datao.object_len);
    object->native_shader.disp_list_len = (u32)datao.object_len;
    DCStoreRange(object->native_shader.disp_list, (u32)datao.object_len);
    
}

static void unload_object(pspl_runtime_psplc_t* object) {
    free(object->native_shader.disp_list);
}

static void print_bytes(void* ptr, size_t size) {
    
    int i;
    
    for (i=0 ; i<size ; ++i) {
        
        if (!(i%32))
            
            printf("\n");
        
        printf("%02X", *(uint8_t*)(ptr+i));
        
    }
    
    printf("\n\n");
    
}

static void bind_object(pspl_runtime_psplc_t* object) {
    
    /*
    printf("About to run:\n");
    sleep(2);
    print_bytes((void*)object->native_shader.disp_list, object->native_shader.disp_list_len);
    sleep(10);
     */
    //GX_SetTev
    
    // This will asynchronously instruct the GPU to run the list
    //GX_CallDispList((void*)object->native_shader.disp_list,
    //                object->native_shader.disp_list_len);
    
}

/* PSPL-IR routines */
static int cur_mtx = 0;
void pspl_ir_load_pos_mtx(pspl_matrix34_t* mtx) {
    GX_LoadPosMtxImm(mtx[0], GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);
}
void pspl_ir_load_norm_mtx(pspl_matrix34_t* mtx) {
    GX_LoadNrmMtxImm(mtx[0], GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);
}
void pspl_ir_load_uv_mtx(pspl_matrix34_t* mtx) {
    GX_LoadTexMtxImm(mtx[0], GX_TEXMTX0+cur_mtx, GX_MTX3x4);
    ++cur_mtx;
}
void pspl_ir_load_finish() {
    cur_mtx = 0;
}

pspl_runtime_platform_t GX_runplat = {
    .load_object_hook = load_object,
    .unload_object_hook = unload_object,
    .bind_object_hook = bind_object
};
