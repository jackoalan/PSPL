//
//  PMDLRuntime.h
//  PSPL
//
//  Created by Jack Andersen on 7/23/13.
//
//

#ifndef PSPL_PMDLRuntime_h
#define PSPL_PMDLRuntime_h

#ifdef __cplusplus
extern "C" {
#endif
    
#include <PSPLRuntime.h>
#include <PSPL/PSPLCommon.h>
#include <PMDLRuntimeRigging.h>
    
/* Pointer decl */
#define P_DECL(ptype, name) union {ptype* name; char name##_ptr_buf[8];}

/* Root PMDL object type */
typedef struct {
    P_DECL(pspl_runtime_arc_file_t, file_ptr);
    P_DECL(pmdl_rigging_ctx, rigging_ptr);
} pmdl_t;
    


/* NOTE - This entire API is NOT THREAD SAFE!!! */

#define PMDL_MAX_TEXCOORD_MATS 8
#define PMDL_MAX_BONES 8

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
    pspl_matrix44_t texcoord_mtx[PMDL_MAX_TEXCOORD_MATS];
    
    /* Default shader, if model doesn't specify one */
    const pspl_runtime_psplc_t* default_shader;
    
    /* DO NOT EDIT FIELDS BELOW!! - Automatically set by update routine */
    
    /* View matrix */
    pspl_matrix34_t cached_view_mtx;
    
    /* Modelview matrix */
    pspl_matrix44_t cached_modelview_mtx;
    pspl_matrix44_t cached_modelview_invxpose_mtx;
    
    /* Cached frustum tangents */
    float f_tanv, f_tanh;
    
    /* Projection matrix */
    pspl_matrix44_t cached_projection_mtx;
    
} pmdl_draw_ctx;

/* Routine to allocate and return a new draw context */
pmdl_draw_ctx* pmdl_new_draw_context();

/* Routine to free draw context */
void pmdl_free_draw_context(pmdl_draw_ctx* context);
    
/* Routine to allocate and return a (NULL-terminated) array of new draw contexts */
pmdl_draw_ctx* pmdl_new_draw_context_array(unsigned count);
    
/* Routine to free said (NULL-terminated) array */
void pmdl_free_draw_context_array(pmdl_draw_ctx* array);

/* Invalidate draw context transformation cache (if values updated) */
enum pmdl_invalidate_bits {
    PMDL_INVALIDATE_NONE        = 0,
    PMDL_INVALIDATE_MODEL       = 1,
    PMDL_INVALIDATE_VIEW        = (1<<1),
    PMDL_INVALIDATE_PROJECTION  = (1<<2),
    PMDL_INVALIDATE_ALL         = 0xff
};
void pmdl_update_context(pmdl_draw_ctx* ctx, enum pmdl_invalidate_bits inv_bits);

/* Lookup routine to get PMDL file reference from PSPLC */
const pmdl_t* pmdl_lookup(const pspl_runtime_psplc_t* pspl_object, const char* pmdl_name);
    
/* Lookup PMDL rigging action */
const pmdl_action* pmdl_action_lookup(const pmdl_t* pmdl, const char* action_name);

/* Master draw routine */
void pmdl_draw(pmdl_draw_ctx* ctx, const pmdl_t* pmdl_file);

/* Rigged master draw routine */
void pmdl_draw_rigged(const pmdl_draw_ctx* ctx, const pmdl_t* pmdl,
                      const pmdl_animation_ctx* anim_ctx);



#pragma mark Linear Argebra


#include <math.h>

/* For GX compatibility, this file is conditionally compiled to
 * link to libogc matrix routines or supply identical versions
 * for OpenGL/D3D */

#define REGISTER_KEY

#define DegToRad(a)   ( (a) *  0.01745329252f )

#if HW_RVL
#include <ogc/gu.h>
#define _pmdl_matrix_orthographic guOrtho
#define _pmdl_matrix_perspective guPerspective
#define _pmdl_matrix_lookat(mp,p,u,l) guLookAt((mp)->m,(guVector*)(p),(guVector*)(u),(guVector*)(l))
#define pmdl_matrix34_identity guMtxIdentity
#define pmdl_matrix34_cpy guMtxCopy
#define pmdl_matrix34_mul(a,b,ab) guMtxConcat((a)->m,(b)->m,(ab)->m)
#define pmdl_matrix34_invxpose guMtxInvXpose
#define pmdl_vector3_dot(a,b) guVecDotProduct((guVector*)(a),(guVector*)(b))
#define pmdl_vector3_cross(a,b,axb) guVecCross((guVector*)(a),(guVector*)(b),(guVector*)(axb))
#define pmdl_vector3_normalise(v) guVecNormalize((guVector*)(v))
#define pmdl_vector3_scale(src,dst,scale) guVecScale((guVector*)(src),(guVector*)(dst),(scale))
#define pmdl_vector3_add(a,b,ab) guVecAdd((guVector*)(a),(guVector*)(b),(guVector*)(ab))
#define pmdl_vector3_sub(a,b,ab) guVecSub((guVector*)(a),(guVector*)(b),(guVector*)(ab))
#define pmdl_vector3_matrix_mul(mp,s,d) guVecMultiply((mp)->m,(guVector*)(s),(guVector*)(d))


/* Extended libogc matrix routines */
void pmdl_matrix44_identity(REGISTER_KEY pspl_matrix44_t* mt);
    
void pspl_matrix44_cpy(REGISTER_KEY pspl_matrix44_t* src, REGISTER_KEY pspl_matrix44_t* dst);

void pmdl_matrix44_mul(REGISTER_KEY pspl_matrix44_t* a, REGISTER_KEY pspl_matrix44_t* b, REGISTER_KEY pspl_matrix44_t* ab);
void pmdl_matrix3444_mul(REGISTER_KEY pspl_matrix34_t* a, REGISTER_KEY pspl_matrix44_t* b, REGISTER_KEY pspl_matrix44_t* ab);

float pmdl_vector4_dot(REGISTER_KEY pspl_vector4_t* a, REGISTER_KEY pspl_vector4_t* b);

void pmdl_vector4_scale(REGISTER_KEY pspl_vector4_t* src, REGISTER_KEY pspl_vector4_t* dst, REGISTER_KEY float scale);

void pmdl_quat_mul(pspl_vector4_t* a, pspl_vector4_t* b, pspl_vector4_t* ab);
    
void pmdl_matrix34_quat(REGISTER_KEY pspl_matrix34_t* m, REGISTER_KEY pspl_vector4_t* a);
    
#define pmdl_vector4_cpy(s,d) (d) = (s)
#define pmdl_vector4_add(a,b,ab) (ab) = (a)+(b)
#define pmdl_vector4_sub(a,b,ab) (ab) = (a)-(b)


#else


void _pmdl_matrix_orthographic(pspl_matrix44_t* mt, float t, float b, float l,
                               float r, float n, float f);

void _pmdl_matrix_perspective(pspl_matrix44_t* mt, float fovy, float aspect, float n, float f);
void _pmdl_matrix_lookat(pspl_matrix34_t* mt, pspl_vector3_t* pos, pspl_vector3_t* up, pspl_vector3_t* look);

void pmdl_matrix34_identity(REGISTER_KEY pspl_matrix34_t* mt);
void pmdl_matrix44_identity(REGISTER_KEY pspl_matrix44_t* mt);

//void pmdl_matrix34_cpy(const REGISTER_KEY pspl_matrix34_t src, REGISTER_KEY pspl_matrix34_t dst);
//void pmdl_matrix44_cpy(REGISTER_KEY pspl_matrix44_t src, REGISTER_KEY pspl_matrix44_t dst);
#define pmdl_matrix34_cpy(s,d) (d)[0] = (s)[0]; (d)[1] = (s)[1]; (d)[2] = (s)[2]
#define pmdl_matrix44_cpy(s,d) (d)[0] = (s)[0]; (d)[1] = (s)[1]; (d)[2] = (s)[2]; (d)[3] = (s)[3]

void pmdl_matrix34_mul(REGISTER_KEY pspl_matrix34_t* a, REGISTER_KEY pspl_matrix34_t* b, REGISTER_KEY pspl_matrix34_t* ab);
void pmdl_matrix44_mul(REGISTER_KEY pspl_matrix44_t* a, REGISTER_KEY pspl_matrix44_t* b, REGISTER_KEY pspl_matrix44_t* ab);
void pmdl_matrix3444_mul(REGISTER_KEY pspl_matrix34_t* a, REGISTER_KEY pspl_matrix44_t* b, REGISTER_KEY pspl_matrix44_t* ab);

void pmdl_matrix34_invxpose(REGISTER_KEY pspl_matrix34_t* src, REGISTER_KEY pspl_matrix34_t* xpose);

/* Reference C implementations from libogc */
#define pmdl_vector3_cpy(s,d) (d) = (s)
#define pmdl_vector4_cpy(s,d) (d) = (s)
    
float pmdl_vector3_dot(REGISTER_KEY pspl_vector3_t* a, REGISTER_KEY pspl_vector3_t* b);
float pmdl_vector4_dot(REGISTER_KEY pspl_vector4_t* a, REGISTER_KEY pspl_vector4_t* b);

void pmdl_vector3_cross(REGISTER_KEY pspl_vector3_t* a, REGISTER_KEY pspl_vector3_t* b, REGISTER_KEY pspl_vector3_t* axb);

void pmdl_vector3_normalise(REGISTER_KEY pspl_vector3_t* v);

//void pmdl_vector3_scale(REGISTER_KEY pspl_vector3_t src, REGISTER_KEY pspl_vector3_t dst, REGISTER_KEY float scale);
//void pmdl_vector4_scale(REGISTER_KEY pspl_vector4_t src, REGISTER_KEY pspl_vector4_t dst, REGISTER_KEY float scale);
#define pmdl_vector3_scale(s,d,sc) (d) = (s)*(sc)
#define pmdl_vector4_scale(s,d,sc) (d) = (s)*(sc)

//void pmdl_vector3_add(REGISTER_KEY pspl_vector3_t a, REGISTER_KEY pspl_vector3_t b, REGISTER_KEY pspl_vector3_t ab);
//void pmdl_vector4_add(REGISTER_KEY pspl_vector4_t a, REGISTER_KEY pspl_vector4_t b, REGISTER_KEY pspl_vector4_t ab);
#define pmdl_vector3_add(a,b,ab) (ab) = (a)+(b)
#define pmdl_vector4_add(a,b,ab) (ab) = (a)+(b)

//void pmdl_vector3_sub(REGISTER_KEY pspl_vector3_t a, REGISTER_KEY pspl_vector3_t b, REGISTER_KEY pspl_vector3_t ab);
//void pmdl_vector4_sub(REGISTER_KEY pspl_vector4_t a, REGISTER_KEY pspl_vector4_t b, REGISTER_KEY pspl_vector4_t ab);
#define pmdl_vector3_sub(a,b,ab) (ab) = (a)-(b)
#define pmdl_vector4_sub(a,b,ab) (ab) = (a)-(b)

void pmdl_vector3_matrix_mul(const REGISTER_KEY pspl_matrix34_t* mtx, const REGISTER_KEY pspl_vector3_t* src, REGISTER_KEY pspl_vector3_t* dst);

void pmdl_matrix34_quat(REGISTER_KEY pspl_matrix34_t* m, REGISTER_KEY pspl_vector4_t* a);
    
void pmdl_quat_mul(pspl_vector4_t* a, pspl_vector4_t* b, pspl_vector4_t* ab);

#endif

#define pmdl_matrix_lookat(m, view) _pmdl_matrix_lookat((m), &(view).pos, &(view).up, &(view).look)
#define pmdl_matrix_orthographic(m, ortho) _pmdl_matrix_orthographic((m), (ortho).top, (ortho).bottom, (ortho).left, (ortho).right, (ortho).near, (ortho).far)
#define pmdl_matrix_perspective(m, persp) _pmdl_matrix_perspective((m), (persp).fov, (persp).aspect, (persp).near, (persp).far)

    
#ifdef __cplusplus
}
#endif
#endif
