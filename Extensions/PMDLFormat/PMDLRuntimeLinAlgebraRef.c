//
//  PMDLRuntimeLinAlgebraRef.c
//  PSPL
//
//  Created by Jack Andersen on 7/23/13.
//
//

/* This is a pure C reference implementation of the 
 * PMDL runtime linear algebra routines */

#include <PMDLRuntime.h>

#if !HW_RVL

void _pmdl_matrix_orthographic(pspl_matrix44_t* mt, float t, float b, float l,
                               float r, float n, float f) {
	float tmp;
    
	tmp = 1.0f/(r-l);
	mt->m[0][0] = 2.0f*tmp;
	mt->m[0][1] = 0.0f;
	mt->m[0][2] = 0.0f;
	mt->m[0][3] = -(r+l)*tmp;
    
	tmp = 1.0f/(t-b);
	mt->m[1][0] = 0.0f;
	mt->m[1][1] = 2.0f*tmp;
	mt->m[1][2] = 0.0f;
	mt->m[1][3] = -(t+b)*tmp;
    
	tmp = 1.0f/(f-n);
	mt->m[2][0] = 0.0f;
	mt->m[2][1] = 0.0f;
	mt->m[2][2] = -(1.0f)*tmp;
	mt->m[2][3] = -(f)*tmp;
    
	mt->m[3][0] = 0.0f;
	mt->m[3][1] = 0.0f;
	mt->m[3][2] = 0.0f;
	mt->m[3][3] = 1.0f;
}

void _pmdl_matrix_perspective(pspl_matrix44_t* mt, float fovy, float aspect, float n, float f) {
	float cot,angle,tmp;
    
	angle = fovy*0.5f;
	angle = DegToRad(angle);
	
	cot = 1.0f/tanf(angle);
    
	mt->m[0][0] = cot/aspect;
	mt->m[0][1] = 0.0f;
	mt->m[0][2] = 0.0f;
	mt->m[0][3] = 0.0f;
    
	mt->m[1][0] = 0.0f;
	mt->m[1][1] = cot;
	mt->m[1][2] = 0.0f;
	mt->m[1][3] = 0.0f;
	
	tmp = 1.0f/(f-n);
	mt->m[2][0] = 0.0f;
	mt->m[2][1] = 0.0f;
	mt->m[2][2] = -(n)*tmp;
	mt->m[2][3] = -(f*n)*tmp;
	
	mt->m[3][0] = 0.0f;
	mt->m[3][1] = 0.0f;
	mt->m[3][2] = -1.0f;
	mt->m[3][3] = 0.0f;
}

void pmdl_matrix34_identity(pspl_matrix34_t* mt) {
    int i,j;
    
	for(i=0;i<3;i++) {
		for(j=0;j<4;j++) {
			if(i==j) mt->m[i][j] = 1.0;
			else mt->m[i][j] = 0.0;
		}
	}
}

void pmdl_matrix44_identity(pspl_matrix44_t* mt) {
    int i,j;
    
	for(i=0;i<4;i++) {
		for(j=0;j<4;j++) {
			if(i==j) mt->m[i][j] = 1.0;
			else mt->m[i][j] = 0.0;
		}
	}
}

/*
void pspl_matrix34_cpy(const pspl_matrix34_t src, pspl_matrix34_t dst) {
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
 */

void pmdl_matrix34_mul(pspl_matrix34_t* a, pspl_matrix34_t* b, pspl_matrix34_t* ab) {
    pspl_matrix34_t tmp;
	pspl_matrix34_t* m;
    
	if(ab==b || ab==a)
		m = &tmp;
	else
		m = ab;
    
    m->m[0][0] = a->m[0][0]*b->m[0][0] + a->m[0][1]*b->m[1][0] + a->m[0][2]*b->m[2][0];
    m->m[0][1] = a->m[0][0]*b->m[0][1] + a->m[0][1]*b->m[1][1] + a->m[0][2]*b->m[2][1];
    m->m[0][2] = a->m[0][0]*b->m[0][2] + a->m[0][1]*b->m[1][2] + a->m[0][2]*b->m[2][2];
    m->m[0][3] = a->m[0][0]*b->m[0][3] + a->m[0][1]*b->m[1][3] + a->m[0][2]*b->m[2][3] + a->m[0][3];
    
    m->m[1][0] = a->m[1][0]*b->m[0][0] + a->m[1][1]*b->m[1][0] + a->m[1][2]*b->m[2][0];
    m->m[1][1] = a->m[1][0]*b->m[0][1] + a->m[1][1]*b->m[1][1] + a->m[1][2]*b->m[2][1];
    m->m[1][2] = a->m[1][0]*b->m[0][2] + a->m[1][1]*b->m[1][2] + a->m[1][2]*b->m[2][2];
    m->m[1][3] = a->m[1][0]*b->m[0][3] + a->m[1][1]*b->m[1][3] + a->m[1][2]*b->m[2][3] + a->m[1][3];
    
    m->m[2][0] = a->m[2][0]*b->m[0][0] + a->m[2][1]*b->m[1][0] + a->m[2][2]*b->m[2][0];
    m->m[2][1] = a->m[2][0]*b->m[0][1] + a->m[2][1]*b->m[1][1] + a->m[2][2]*b->m[2][1];
    m->m[2][2] = a->m[2][0]*b->m[0][2] + a->m[2][1]*b->m[1][2] + a->m[2][2]*b->m[2][2];
    m->m[2][3] = a->m[2][0]*b->m[0][3] + a->m[2][1]*b->m[1][3] + a->m[2][2]*b->m[2][3] + a->m[2][3];
    //float debug = a->m[2][2]*b->m[2][3];
    //float debpg2 = m->m[2][3];
    //printf("%f %f\n", debug, debpg2);
    
	if(m==&tmp) {
		pmdl_matrix34_cpy(tmp.v,ab->v);
    }
}
void pmdl_matrix44_mul(pspl_matrix44_t* a, pspl_matrix44_t* b, pspl_matrix44_t* ab) {
    pspl_matrix44_t tmp;
	pspl_matrix44_t* m;
    
	if(ab==b || ab==a)
		m = &tmp;
	else
		m = ab;
    
    m->m[0][0] = a->m[0][0]*b->m[0][0] + a->m[0][1]*b->m[1][0] + a->m[0][2]*b->m[2][0] + a->m[0][3]*b->m[3][0];
    m->m[0][1] = a->m[0][0]*b->m[0][1] + a->m[0][1]*b->m[1][1] + a->m[0][2]*b->m[2][1] + a->m[0][3]*b->m[3][1];
    m->m[0][2] = a->m[0][0]*b->m[0][2] + a->m[0][1]*b->m[1][2] + a->m[0][2]*b->m[2][2] + a->m[0][3]*b->m[3][2];
    m->m[0][3] = a->m[0][0]*b->m[0][3] + a->m[0][1]*b->m[1][3] + a->m[0][2]*b->m[2][3] + a->m[0][3]*b->m[3][3];
    
    m->m[1][0] = a->m[1][0]*b->m[0][0] + a->m[1][1]*b->m[1][0] + a->m[1][2]*b->m[2][0] + a->m[1][3]*b->m[3][0];
    m->m[1][1] = a->m[1][0]*b->m[0][1] + a->m[1][1]*b->m[1][1] + a->m[1][2]*b->m[2][1] + a->m[1][3]*b->m[3][1];
    m->m[1][2] = a->m[1][0]*b->m[0][2] + a->m[1][1]*b->m[1][2] + a->m[1][2]*b->m[2][2] + a->m[1][3]*b->m[3][2];
    m->m[1][3] = a->m[1][0]*b->m[0][3] + a->m[1][1]*b->m[1][3] + a->m[1][2]*b->m[2][3] + a->m[1][3]*b->m[3][3];
    
    m->m[2][0] = a->m[2][0]*b->m[0][0] + a->m[2][1]*b->m[1][0] + a->m[2][2]*b->m[2][0] + a->m[2][3]*b->m[3][0];
    m->m[2][1] = a->m[2][0]*b->m[0][1] + a->m[2][1]*b->m[1][1] + a->m[2][2]*b->m[2][1] + a->m[2][3]*b->m[3][1];
    m->m[2][2] = a->m[2][0]*b->m[0][2] + a->m[2][1]*b->m[1][2] + a->m[2][2]*b->m[2][2] + a->m[2][3]*b->m[3][2];
    m->m[2][3] = a->m[2][0]*b->m[0][3] + a->m[2][1]*b->m[1][3] + a->m[2][2]*b->m[2][3] + a->m[2][3]*b->m[3][3];
    
    m->m[3][0] = a->m[3][0]*b->m[0][0] + a->m[3][1]*b->m[1][0] + a->m[3][2]*b->m[2][0] + a->m[3][3]*b->m[3][0];
    m->m[3][1] = a->m[3][0]*b->m[0][1] + a->m[3][1]*b->m[1][1] + a->m[3][2]*b->m[2][1] + a->m[3][3]*b->m[3][1];
    m->m[3][2] = a->m[3][0]*b->m[0][2] + a->m[3][1]*b->m[1][2] + a->m[3][2]*b->m[2][2] + a->m[3][3]*b->m[3][2];
    m->m[3][3] = a->m[3][0]*b->m[0][3] + a->m[3][1]*b->m[1][3] + a->m[3][2]*b->m[2][2] + a->m[3][3]*b->m[3][3];
    
    
	if(m==&tmp) {
		pmdl_matrix44_cpy(tmp.v,ab->v);
    }
}
void pmdl_matrix3444_mul(pspl_matrix34_t* a, pspl_matrix44_t* b, pspl_matrix44_t* ab) {
    pspl_matrix44_t tmp;
	pspl_matrix44_t* m;
    
	if(ab==b)
		m = &tmp;
	else
		m = ab;
    
    m->m[0][0] = a->m[0][0]*b->m[0][0] + a->m[0][1]*b->m[1][0] + a->m[0][2]*b->m[2][0] + a->m[0][3]*b->m[3][0];
    m->m[0][1] = a->m[0][0]*b->m[0][1] + a->m[0][1]*b->m[1][1] + a->m[0][2]*b->m[2][1] + a->m[0][3]*b->m[3][1];
    m->m[0][2] = a->m[0][0]*b->m[0][2] + a->m[0][1]*b->m[1][2] + a->m[0][2]*b->m[2][2] + a->m[0][3]*b->m[3][2];
    m->m[0][3] = a->m[0][0]*b->m[0][3] + a->m[0][1]*b->m[1][3] + a->m[0][2]*b->m[2][3] + a->m[0][3]*b->m[3][3];
    
    m->m[1][0] = a->m[1][0]*b->m[0][0] + a->m[1][1]*b->m[1][0] + a->m[1][2]*b->m[2][0] + a->m[1][3]*b->m[3][0];
    m->m[1][1] = a->m[1][0]*b->m[0][1] + a->m[1][1]*b->m[1][1] + a->m[1][2]*b->m[2][1] + a->m[1][3]*b->m[3][1];
    m->m[1][2] = a->m[1][0]*b->m[0][2] + a->m[1][1]*b->m[1][2] + a->m[1][2]*b->m[2][2] + a->m[1][3]*b->m[3][2];
    m->m[1][3] = a->m[1][0]*b->m[0][3] + a->m[1][1]*b->m[1][3] + a->m[1][2]*b->m[2][3] + a->m[1][3]*b->m[3][3];
    
    m->m[2][0] = a->m[2][0]*b->m[0][0] + a->m[2][1]*b->m[1][0] + a->m[2][2]*b->m[2][0] + a->m[2][3]*b->m[3][0];
    m->m[2][1] = a->m[2][0]*b->m[0][1] + a->m[2][1]*b->m[1][1] + a->m[2][2]*b->m[2][1] + a->m[2][3]*b->m[3][1];
    m->m[2][2] = a->m[2][0]*b->m[0][2] + a->m[2][1]*b->m[1][2] + a->m[2][2]*b->m[2][2] + a->m[2][3]*b->m[3][2];
    m->m[2][3] = a->m[2][0]*b->m[0][3] + a->m[2][1]*b->m[1][3] + a->m[2][2]*b->m[2][3] + a->m[2][3]*b->m[3][3];
    
    m->m[3][0] = b->m[3][0];
    m->m[3][1] = b->m[3][1];
    m->m[3][2] = b->m[3][2];
    m->m[3][3] = b->m[3][3];
    
    
	if(m==&tmp) {
		pmdl_matrix44_cpy(tmp.v,ab->v);
    }
}

void pmdl_matrix34_invxpose(pspl_matrix34_t* src, pspl_matrix34_t* xPose) {
    pspl_matrix34_t mTmp;
    pspl_matrix34_t* m;
    float det;
    
    if(src == xPose)
        m = &mTmp;
    else
        m = xPose;
    
    // Compute the determinant of the upper 3x3 submatrix
    det =   src->m[0][0]*src->m[1][1]*src->m[2][2] + src->m[0][1]*src->m[1][2]*src->m[2][0] + src->m[0][2]*src->m[1][0]*src->m[2][1]
    - src->m[2][0]*src->m[1][1]*src->m[0][2] - src->m[1][0]*src->m[0][1]*src->m[2][2] - src->m[0][0]*src->m[2][1]*src->m[1][2];
    
    // Check if matrix is singular
    if(det == 0.0f) return;
    
    // Compute the inverse of the upper submatrix:
    
    // Find the transposed matrix of cofactors of the upper submatrix
    // and multiply by (1/det)
    
    det = 1.0f / det;
    
    m->m[0][0] =  (src->m[1][1]*src->m[2][2] - src->m[2][1]*src->m[1][2]) * det;
    m->m[0][1] = -(src->m[1][0]*src->m[2][2] - src->m[2][0]*src->m[1][2]) * det;
    m->m[0][2] =  (src->m[1][0]*src->m[2][1] - src->m[2][0]*src->m[1][1]) * det;
    
    m->m[1][0] = -(src->m[0][1]*src->m[2][2] - src->m[2][1]*src->m[0][2]) * det;
    m->m[1][1] =  (src->m[0][0]*src->m[2][2] - src->m[2][0]*src->m[0][2]) * det;
    m->m[1][2] = -(src->m[0][0]*src->m[2][1] - src->m[2][0]*src->m[0][1]) * det;
    
    m->m[2][0] =  (src->m[0][1]*src->m[1][2] - src->m[1][1]*src->m[0][2]) * det;
    m->m[2][1] = -(src->m[0][0]*src->m[1][2] - src->m[1][0]*src->m[0][2]) * det;
    m->m[2][2] =  (src->m[0][0]*src->m[1][1] - src->m[1][0]*src->m[0][1]) * det;
    
    
    // The 4th columns should be zero
    m->m[0][3] = 0.0F;
    m->m[1][3] = 0.0F;
    m->m[2][3] = 0.0F;
    
    // Copy back if needed
    if(m == &mTmp) {
        pmdl_matrix34_cpy(mTmp.v, xPose->v);
    }
    
}

/* Reference C implementations from libogc */
float pmdl_vector3_dot(pspl_vector3_t* a, pspl_vector3_t* b) {
    pspl_vector3_t tmp;
    tmp.v = a->v * b->v;
	return tmp.v[0] + tmp.v[1] + tmp.v[2];
}
float pmdl_vector4_dot(pspl_vector4_t* a, pspl_vector4_t* b) {
    pspl_vector4_t tmp;
    tmp.v = a->v * b->v;
	return tmp.v[0] + tmp.v[1] + tmp.v[2] + tmp.v[3];
}

void pmdl_vector3_cross(pspl_vector3_t* a, pspl_vector3_t* b, pspl_vector3_t* axb) {
    pspl_vector3_t vTmp;
    
	vTmp.f[0] = (a->f[1]*b->f[2])-(a->f[2]*b->f[1]);
	vTmp.f[1] = (a->f[2]*b->f[0])-(a->f[0]*b->f[2]);
	vTmp.f[2] = (a->f[0]*b->f[1])-(a->f[1]*b->f[0]);
    
	axb->f[0] = vTmp.f[0];
	axb->f[1] = vTmp.f[1];
	axb->f[2] = vTmp.f[2];
}

void pmdl_vector3_normalise(pspl_vector3_t* v) {
    float m;
    pspl_vector3_t tmp;
    tmp.v = v->v * v->v;
	m = tmp.v[0] + tmp.v[1] + tmp.v[2];
	m = 1/sqrtf(m);
    v->v *= m;
}

/*
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
 */

void pmdl_vector3_matrix_mul(pspl_matrix34_t* mt, pspl_vector3_t* src, pspl_vector3_t* dst) {
    pspl_vector3_t tmp;
	
    tmp.f[0] = mt->m[0][0]*src->f[0] + mt->m[0][1]*src->f[1] + mt->m[0][2]*src->f[2] + mt->m[0][3];
    tmp.f[1] = mt->m[1][0]*src->f[0] + mt->m[1][1]*src->f[1] + mt->m[1][2]*src->f[2] + mt->m[1][3];
    tmp.f[2] = mt->m[2][0]*src->f[0] + mt->m[2][1]*src->f[1] + mt->m[2][2]*src->f[2] + mt->m[2][3];
    
    dst->v = tmp.v;

}

void _pmdl_matrix_lookat(pspl_matrix34_t* mt, pspl_vector3_t* pos, pspl_vector3_t* up, pspl_vector3_t* look) {
    pspl_vector3_t vLook,vRight,vUp;
    
    vLook.v = pos->v - look->v;
    pmdl_vector3_normalise(&vLook);
    
	pmdl_vector3_cross(up,&vLook,&vRight);
	pmdl_vector3_normalise(&vRight);
	
	pmdl_vector3_cross(&vLook,&vRight,&vUp);
    
    mt->m[0][0] = vRight.f[0];
    mt->m[0][1] = vRight.f[1];
    mt->m[0][2] = vRight.f[2];
    mt->m[0][3] = -pmdl_vector3_dot(pos, &vRight);
    
    mt->m[1][0] = vUp.f[0];
    mt->m[1][1] = vUp.f[1];
    mt->m[1][2] = vUp.f[2];
    mt->m[1][3] = -pmdl_vector3_dot(pos, &vUp);
    
    mt->m[2][0] = vLook.f[0];
    mt->m[2][1] = vLook.f[1];
    mt->m[2][2] = vLook.f[2];
    mt->m[2][3] = -pmdl_vector3_dot(pos, &vLook);
    
}


void pmdl_matrix34_quat(pspl_matrix34_t m, pspl_vector4_t* a) {
	m.m[0][0] = 1.0f - 2.0f*a->f[1]*a->f[1] - 2.0f*a->f[2]*a->f[2];
	m.m[1][0] = 2.0f*a->f[0]*a->f[1] - 2.0f*a->f[2]*a->f[3];
	m.m[2][0] = 2.0f*a->f[0]*a->f[2] + 2.0f*a->f[1]*a->f[3];
    
	m.m[0][1] = 2.0f*a->f[0]*a->f[1] + 2.0f*a->f[2]*a->f[3];
	m.m[1][1] = 1.0f - 2.0f*a->f[0]*a->f[0] - 2.0f*a->f[2]*a->f[2];
	m.m[2][1] = 2.0f*a->f[2]*a->f[1] - 2.0f*a->f[0]*a->f[3];
    
	m.m[0][2] = 2.0f*a->f[0]*a->f[2] - 2.0f*a->f[1]*a->f[3];
	m.m[1][2] = 2.0f*a->f[2]*a->f[1] + 2.0f*a->f[0]*a->f[3];
	m.m[2][2] = 1.0f - 2.0f*a->f[0]*a->f[0] - 2.0f*a->f[1]*a->f[1];
}

#endif
