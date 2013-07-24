//
//  PMDLRuntimeLinAlgebraRef.c
//  PSPL
//
//  Created by Jack Andersen on 7/23/13.
//
//

/* This is a pure C reference implementation of the 
 * PMDL runtime linear algebra routines */

#include "PMDLRuntimeLinAlgebra.h"

#define DegToRad(a)   ( (a) *  0.01745329252f )

void _pmdl_matrix_orthographic(pspl_matrix44_t mt, float t, float b, float l,
                               float r, float n, float f) {
	float tmp;
    
	tmp = 1.0f/(r-l);
	mt[0][0] = 2.0f*tmp;
	mt[0][1] = 0.0f;
	mt[0][2] = 0.0f;
	mt[0][3] = -(r+l)*tmp;
    
	tmp = 1.0f/(t-b);
	mt[1][0] = 0.0f;
	mt[1][1] = 2.0f*tmp;
	mt[1][2] = 0.0f;
	mt[1][3] = -(t+b)*tmp;
    
	tmp = 1.0f/(f-n);
	mt[2][0] = 0.0f;
	mt[2][1] = 0.0f;
	mt[2][2] = -(1.0f)*tmp;
	mt[2][3] = -(f)*tmp;
    
	mt[3][0] = 0.0f;
	mt[3][1] = 0.0f;
	mt[3][2] = 0.0f;
	mt[3][3] = 1.0f;
}

void _pmdl_matrix_perspective(pspl_matrix44_t mt, float fovy, float aspect, float n, float f) {
	float cot,angle,tmp;
    
	angle = fovy*0.5f;
	angle = DegToRad(angle);
	
	cot = 1.0f/tanf(angle);
    
	mt[0][0] = cot/aspect;
	mt[0][1] = 0.0f;
	mt[0][2] = 0.0f;
	mt[0][3] = 0.0f;
    
	mt[1][0] = 0.0f;
	mt[1][1] = cot;
	mt[1][2] = 0.0f;
	mt[1][3] = 0.0f;
	
	tmp = 1.0f/(f-n);
	mt[2][0] = 0.0f;
	mt[2][1] = 0.0f;
	mt[2][2] = -(n)*tmp;
	mt[2][3] = -(f*n)*tmp;
	
	mt[3][0] = 0.0f;
	mt[3][1] = 0.0f;
	mt[3][2] = -1.0f;
	mt[3][3] = 0.0f;
}

void pspl_matrix34_cpy(pspl_matrix34_t src, pspl_matrix34_t dst) {
    if(src==dst) return;
    
    dst[0][0] = src[0][0];    dst[0][1] = src[0][1];    dst[0][2] = src[0][2];    dst[0][3] = src[0][3];
    dst[1][0] = src[1][0];    dst[1][1] = src[1][1];    dst[1][2] = src[1][2];    dst[1][3] = src[1][3];
    dst[2][0] = src[2][0];    dst[2][1] = src[2][1];    dst[2][2] = src[2][2];    dst[2][3] = src[2][3];
}
void pspl_matrix44_cpy(pspl_matrix44_t src, pspl_matrix44_t dst) {
    if(src==dst) return;
    
    dst[0][0] = src[0][0];    dst[0][1] = src[0][1];    dst[0][2] = src[0][2];    dst[0][3] = src[0][3];
    dst[1][0] = src[1][0];    dst[1][1] = src[1][1];    dst[1][2] = src[1][2];    dst[1][3] = src[1][3];
    dst[2][0] = src[2][0];    dst[2][1] = src[2][1];    dst[2][2] = src[2][2];    dst[2][3] = src[2][3];
    dst[3][0] = src[3][0];    dst[3][1] = src[3][1];    dst[3][2] = src[3][2];    dst[3][3] = src[3][3];
}

typedef float (*pspl_matrix_ptr_t)[4];
void pmdl_matrix34_mul(pspl_matrix34_t a, pspl_matrix34_t b, pspl_matrix34_t ab) {
    pspl_matrix34_t tmp;
	pspl_matrix_ptr_t m;
    
	if(ab==b || ab==a)
		m = tmp;
	else
		m = ab;
    
    m[0][0] = a[0][0]*b[0][0] + a[0][1]*b[1][0] + a[0][2]*b[2][0];
    m[0][1] = a[0][0]*b[0][1] + a[0][1]*b[1][1] + a[0][2]*b[2][1];
    m[0][2] = a[0][0]*b[0][2] + a[0][1]*b[1][2] + a[0][2]*b[2][2];
    m[0][3] = a[0][0]*b[0][3] + a[0][1]*b[1][3] + a[0][2]*b[2][3] + a[0][3];
    
    m[1][0] = a[1][0]*b[0][0] + a[1][1]*b[1][0] + a[1][2]*b[2][0];
    m[1][1] = a[1][0]*b[0][1] + a[1][1]*b[1][1] + a[1][2]*b[2][1];
    m[1][2] = a[1][0]*b[0][2] + a[1][1]*b[1][2] + a[1][2]*b[2][2];
    m[1][3] = a[1][0]*b[0][3] + a[1][1]*b[1][3] + a[1][2]*b[2][3] + a[1][3];
    
    m[2][0] = a[2][0]*b[0][0] + a[2][1]*b[1][0] + a[2][2]*b[2][0];
    m[2][1] = a[2][0]*b[0][1] + a[2][1]*b[1][1] + a[2][2]*b[2][1];
    m[2][2] = a[2][0]*b[0][2] + a[2][1]*b[1][2] + a[2][2]*b[2][2];
    m[2][3] = a[2][0]*b[0][3] + a[2][1]*b[1][3] + a[2][2]*b[2][3] + a[2][3];
    
	if(m==tmp)
		pspl_matrix34_cpy(tmp,ab);
}
void pmdl_matrix44_mul(pspl_matrix44_t a, pspl_matrix44_t b, pspl_matrix44_t ab) {
    pspl_matrix44_t tmp;
	pspl_matrix_ptr_t m;
    
	if(ab==b || ab==a)
		m = tmp;
	else
		m = ab;
    
    m[0][0] = a[0][0]*b[0][0] + a[0][1]*b[1][0] + a[0][2]*b[2][0] + a[0][3]*b[3][0];
    m[0][1] = a[0][0]*b[0][1] + a[0][1]*b[1][1] + a[0][2]*b[2][1] + a[0][3]*b[3][1];
    m[0][2] = a[0][0]*b[0][2] + a[0][1]*b[1][2] + a[0][2]*b[2][2] + a[0][3]*b[3][2];
    m[0][3] = a[0][0]*b[0][3] + a[0][1]*b[1][3] + a[0][2]*b[2][3] + a[0][3]*b[3][3];
    
    m[1][0] = a[1][0]*b[0][0] + a[1][1]*b[1][0] + a[1][2]*b[2][0] + a[1][3]*b[3][0];
    m[1][1] = a[1][0]*b[0][1] + a[1][1]*b[1][1] + a[1][2]*b[2][1] + a[1][3]*b[3][1];
    m[1][2] = a[1][0]*b[0][2] + a[1][1]*b[1][2] + a[1][2]*b[2][2] + a[1][3]*b[3][2];
    m[1][3] = a[1][0]*b[0][3] + a[1][1]*b[1][3] + a[1][2]*b[2][3] + a[1][3]*b[3][3];
    
    m[2][0] = a[2][0]*b[0][0] + a[2][1]*b[1][0] + a[2][2]*b[2][0] + a[2][3]*b[3][0];
    m[2][1] = a[2][0]*b[0][1] + a[2][1]*b[1][1] + a[2][2]*b[2][1] + a[2][3]*b[3][1];
    m[2][2] = a[2][0]*b[0][2] + a[2][1]*b[1][2] + a[2][2]*b[2][2] + a[2][3]*b[3][2];
    m[2][3] = a[2][0]*b[0][3] + a[2][1]*b[1][3] + a[2][2]*b[2][3] + a[2][3]*b[3][3];
    
    m[3][0] = a[3][0]*b[0][0] + a[3][1]*b[1][0] + a[3][2]*b[2][0] + a[3][3]*b[3][0];
    m[3][1] = a[3][0]*b[0][1] + a[3][1]*b[1][1] + a[3][2]*b[2][1] + a[3][3]*b[3][1];
    m[3][2] = a[3][0]*b[0][2] + a[3][1]*b[1][2] + a[3][2]*b[2][2] + a[3][3]*b[3][2];
    m[3][3] = a[3][0]*b[0][3] + a[3][1]*b[1][3] + a[3][2]*b[2][2] + a[3][3]*b[3][3];
    
    
	if(m==tmp)
		pspl_matrix44_cpy(tmp,ab);
}
void pmdl_matrix3444_mul(pspl_matrix34_t a, pspl_matrix44_t b, pspl_matrix44_t ab) {
    pspl_matrix44_t tmp;
	pspl_matrix_ptr_t m;
    
	if(ab==b || ab==a)
		m = tmp;
	else
		m = ab;
    
    m[0][0] = a[0][0]*b[0][0] + a[0][1]*b[1][0] + a[0][2]*b[2][0] + a[0][3]*b[3][0];
    m[0][1] = a[0][0]*b[0][1] + a[0][1]*b[1][1] + a[0][2]*b[2][1] + a[0][3]*b[3][1];
    m[0][2] = a[0][0]*b[0][2] + a[0][1]*b[1][2] + a[0][2]*b[2][2] + a[0][3]*b[3][2];
    m[0][3] = a[0][0]*b[0][3] + a[0][1]*b[1][3] + a[0][2]*b[2][3] + a[0][3]*b[3][3];
    
    m[1][0] = a[1][0]*b[0][0] + a[1][1]*b[1][0] + a[1][2]*b[2][0] + a[1][3]*b[3][0];
    m[1][1] = a[1][0]*b[0][1] + a[1][1]*b[1][1] + a[1][2]*b[2][1] + a[1][3]*b[3][1];
    m[1][2] = a[1][0]*b[0][2] + a[1][1]*b[1][2] + a[1][2]*b[2][2] + a[1][3]*b[3][2];
    m[1][3] = a[1][0]*b[0][3] + a[1][1]*b[1][3] + a[1][2]*b[2][3] + a[1][3]*b[3][3];
    
    m[2][0] = a[2][0]*b[0][0] + a[2][1]*b[1][0] + a[2][2]*b[2][0] + a[2][3]*b[3][0];
    m[2][1] = a[2][0]*b[0][1] + a[2][1]*b[1][1] + a[2][2]*b[2][1] + a[2][3]*b[3][1];
    m[2][2] = a[2][0]*b[0][2] + a[2][1]*b[1][2] + a[2][2]*b[2][2] + a[2][3]*b[3][2];
    m[2][3] = a[2][0]*b[0][3] + a[2][1]*b[1][3] + a[2][2]*b[2][3] + a[2][3]*b[3][3];
    
    m[3][0] = b[3][0];
    m[3][1] = b[3][1];
    m[3][2] = b[3][2];
    m[3][3] = b[3][3];
    
    
	if(m==tmp)
		pspl_matrix44_cpy(tmp,ab);
}

void pmdl_matrix34_invxpose(pspl_matrix34_t src, pspl_matrix34_t xPose) {
    pspl_matrix34_t mTmp;
    pspl_matrix_ptr_t m;
    float det;
    
    if(src == xPose)
        m = mTmp;
    else
        m = xPose;
    
    // Compute the determinant of the upper 3x3 submatrix
    det =   src[0][0]*src[1][1]*src[2][2] + src[0][1]*src[1][2]*src[2][0] + src[0][2]*src[1][0]*src[2][1]
    - src[2][0]*src[1][1]*src[0][2] - src[1][0]*src[0][1]*src[2][2] - src[0][0]*src[2][1]*src[1][2];
    
    // Check if matrix is singular
    if(det == 0.0f) return;
    
    // Compute the inverse of the upper submatrix:
    
    // Find the transposed matrix of cofactors of the upper submatrix
    // and multiply by (1/det)
    
    det = 1.0f / det;
    
    m[0][0] =  (src[1][1]*src[2][2] - src[2][1]*src[1][2]) * det;
    m[0][1] = -(src[1][0]*src[2][2] - src[2][0]*src[1][2]) * det;
    m[0][2] =  (src[1][0]*src[2][1] - src[2][0]*src[1][1]) * det;
    
    m[1][0] = -(src[0][1]*src[2][2] - src[2][1]*src[0][2]) * det;
    m[1][1] =  (src[0][0]*src[2][2] - src[2][0]*src[0][2]) * det;
    m[1][2] = -(src[0][0]*src[2][1] - src[2][0]*src[0][1]) * det;
    
    m[2][0] =  (src[0][1]*src[1][2] - src[1][1]*src[0][2]) * det;
    m[2][1] = -(src[0][0]*src[1][2] - src[1][0]*src[0][2]) * det;
    m[2][2] =  (src[0][0]*src[1][1] - src[1][0]*src[0][1]) * det;
    
    
    // The 4th columns should be zero
    m[0][3] = 0.0F;
    m[1][3] = 0.0F;
    m[2][3] = 0.0F;
    
    // Copy back if needed
    if(m == mTmp)
        pspl_matrix34_cpy(mTmp, xPose);
    
}

/* Reference C implementations from libogc */
float pmdl_vector3_dot(pspl_vector3_t a, pspl_vector3_t b) {
    float dot;
    
	dot = (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
    
    return dot;
}
float pmdl_vector4_dot(pspl_vector4_t a, pspl_vector4_t b) {
    float dot;
    
	dot = (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]) + (a[3] * b[3]);
    
    return dot;
}

void pmdl_vector3_cross(pspl_vector3_t a, pspl_vector3_t b, pspl_vector3_t axb) {
    pspl_vector3_t vTmp;
    
	vTmp[0] = (a[1]*b[2])-(a[2]*b[1]);
	vTmp[1] = (a[2]*b[0])-(a[0]*b[2]);
	vTmp[2] = (a[0]*b[1])-(a[1]*b[0]);
    
	axb[0] = vTmp[0];
	axb[1] = vTmp[1];
	axb[2] = vTmp[2];
}

void pmdl_vector3_normalise(pspl_vector3_t v) {
    float m;
    
	m = ((v[0])*(v[0])) + ((v[1])*(v[1])) + ((v[2])*(v[2]));
	m = 1/sqrtf(m);
	v[0] *= m;
	v[1] *= m;
	v[2] *= m;
}

void pmdl_vector3_scale(pspl_vector3_t src, pspl_vector3_t dst, float scale) {
    dst[0] = src[0] * scale;
    dst[1] = src[1] * scale;
    dst[2] = src[2] * scale;
}
void pmdl_vector4_scale(pspl_vector4_t src, pspl_vector4_t dst, float scale) {
    dst[0] = src[0] * scale;
    dst[1] = src[1] * scale;
    dst[2] = src[2] * scale;
    dst[3] = src[3] * scale;
}

void pmdl_vector3_add(pspl_vector3_t a, pspl_vector3_t b, pspl_vector3_t ab) {
    ab[0] = a[0] + b[0];
    ab[1] = a[1] + b[1];
    ab[2] = a[2] + b[2];
}
void pmdl_vector4_add(pspl_vector4_t a, pspl_vector4_t b, pspl_vector4_t ab) {
    ab[0] = a[0] + b[0];
    ab[1] = a[1] + b[1];
    ab[2] = a[2] + b[2];
    ab[3] = a[3] + b[3];
}

void pmdl_vector3_sub(pspl_vector3_t a, pspl_vector3_t b, pspl_vector3_t ab) {
    ab[0] = a[0] - b[0];
    ab[1] = a[1] - b[1];
    ab[2] = a[2] - b[2];
}
void pmdl_vector4_sub(pspl_vector4_t a, pspl_vector4_t b, pspl_vector4_t ab) {
    ab[0] = a[0] - b[0];
    ab[1] = a[1] - b[1];
    ab[2] = a[2] - b[2];
    ab[3] = a[3] - b[3];
}

void _pmdl_matrix_lookat(pspl_matrix34_t mt, pspl_vector3_t pos, pspl_vector3_t up, pspl_vector3_t look) {
    pspl_vector3_t vLook,vRight,vUp;
    
	vLook[0] = pos[0] - look[0];
	vLook[1] = pos[1] - look[1];
	vLook[2] = pos[2] - look[2];
    pmdl_vector3_normalise(vLook);
    
	pmdl_vector3_cross(up,vLook,vRight);
	pmdl_vector3_normalise(vRight);
	
	pmdl_vector3_cross(vLook,vRight,vUp);
    
    mt[0][0] = vRight[0];
    mt[0][1] = vRight[1];
    mt[0][2] = vRight[2];
    mt[0][3] = -( pos[0] * vRight[0] + pos[1] * vRight[1] + pos[2] * vRight[2] );
    
    mt[1][0] = vUp[0];
    mt[1][1] = vUp[1];
    mt[1][2] = vUp[2];
    mt[1][3] = -( pos[0] * vUp[0] + pos[1] * vUp[1] + pos[2] * vUp[2] );
    
    mt[2][0] = vLook[0];
    mt[2][1] = vLook[1];
    mt[2][2] = vLook[2];
    mt[2][3] = -( pos[0] * vLook[0] + pos[1] * vLook[1] + pos[2] * vLook[2] );
}
