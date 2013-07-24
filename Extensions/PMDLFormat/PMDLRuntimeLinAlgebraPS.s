//
//  PMDLRuntimeLinAlgebraPS.s
//  PSPL
//
//  Created by Jack Andersen on 7/23/13.
//
//

// Extension of libogc's GU matrix library (using Broadway's Paired-Single instructions)
// Adds 4-component vector operations and 4x4 matrix operations

#include <ogc/machine/asm.h>

#define A00_A01		fr0
#define A02_A03		fr1
#define A10_A11		fr2
#define A12_A13		fr3
#define A20_A21		fr4
#define A22_A23		fr5
#define A30_A31		fr6
#define A32_A33		fr7

#define B00_B01		fr8
#define B02_B03		fr9
#define B10_B11		fr10
#define B12_B13		fr11
#define B20_B21		fr12
#define B22_B23		fr13
#define B30_B31		fr14
#define B32_B33		fr15

#define D00_D01		fr16
#define D02_D03		fr17
#define D10_D11		fr18
#define D12_D13		fr19
#define D20_D21		fr20
#define D22_D23		fr21
#define D30_D31		fr22
#define D32_D33		fr23

#define UNIT01		fr31

#define RET_REG		fr1
#define V1_XY		fr2
#define V1_ZW		fr3
#define V2_XY		fr4
#define V2_ZW		fr5
#define D1_XY		fr6
#define D1_ZW		fr7
#define D2_XY		fr8
#define D2_ZW		fr9
#define W1_XY		fr10
#define W1_ZW		fr11
#define W2_XY		fr12
#define W2_ZW		fr13


	.globl	pmdl_matrix44_mul
	//r3 = mtxA, r4 = mtxB, r5 = mtxAB
pmdl_matrix44_mul:
	stwu		r1,-64(r1)
	psq_l		A00_A01,0(r3),0,0
	stfd		fr14,8(r1)
	psq_l		B00_B01,0(r4),0,0
	psq_l		B02_B03,8(r4),0,0
	stfd		fr15,16(r1)
	stfd		fr31,40(r1)
	psq_l		B10_B11,16(r4),0,0
	ps_muls0	D00_D01,B00_B01,A00_A01
	psq_l		A10_A11,16(r3),0,0
	ps_muls0	D02_D03,B02_B03,A00_A01
	psq_l		UNIT01,Unit01@sdarel(r13),0,0
	ps_muls0	D10_D11,B00_B01,A10_A11
	psq_l		B12_B13,24(r4),0,0
	ps_muls0	D12_D13,B02_B03,A10_A11
	psq_l		A02_A03,8(r3),0,0
	ps_madds1	D00_D01,B10_B11,A00_A01,D00_D01
	psq_l		A12_A13,24(r3),0,0
	ps_madds1	D10_D11,B10_B11,A10_A11,D10_D11
	psq_l		B20_B21,32(r4),0,0
	ps_madds1	D02_D03,B12_B13,A00_A01,D02_D03
	psq_l		B22_B23,40(r4),0,0
	ps_madds1	D12_D13,B12_B13,A10_A11,D12_D13
	psq_l		A20_A21,32(r3),0,0
	psq_l		A22_A23,40(r3),0,0
	ps_madds0	D00_D01,B20_B21,A02_A03,D00_D01
	ps_madds0	D02_D03,B22_B23,A02_A03,D02_D03
	ps_madds0	D10_D11,B20_B21,A12_A13,D10_D11
	ps_madds0	D12_D13,B22_B23,A12_A13,D12_D13
	psq_st		D00_D01,0(r5),0,0
	ps_muls0	D20_D21,B00_B01,A20_A21
	ps_madds1	D02_D03,UNIT01,A02_A03,D02_D03
	ps_muls0	D22_D23,B02_B03,A20_A21
	psq_st		D10_D11,16(r5),0,0
	ps_madds1	D12_D13,UNIT01,A12_A13,D12_D13
	psq_st		D02_D03,8(r5),0,0
	ps_madds1	D20_D21,B10_B11,A20_A21,D20_D21
	ps_madds1	D22_D23,B12_B13,A20_A21,D22_D23
	ps_madds0	D20_D21,B20_B21,A22_A23,D20_D21
	lfd		fr14,8(r1)
	psq_st		D12_D13,24(r5),0,0
	ps_madds0	D22_D23,B22_B23,A22_A23,D22_D23
	psq_st		D20_D21,32(r5),0,0
	ps_madds1	D22_D23,UNIT01,A22_A23,D22_D23
	lfd		fr15,16(r1)
	psq_st		D22_D23,40(r5),0,0
	lfd		fr31,40(r1)
	addi		r1,r1,64
	blr
