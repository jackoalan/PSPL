//
//  PMDLRuntime.h
//  PSPL
//
//  Created by Jack Andersen on 7/23/13.
//
//

#ifndef PSPL_PMDLRuntime_h
#define PSPL_PMDLRuntime_h

/* OpenGL ES on iOS doesn't support 3x4 matrix uniforms */
#define MTX_TYPE pspl_matrix34_t
#if __APPLE__
#   include <TargetConditionals.h>
#   if TARGET_OS_IPHONE
#       undef MTX_TYPE
#       define MTX_TYPE pspl_matrix44_t
#   endif
#endif

#include <PSPL/PSPLCommon.h>
#include <PSPLRuntime.h>

/* NOTE - This entire API is NOT THREAD SAFE!!! */

/* Projection type enumeration */
enum pmdl_projection_type {
    PMDL_PERSPECTIVE = 0,
    PMDL_ORTHOGRAPHIC = 1
};

/* Frustum plane-index enumeration */
enum pmdl_plane_indices {
    PTOP    = 0,
    PBOTTOM = 1,
    PLEFT   = 2,
    PRIGHT  = 3,
    PNEAR   = 4,
    PFAR    = 5
};

/* Plane representation */
typedef struct {
    pspl_vector4_t ABCD;
} pmdl_plane_t;

/* This type allows the PMDL-using application to establish
 * a drawing context (containing master transform info)
 * to use against an initialised PMDL model */
typedef struct {
    
    /* Model transform */
    pspl_matrix34_t model_mtx;
    
    /* Camera view */
    pspl_camera_view_t camera_view;
    
    /* Projection transform */
    enum pmdl_projection_type projection_type; // Enumerated projection type
    union {
        pspl_perspective_t perspective;
        pspl_orthographic_t orthographic;
    } projection;
    
    /* Texture Coordinate Matrices */
    MTX_TYPE texcoord_mtx[8];
    
    /* DO NOT EDIT FIELDS BELOW!! - Automatically set by update routine */
    
    /* View matrix */
    pspl_matrix34_t cached_view_mtx;
    
    /* Modelview matrix */
    MTX_TYPE cached_modelview_mtx;
    MTX_TYPE cached_modelview_invxpose_mtx;
    
    /* Cached frustum tangents */
    float f_tanv, f_tanh;
    
    /* Projection matrix */
    pspl_matrix44_t cached_projection_mtx;
    
} pmdl_draw_context_t;

/* Invalidate context transformation cache (if values updated) */
enum pmdl_invalidate_bits {
    PMDL_INVALIDATE_NONE        = 0,
    PMDL_INVALIDATE_MODEL       = 1,
    PMDL_INVALIDATE_VIEW        = (1<<1),
    PMDL_INVALIDATE_PROJECTION  = (1<<2),
    PMDL_INVALIDATE_ALL         = 0xff
};
void pmdl_update_context(pmdl_draw_context_t* ctx, enum pmdl_invalidate_bits inv_bits);

/* Master draw routine */
void pmdl_draw(pmdl_draw_context_t* ctx, pspl_runtime_arc_file_t* pmdl_file);

#endif
