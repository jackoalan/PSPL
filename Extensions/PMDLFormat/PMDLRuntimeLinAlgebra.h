//
//  PMDLRuntimeLinAlgebra.h
//  PSPL
//
//  Created by Jack Andersen on 7/23/13.
//
//

#ifndef PSPL_PMDLRuntimeMatrix_h
#define PSPL_PMDLRuntimeMatrix_h

#include <PSPL/PSPLCommon.h>
#include <math.h>

/* Use assembly leaf implementations */
#ifdef PMDL_ASM
#define HW_RVL 1
#define REGISTER_KEY register
#else
#define REGISTER_KEY
#endif


/* For GX compatibility, this file is conditionally compiled to
 * link to libogc matrix routines or supply identical versions
 * for OpenGL/D3D */

#define DegToRad(a)   ( (a) *  0.01745329252f )

#if PSPL_RUNTIME_PLATFORM_GX
#include <ogc/gu.h>
#define _pmdl_matrix_orthographic guOrtho
#define _pmdl_matrix_perspective guPerspective
#define _pmdl_matrix_lookat(m,p,u,l) guLookAt((m),(guVector*)(p),(guVector*)(u),(guVector*)(l))
#define pmdl_matrix34_mul guMtxConcat
#define pmdl_matrix34_invxpose guMtxInvXpose
#define pmdl_vector3_dot(a,b) guVecDotProduct((guVector*)(a),(guVector*)(b))
#define pmdl_vector3_cross(a,b,axb) guVecCross((guVector*)(a),(guVector*)(b),(guVector*)(axb))
#define pmdl_vector3_normalise(v) guVecNormalize((guVector*)(v))
#define pmdl_vector3_scale(src,dst,scale) guVecScale((guVector*)(src),(guVector*)(dst),(scale))
#define pmdl_vector3_add(a,b,ab) guVecAdd((guVector*)(a),(guVector*)(b),(guVector*)(ab))
#define pmdl_vector3_sub(a,b,ab) guVecSub((guVector*)(a),(guVector*)(b),(guVector*)(ab))
#define pmdl_vector3_matrix_mul(m,s,d) guVecMultiply((m),(guVector*)(&s[0]),(guVector*)(&d[0]))

/* Extended libogc matrix routines */
void pspl_matrix44_cpy(REGISTER_KEY pspl_matrix44_t src, REGISTER_KEY pspl_matrix44_t dst);

void pmdl_matrix44_mul(REGISTER_KEY pspl_matrix44_t a, REGISTER_KEY pspl_matrix44_t b, REGISTER_KEY pspl_matrix44_t ab);
void pmdl_matrix3444_mul(REGISTER_KEY pspl_matrix34_t a, REGISTER_KEY pspl_matrix44_t b, REGISTER_KEY pspl_matrix44_t ab);

float pmdl_vector4_dot(REGISTER_KEY pspl_vector4_t a, REGISTER_KEY pspl_vector4_t b);

void pmdl_vector4_scale(REGISTER_KEY pspl_vector4_t src, REGISTER_KEY pspl_vector4_t dst, REGISTER_KEY float scale);

void pmdl_vector4_add(REGISTER_KEY pspl_vector4_t a, REGISTER_KEY pspl_vector4_t b, REGISTER_KEY pspl_vector4_t ab);

void pmdl_vector4_sub(REGISTER_KEY pspl_vector4_t a, REGISTER_KEY pspl_vector4_t b, REGISTER_KEY pspl_vector4_t ab);


#else

void _pmdl_matrix_orthographic(pspl_matrix44_t mt, float t, float b, float l,
                               float r, float n, float f);

void _pmdl_matrix_perspective(pspl_matrix44_t mt, float fovy, float aspect, float n, float f);
void _pmdl_matrix_lookat(pspl_matrix34_t mt, pspl_vector3_t pos, pspl_vector3_t up, pspl_vector3_t look);

void pspl_matrix34_cpy(REGISTER_KEY pspl_matrix34_t src, REGISTER_KEY pspl_matrix34_t dst);
void pspl_matrix44_cpy(REGISTER_KEY pspl_matrix44_t src, REGISTER_KEY pspl_matrix44_t dst);

void pmdl_matrix34_mul(REGISTER_KEY pspl_matrix34_t a, REGISTER_KEY pspl_matrix34_t b, REGISTER_KEY pspl_matrix34_t ab);
void pmdl_matrix44_mul(REGISTER_KEY pspl_matrix44_t a, REGISTER_KEY pspl_matrix44_t b, REGISTER_KEY pspl_matrix44_t ab);
void pmdl_matrix3444_mul(REGISTER_KEY pspl_matrix34_t a, REGISTER_KEY pspl_matrix44_t b, REGISTER_KEY pspl_matrix44_t ab);

void pmdl_matrix34_invxpose(REGISTER_KEY pspl_matrix34_t src, REGISTER_KEY pspl_matrix34_t xpose);

/* Reference C implementations from libogc */
float pmdl_vector3_dot(REGISTER_KEY pspl_vector3_t a, REGISTER_KEY pspl_vector3_t b);
float pmdl_vector4_dot(REGISTER_KEY pspl_vector4_t a, REGISTER_KEY pspl_vector4_t b);

void pmdl_vector3_cross(REGISTER_KEY pspl_vector3_t a, REGISTER_KEY pspl_vector3_t b, REGISTER_KEY pspl_vector3_t axb);

void pmdl_vector3_normalise(REGISTER_KEY pspl_vector3_t v);

void pmdl_vector3_scale(REGISTER_KEY pspl_vector3_t src, REGISTER_KEY pspl_vector3_t dst, REGISTER_KEY float scale);
void pmdl_vector4_scale(REGISTER_KEY pspl_vector4_t src, REGISTER_KEY pspl_vector4_t dst, REGISTER_KEY float scale);

void pmdl_vector3_add(REGISTER_KEY pspl_vector3_t a, REGISTER_KEY pspl_vector3_t b, REGISTER_KEY pspl_vector3_t ab);
void pmdl_vector4_add(REGISTER_KEY pspl_vector4_t a, REGISTER_KEY pspl_vector4_t b, REGISTER_KEY pspl_vector4_t ab);

void pmdl_vector3_sub(REGISTER_KEY pspl_vector3_t a, REGISTER_KEY pspl_vector3_t b, REGISTER_KEY pspl_vector3_t ab);
void pmdl_vector4_sub(REGISTER_KEY pspl_vector4_t a, REGISTER_KEY pspl_vector4_t b, REGISTER_KEY pspl_vector4_t ab);

void pmdl_vector3_matrix_mul(REGISTER_KEY pspl_matrix34_t mtx, REGISTER_KEY pspl_vector3_t src, REGISTER_KEY pspl_vector3_t dst);


#endif

#define pmdl_matrix_lookat(m, view) _pmdl_matrix_lookat((m), (view).pos, (view).up, (view).look)
#define pmdl_matrix_orthographic(m, ortho) _pmdl_matrix_orthographic((m), (ortho).top, (ortho).bottom, (ortho).left, (ortho).right, (ortho).near, (ortho).far)
#define pmdl_matrix_perspective(m, persp) _pmdl_matrix_perspective((m), (persp).fov, (persp).aspect, (persp).near, (persp).far)

#endif
