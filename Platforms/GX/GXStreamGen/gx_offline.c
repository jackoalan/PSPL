//
//  gx_offline.c
//  PSPL
//
//  Created by Jack Andersen on 4/16/13.
//
//

/* This file is primarily derived from libogc's zlib-licenced GX implementation
 * http://sourceforge.net/p/devkitpro/libogc/ci/master/tree/libogc_license.txt */

/* This file serves as a deferred, offline GX implementation (sans drawing commands)
 * It includes a subset of GX commands to bind internal objects (like matrices), 
 * set TEV states, and set vertex lighting channel states
 * (Material and Ambient colours).
 *
 * Calling these functions (encapsulated by `pspl_gx_offline_begin_transaction` 
 * and `pspl_gx_offline_end_transaction`) build a display list stream for the 
 * GameCube/Wii GPU write-gather pipe */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <PSPL/PSPLValue.h>
#include <PSPL/PSPLCommon.h>
#include "gx_offline.h"

// Offline GX register state for building transaction
static struct __gx_regdef gx_o;
static struct __gx_regdef *__gx = &gx_o;

#pragma mark Offline Transaction Builder for PSGX stream

static struct gx_offline_trans_state {
    // Dynamically-sized buffer
    void* mem_buf;
    union {
        void* buf;
        uint8_t* u8;
        uint16_t* u16;
        uint32_t* u32;
        float* fl;
    } mem_cur;
    size_t mem_cap;
} gx_trans_o;


void pspl_gx_offline_check_capacity() {
    size_t size = gx_trans_o.mem_cur.buf - gx_trans_o.mem_buf;
    if (size+4 > gx_trans_o.mem_cap) {
        gx_trans_o.mem_cap *= 2;
        gx_trans_o.mem_buf = realloc(gx_trans_o.mem_buf, gx_trans_o.mem_cap);
        gx_trans_o.mem_cur.buf = gx_trans_o.mem_buf + size;
    }
}

void pspl_gx_offline_begin_transaction() {
    memset(__gx, 0, sizeof(struct __gx_regdef));
    gx_trans_o.mem_cap = 1024;
    gx_trans_o.mem_buf = malloc(gx_trans_o.mem_cap);
    gx_trans_o.mem_cur.buf = gx_trans_o.mem_buf;
}

void pspl_gx_offline_add_u8(uint8_t val);
size_t pspl_gx_offline_end_transaction(void** buf_out) {
    *buf_out = gx_trans_o.mem_buf;
    size_t size = gx_trans_o.mem_cur.buf - gx_trans_o.mem_buf;
    size_t extra = ROUND_UP_32(size) - size;
    size += extra;
    int i;
    for (i=0 ; i<extra ; ++i)
        pspl_gx_offline_add_u8(0);
    return size;
}


void pspl_gx_offline_add_u8(uint8_t val) {
    pspl_gx_offline_check_capacity();
    *gx_trans_o.mem_cur.u8 = val;
    gx_trans_o.mem_cur.buf += 1;
}

void pspl_gx_offline_add_u16(uint16_t val) {
#   if __LITTLE_ENDIAN__
    val = swap_uint16(val);
#   endif
    pspl_gx_offline_check_capacity();
    *gx_trans_o.mem_cur.u16 = val;
    gx_trans_o.mem_cur.buf += 2;
}

void pspl_gx_offline_add_u32(uint32_t val) {
#   if __LITTLE_ENDIAN__
    val = swap_uint32(val);
#   endif
    pspl_gx_offline_check_capacity();
    *gx_trans_o.mem_cur.u32 = val;
    gx_trans_o.mem_cur.buf += 4;
}

void pspl_gx_offline_add_float(float val) {
#   if __LITTLE_ENDIAN__
    val = swap_float(val);
#   endif
    pspl_gx_offline_check_capacity();
    *gx_trans_o.mem_cur.fl = val;
    gx_trans_o.mem_cur.buf += 4;
}

#pragma mark libogc Derivative

//#define _GP_DEBUG
#define TEXCACHE_TESTING


#define GX_FINISH  	2

#if defined(HW_DOL)
    #define LARGE_NUMBER	(-1048576.0f)
#elif defined(HW_RVL)
    #define LARGE_NUMBER	(-1.0e+18f)
#endif

#define _SHIFTL(v, s, w)	\
    ((u32) (((u32)(v) & ((0x01 << (w)) - 1)) << (s)))
#define _SHIFTR(v, s, w)	\
    ((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))

#define GX_LOAD_BP_REG(x)				\
    do {								\
        pspl_gx_offline_add_u8(0x61);				\
        pspl_gx_offline_add_u32((u32)(x));		\
    } while(0)

#define GX_LOAD_CP_REG(x, y)			\
    do {								\
        pspl_gx_offline_add_u8(0x08);				\
        pspl_gx_offline_add_u8((u8)(x));			\
        pspl_gx_offline_add_u32((u32)(y));		\
    } while(0)

#define GX_LOAD_XF_REG(x, y)			\
    do {								\
        pspl_gx_offline_add_u8(0x10);				\
        pspl_gx_offline_add_u32((u32)((x)&0xffff));		\
        pspl_gx_offline_add_u32((u32)(y));		\
    } while(0)

#define GX_LOAD_XF_REGS(x, n)			\
    do {								\
        pspl_gx_offline_add_u8(0x10);				\
        pspl_gx_offline_add_u32((u32)(((((n)&0xffff)-1)<<16)|((x)&0xffff)));				\
    } while(0)

#define XY(x, y)   (((y) << 10) | (x))

#define GX_DEFAULT_BG	{64,64,64,255}
#define BLACK			{0,0,0,0}
#define WHITE			{255,255,255,255}


static u8 _gxtevcolid[9] = {0,1,0,1,0,1,7,5,6};




static s32 __gx_onreset(s32 final);



static void __GX_InitRevBits()
{
    s32 i;

    i=0;
    while(i<8) {
        __gx->VAT0reg[i] = 0x40000000;
        __gx->VAT1reg[i] = 0x80000000;
        GX_LOAD_CP_REG((0x0080|i),__gx->VAT1reg[i]);
        i++;
    }

    GX_LOAD_XF_REG(0x1000,0x3f);
    GX_LOAD_XF_REG(0x1012,0x01);

    GX_LOAD_BP_REG(0x5800000f);

}

static void __GX_FlushTextureState()
{
    GX_LOAD_BP_REG(__gx->tevIndMask);
}

static void __GX_XfVtxSpecs()
{
    u32 xfvtxspecs = 0;
    u32 nrms,texs,cols;

    cols = 0;
    if(__gx->vcdLo&0x6000) cols++;
    if(__gx->vcdLo&0x18000) cols++;

    nrms = 0;
    if(__gx->vcdNrms==1) nrms = 1;
    else if(__gx->vcdNrms==2) nrms = 2;

    texs = 0;
    if(__gx->vcdHi&0x3) texs++;
    if(__gx->vcdHi&0xc) texs++;
    if(__gx->vcdHi&0x30) texs++;
    if(__gx->vcdHi&0xc0) texs++;
    if(__gx->vcdHi&0x300) texs++;
    if(__gx->vcdHi&0xc00) texs++;
    if(__gx->vcdHi&0x3000) texs++;
    if(__gx->vcdHi&0xc000) texs++;

    xfvtxspecs = (_SHIFTL(texs,4,4))|(_SHIFTL(nrms,2,2))|(cols&0x3);
    GX_LOAD_XF_REG(0x1008,xfvtxspecs);
}

static void __GX_SetMatrixIndex(u32 mtx)
{
    if(mtx<5) {
        GX_LOAD_CP_REG(0x30,__gx->mtxIdxLo);
        GX_LOAD_XF_REG(0x1018,__gx->mtxIdxLo);
    } else {
        GX_LOAD_CP_REG(0x40,__gx->mtxIdxHi);
        GX_LOAD_XF_REG(0x1019,__gx->mtxIdxHi);
    }
}


static void __GX_SetVCD()
{
    GX_LOAD_CP_REG(0x50,__gx->vcdLo);
    GX_LOAD_CP_REG(0x60,__gx->vcdHi);
    __GX_XfVtxSpecs();
}

static void __GX_SetVAT()
{
    u8 setvtx = 0;
    s32 i;

    for(i=0;i<8;i++) {
        setvtx = (1<<i);
        if(__gx->VATTable&setvtx) {
            GX_LOAD_CP_REG((0x70+(i&7)),__gx->VAT0reg[i]);
            GX_LOAD_CP_REG((0x80+(i&7)),__gx->VAT1reg[i]);
            GX_LOAD_CP_REG((0x90+(i&7)),__gx->VAT2reg[i]);
        }
    }
    __gx->VATTable = 0;
}

static void __SetSURegs(u8 texmap,u8 texcoord)
{
    u32 reg;
    u16 wd,ht;
    u8 wrap_s,wrap_t;

    wd = __gx->texMapSize[texmap]&0x3ff;
    ht = _SHIFTR(__gx->texMapSize[texmap],10,10);
    wrap_s = __gx->texMapWrap[texmap]&3;
    wrap_t = _SHIFTR(__gx->texMapWrap[texmap],2,2);

    reg = (texcoord&0x7);
    __gx->suSsize[reg] = (__gx->suSsize[reg]&~0x0000ffff)|wd;
    __gx->suTsize[reg] = (__gx->suTsize[reg]&~0x0000ffff)|ht;
    __gx->suSsize[reg] = (__gx->suSsize[reg]&~0x00010000)|(_SHIFTL(wrap_s,16,1));
    __gx->suTsize[reg] = (__gx->suTsize[reg]&~0x00010000)|(_SHIFTL(wrap_t,16,1));

    GX_LOAD_BP_REG(__gx->suSsize[reg]);
    GX_LOAD_BP_REG(__gx->suTsize[reg]);
}

static void __GX_SetSUTexRegs()
{
    u32 i;
    u32 indtev,dirtev;
    u8 texcoord,texmap;
    u32 tevreg,tevm,texcm;

    dirtev = (_SHIFTR(__gx->genMode,10,4))+1;
    indtev = _SHIFTR(__gx->genMode,16,3);

    //indirect texture order
    for(i=0;i<indtev;i++) {
        switch(i) {
            case GX_INDTEXSTAGE0:
                texmap = __gx->tevRasOrder[2]&7;
                texcoord = _SHIFTR(__gx->tevRasOrder[2],3,3);
                break;
            case GX_INDTEXSTAGE1:
                texmap = _SHIFTR(__gx->tevRasOrder[2],6,3);
                texcoord = _SHIFTR(__gx->tevRasOrder[2],9,3);
                break;
            case GX_INDTEXSTAGE2:
                texmap = _SHIFTR(__gx->tevRasOrder[2],12,3);
                texcoord = _SHIFTR(__gx->tevRasOrder[2],15,3);
                break;
            case GX_INDTEXSTAGE3:
                texmap = _SHIFTR(__gx->tevRasOrder[2],18,3);
                texcoord = _SHIFTR(__gx->tevRasOrder[2],21,3);
                break;
            default:
                texmap = 0;
                texcoord = 0;
                break;
        }

        texcm = _SHIFTL(1,texcoord,1);
        if(!(__gx->texCoordManually&texcm))
            __SetSURegs(texmap,texcoord);
    }

    //direct texture order
    for(i=0;i<dirtev;i++) {
        tevreg = 3+(_SHIFTR(i,1,3));
        texmap = (__gx->tevTexMap[i]&0xff);

        if(i&1) texcoord = _SHIFTR(__gx->tevRasOrder[tevreg],15,3);
        else texcoord = _SHIFTR(__gx->tevRasOrder[tevreg],3,3);

        tevm = _SHIFTL(1,i,1);
        texcm = _SHIFTL(1,texcoord,1);
        if(texmap!=0xff && (__gx->tevTexCoordEnable&tevm) && !(__gx->texCoordManually&texcm)) {
            __SetSURegs(texmap,texcoord);
        }
    }
}

static void __GX_SetGenMode()
{
    GX_LOAD_BP_REG(__gx->genMode);
    __gx->xfFlush = 0;
}

static void __GX_UpdateBPMask()
{
#if defined(HW_DOL)
    u32 i;
    u32 nbmp,nres;
    u8 ntexmap;

    nbmp = _SHIFTR(__gx->genMode,16,3);

    nres = 0;
    for(i=0;i<nbmp;i++) {
        switch(i) {
            case GX_INDTEXSTAGE0:
                ntexmap = __gx->tevRasOrder[2]&7;
                break;
            case GX_INDTEXSTAGE1:
                ntexmap = _SHIFTR(__gx->tevRasOrder[2],6,3);
                break;
            case GX_INDTEXSTAGE2:
                ntexmap = _SHIFTR(__gx->tevRasOrder[2],12,3);
                break;
            case GX_INDTEXSTAGE3:
                ntexmap = _SHIFTR(__gx->tevRasOrder[2],18,3);
                break;
            default:
                ntexmap = 0;
                break;
        }
        nres |= (1<<ntexmap);
    }

    if((__gx->tevIndMask&0xff)!=nres) {
        __gx->tevIndMask = (__gx->tevIndMask&~0xff)|(nres&0xff);
        GX_LOAD_BP_REG(__gx->tevIndMask);
    }
#endif
}

static void __GX_SetIndirectMask(u32 mask)
{
    __gx->tevIndMask = ((__gx->tevIndMask&~0xff)|(mask&0xff));
    GX_LOAD_BP_REG(__gx->tevIndMask);
}

static void __GX_SetTexCoordGen()
{
    u32 i,mask;
    u32 texcoord;

    if(__gx->dirtyState&0x02000000) GX_LOAD_XF_REG(0x103f,(__gx->genMode&0xf));

    i = 0;
    texcoord = 0x1040;
    mask = _SHIFTR(__gx->dirtyState,16,8);
    while(mask) {
        if(mask&0x0001) {
            GX_LOAD_XF_REG(texcoord,__gx->texCoordGen[i]);
            GX_LOAD_XF_REG((texcoord+0x10),__gx->texCoordGen2[i]);
        }
        mask >>= 1;
        texcoord++;
        i++;
    }
}

static void __GX_SetChanColor()
{
    if(__gx->dirtyState&0x0100)
        GX_LOAD_XF_REG(0x100a,__gx->chnAmbColor[0]);
    if(__gx->dirtyState&0x0200)
        GX_LOAD_XF_REG(0x100b,__gx->chnAmbColor[1]);
    if(__gx->dirtyState&0x0400)
        GX_LOAD_XF_REG(0x100c,__gx->chnMatColor[0]);
    if(__gx->dirtyState&0x0800)
        GX_LOAD_XF_REG(0x100d,__gx->chnMatColor[1]);
}

static void __GX_SetChanCntrl()
{
    u32 i,chan,mask;

    if(__gx->dirtyState&0x01000000) GX_LOAD_XF_REG(0x1009,(_SHIFTR(__gx->genMode,4,3)));

    i = 0;
    chan = 0x100e;
    mask = _SHIFTR(__gx->dirtyState,12,4);
    while(mask) {
        if(mask&0x0001) GX_LOAD_XF_REG(chan,__gx->chnCntrl[i]);

        mask >>= 1;
        chan++;
        i++;
    }
}

static void __GX_SetDirtyState()
{
    if(__gx->dirtyState&0x0001) {
        __GX_SetSUTexRegs();
    }
    if(__gx->dirtyState&0x0002) {
        __GX_UpdateBPMask();
    }
    if(__gx->dirtyState&0x0004) {
        __GX_SetGenMode();
    }
    if(__gx->dirtyState&0x0008) {
        __GX_SetVCD();
    }
    if(__gx->dirtyState&0x0010) {
        __GX_SetVAT();
    }
    if(__gx->dirtyState&~0xff) {
        if(__gx->dirtyState&0x0f00) {
            __GX_SetChanColor();
        }
        if(__gx->dirtyState&0x0100f000) {
            __GX_SetChanCntrl();
        }
        if(__gx->dirtyState&0x02ff0000) {
            __GX_SetTexCoordGen();
        }
        if(__gx->dirtyState&0x04000000) {
            __GX_SetMatrixIndex(0);
            __GX_SetMatrixIndex(5);
        }
    }
    __gx->dirtyState = 0;
}

void GX_Flush()
{
    if(__gx->dirtyState)
        __GX_SetDirtyState();
    
    pspl_gx_offline_add_u32(0);
    pspl_gx_offline_add_u32(0);
    pspl_gx_offline_add_u32(0);
    pspl_gx_offline_add_u32(0);
    pspl_gx_offline_add_u32(0);
    pspl_gx_offline_add_u32(0);
    pspl_gx_offline_add_u32(0);
    pspl_gx_offline_add_u32(0);
    
}


void GX_SetChanCtrl(s32 channel,u8 enable,u8 ambsrc,u8 matsrc,u8 litmask,u8 diff_fn,u8 attn_fn)
{
    u32 reg,difffn = (attn_fn==GX_AF_SPEC)?GX_DF_NONE:diff_fn;
    u32 val = (matsrc&1)|(_SHIFTL(enable,1,1))|(_SHIFTL(litmask,2,4))|(_SHIFTL(ambsrc,6,1))|(_SHIFTL(difffn,7,2))|(_SHIFTL(((GX_AF_NONE-attn_fn)>0),9,1))|(_SHIFTL((attn_fn>0),10,1))|(_SHIFTL((_SHIFTR(litmask,4,4)),11,4));

    reg = (channel&0x03);
    __gx->chnCntrl[reg] = val;
    __gx->dirtyState |= (0x1000<<reg);

    if(channel==GX_COLOR0A0) {
        __gx->chnCntrl[2] = val;
        __gx->dirtyState |= 0x5000;
    } else {
        __gx->chnCntrl[3] = val;
        __gx->dirtyState |= 0xa000;
    }
}

void GX_SetChanAmbColor(s32 channel,GXColor color)
{
    u32 reg,val = (_SHIFTL(color.r,24,8))|(_SHIFTL(color.g,16,8))|(_SHIFTL(color.b,8,8))|0x00;
    switch(channel) {
        case GX_COLOR0:
            reg = 0;
            val |= (__gx->chnAmbColor[0]&0xff);
            break;
        case GX_COLOR1:
            reg = 1;
            val |= (__gx->chnAmbColor[1]&0xff);
            break;
        case GX_ALPHA0:
            reg = 0;
            val = ((__gx->chnAmbColor[0]&~0xff)|(color.a&0xff));
            break;
        case GX_ALPHA1:
            reg = 1;
            val = ((__gx->chnAmbColor[1]&~0xff)|(color.a&0xff));
            break;
        case GX_COLOR0A0:
            reg = 0;
            val |= (color.a&0xFF);
            break;
        case GX_COLOR1A1:
            reg = 1;
            val |= (color.a&0xFF);
            break;
        default:
            return;
    }

    __gx->chnAmbColor[reg] = val;
    __gx->dirtyState |= (0x0100<<reg);
}

void GX_SetChanMatColor(s32 channel,GXColor color)
{
    u32 reg,val = (_SHIFTL(color.r,24,8))|(_SHIFTL(color.g,16,8))|(_SHIFTL(color.b,8,8))|0x00;
    switch(channel) {
        case GX_COLOR0:
            reg = 0;
            val |= (__gx->chnMatColor[0]&0xff);
            break;
        case GX_COLOR1:
            reg = 1;
            val |= (__gx->chnMatColor[1]&0xff);
            break;
        case GX_ALPHA0:
            reg = 0;
            val = ((__gx->chnMatColor[0]&~0xff)|(color.a&0xff));
            break;
        case GX_ALPHA1:
            reg = 1;
            val = ((__gx->chnMatColor[1]&~0xff)|(color.a&0xff));
            break;
        case GX_COLOR0A0:
            reg = 0;
            val |= (color.a&0xFF);
            break;
        case GX_COLOR1A1:
            reg = 1;
            val |= (color.a&0xFF);
            break;
        default:
            return;
    }

    __gx->chnMatColor[reg] = val;
    __gx->dirtyState |= (0x0400<<reg);
}

void GX_SetTexCoordGen(u16 texcoord,u32 tgen_typ,u32 tgen_src,u32 mtxsrc)
{
        GX_SetTexCoordGen2(texcoord,tgen_typ,tgen_src,mtxsrc,GX_FALSE,GX_DTTIDENTITY);
}

void GX_SetTexCoordGen2(u16 texcoord,u32 tgen_typ,u32 tgen_src,u32 mtxsrc,u32 normalize,u32 postmtx)
{
    u32 txc;
    u32 texcoords;
    u8 vtxrow,stq;

    if(texcoord>=GX_MAXCOORD) return;

    stq = 0;
    switch(tgen_src) {
        case GX_TG_POS:
            vtxrow = 0;
            stq = 1;
            break;
        case GX_TG_NRM:
            vtxrow = 1;
            stq = 1;
            break;
        case GX_TG_BINRM:
            vtxrow = 3;
            stq = 1;
            break;
        case GX_TG_TANGENT:
            vtxrow = 4;
            stq = 1;
            break;
        case GX_TG_COLOR0:
            vtxrow = 2;
            break;
        case GX_TG_COLOR1:
            vtxrow = 2;
            break;
        case GX_TG_TEX0:
            vtxrow = 5;
            break;
        case GX_TG_TEX1:
            vtxrow = 6;
            break;
        case GX_TG_TEX2:
            vtxrow = 7;
            break;
        case GX_TG_TEX3:
            vtxrow = 8;
            break;
        case GX_TG_TEX4:
            vtxrow = 9;
            break;
        case GX_TG_TEX5:
            vtxrow = 10;
            break;
        case GX_TG_TEX6:
            vtxrow = 11;
            break;
        case GX_TG_TEX7:
            vtxrow = 12;
            break;
        default:
            vtxrow = 5;
            break;
    }

    texcoords = 0;
    txc = (texcoord&7);
    if((tgen_typ==GX_TG_MTX3x4 || tgen_typ==GX_TG_MTX2x4))
    {
        if(tgen_typ==GX_TG_MTX3x4) texcoords = 0x02;

        texcoords |= (_SHIFTL(stq,2,1));
        texcoords |= (_SHIFTL(vtxrow,7,5));
    } else if((tgen_typ>=GX_TG_BUMP0 && tgen_typ<=GX_TG_BUMP7))
    {
        tgen_src -= GX_TG_TEXCOORD0;
        tgen_typ -= GX_TG_BUMP0;

        texcoords = 0x10;
        texcoords |= (_SHIFTL(stq,2,1));
        texcoords |= (_SHIFTL(vtxrow,7,5));
        texcoords |= (_SHIFTL(tgen_src,12,3));
        texcoords |= (_SHIFTL(tgen_typ,15,3));
    } else if(tgen_typ==GX_TG_SRTG) {
        if(tgen_src==GX_TG_COLOR0) texcoords = 0x20;
        else if(tgen_src==GX_TG_COLOR1) texcoords = 0x30;
        texcoords |= (_SHIFTL(stq,2,1));
        texcoords |= (_SHIFTL(2,7,5));
    }

    postmtx -= GX_DTTMTX0;
    __gx->texCoordGen[txc] = texcoords;
    __gx->texCoordGen2[txc] = ((_SHIFTL(normalize,8,1))|(postmtx&0x3f));

    switch(texcoord) {
        case GX_TEXCOORD0:
            __gx->mtxIdxLo = (__gx->mtxIdxLo&~0xfc0)|(_SHIFTL(mtxsrc,6,6));
            break;
        case GX_TEXCOORD1:
            __gx->mtxIdxLo = (__gx->mtxIdxLo&~0x3f000)|(_SHIFTL(mtxsrc,12,6));
            break;
        case GX_TEXCOORD2:
            __gx->mtxIdxLo = (__gx->mtxIdxLo&~0xfc0000)|(_SHIFTL(mtxsrc,18,6));
            break;
        case GX_TEXCOORD3:
            __gx->mtxIdxLo = (__gx->mtxIdxLo&~0x3f000000)|(_SHIFTL(mtxsrc,24,6));
            break;
        case GX_TEXCOORD4:
            __gx->mtxIdxHi = (__gx->mtxIdxHi&~0x3f)|(mtxsrc&0x3f);
            break;
        case GX_TEXCOORD5:
            __gx->mtxIdxHi = (__gx->mtxIdxHi&~0xfc0)|(_SHIFTL(mtxsrc,6,6));
            break;
        case GX_TEXCOORD6:
            __gx->mtxIdxHi = (__gx->mtxIdxHi&~0x3f000)|(_SHIFTL(mtxsrc,12,6));
            break;
        case GX_TEXCOORD7:
            __gx->mtxIdxHi = (__gx->mtxIdxHi&~0xfc0000)|(_SHIFTL(mtxsrc,18,6));
            break;
    }
    __gx->dirtyState |= (0x04000000|(0x00010000<<texcoord));
}


void GX_SetCurrentMtx(u32 mtx)
{
    __gx->mtxIdxLo = (__gx->mtxIdxLo&~0x3f)|(mtx&0x3f);
    __gx->dirtyState |= 0x04000000;
}

void GX_SetNumTexGens(u32 nr)
{
    __gx->genMode = (__gx->genMode&~0xf)|(nr&0xf);
    __gx->dirtyState |= 0x02000004;
}

void GX_SetZMode(u8 enable,u8 func,u8 update_enable)
{
    __gx->peZMode = (__gx->peZMode&~0x1)|(enable&1);
    __gx->peZMode = (__gx->peZMode&~0xe)|(_SHIFTL(func,1,3));
    __gx->peZMode = (__gx->peZMode&~0x10)|(_SHIFTL(update_enable,4,1));
    GX_LOAD_BP_REG(__gx->peZMode);
}

void GX_SetTexCoordScaleManually(u8 texcoord,u8 enable,u16 ss,u16 ts)
{
    u32 reg;

    __gx->texCoordManually = (__gx->texCoordManually&~(_SHIFTL(1,texcoord,1)))|(_SHIFTL(enable,texcoord,1));
    if(!enable) return;

    reg = (texcoord&0x7);
    __gx->suSsize[reg] = (__gx->suSsize[reg]&~0xffff)|((ss-1)&0xffff);
    __gx->suTsize[reg] = (__gx->suTsize[reg]&~0xffff)|((ts-1)&0xffff);

    GX_LOAD_BP_REG(__gx->suSsize[reg]);
    GX_LOAD_BP_REG(__gx->suTsize[reg]);
}

void GX_SetTexCoordCylWrap(u8 texcoord,u8 s_enable,u8 t_enable)
{
    u32 reg;

    reg = (texcoord&0x7);
    __gx->suSsize[reg] = (__gx->suSsize[reg]&~0x20000)|(_SHIFTL(s_enable,17,1));
    __gx->suTsize[reg] = (__gx->suTsize[reg]&~0x20000)|(_SHIFTL(t_enable,17,1));

    if(!(__gx->texCoordManually&(_SHIFTL(1,texcoord,1)))) return;

    GX_LOAD_BP_REG(__gx->suSsize[reg]);
    GX_LOAD_BP_REG(__gx->suTsize[reg]);
}

void GX_SetTexCoordBias(u8 texcoord,u8 s_enable,u8 t_enable)
{
    u32 reg;

    reg = (texcoord&0x7);
    __gx->suSsize[reg] = (__gx->suSsize[reg]&~0x10000)|(_SHIFTL(s_enable,16,1));
    __gx->suTsize[reg] = (__gx->suTsize[reg]&~0x10000)|(_SHIFTL(t_enable,16,1));

    if(!(__gx->texCoordManually&(_SHIFTL(1,texcoord,1)))) return;

    GX_LOAD_BP_REG(__gx->suSsize[reg]);
    GX_LOAD_BP_REG(__gx->suTsize[reg]);
}

void GX_SetBlendMode(u8 type,u8 src_fact,u8 dst_fact,u8 op)
{
    __gx->peCMode0 = (__gx->peCMode0&~0x1);
    if(type==GX_BM_BLEND || type==GX_BM_SUBTRACT) __gx->peCMode0 |= 0x1;

    __gx->peCMode0 = (__gx->peCMode0&~0x800);
    if(type==GX_BM_SUBTRACT) __gx->peCMode0 |= 0x800;

    __gx->peCMode0 = (__gx->peCMode0&~0x2);
    if(type==GX_BM_LOGIC) __gx->peCMode0 |= 0x2;

    __gx->peCMode0 = (__gx->peCMode0&~0xF000)|(_SHIFTL(op,12,4));
    __gx->peCMode0 = (__gx->peCMode0&~0xE0)|(_SHIFTL(dst_fact,5,3));
    __gx->peCMode0 = (__gx->peCMode0&~0x700)|(_SHIFTL(src_fact,8,3));

    GX_LOAD_BP_REG(__gx->peCMode0);
}

void GX_ClearVtxDesc()
{
    __gx->vcdNrms = 0;
    __gx->vcdClear = ((__gx->vcdClear&~0x0600)|0x0200);
    __gx->vcdLo = __gx->vcdHi = 0;
    __gx->dirtyState |= 0x0008;
}

void GX_SetLineWidth(u8 width,u8 fmt)
{
    __gx->lpWidth = (__gx->lpWidth&~0xff)|(width&0xff);
    __gx->lpWidth = (__gx->lpWidth&~0x70000)|(_SHIFTL(fmt,16,3));
    GX_LOAD_BP_REG(__gx->lpWidth);
}

void GX_SetPointSize(u8 width,u8 fmt)
{
    __gx->lpWidth = (__gx->lpWidth&~0xFF00)|(_SHIFTL(width,8,8));
    __gx->lpWidth = (__gx->lpWidth&~0x380000)|(_SHIFTL(fmt,19,3));
    GX_LOAD_BP_REG(__gx->lpWidth);
}

void GX_SetTevColor(u8 tev_regid,GXColor color)
{
    u32 reg;

    reg = (_SHIFTL((0xe0+(tev_regid<<1)),24,8)|(_SHIFTL(color.a,12,8))|(color.r&0xff));
    GX_LOAD_BP_REG(reg);

    reg = (_SHIFTL((0xe1+(tev_regid<<1)),24,8)|(_SHIFTL(color.g,12,8))|(color.b&0xff));
    GX_LOAD_BP_REG(reg);

    //this two calls should obviously flush the Write Gather Pipe.
    GX_LOAD_BP_REG(reg);
    GX_LOAD_BP_REG(reg);
}

void GX_SetTevColorS10(u8 tev_regid,GXColorS10 color)
{
    u32 reg;

    reg = (_SHIFTL((0xe0+(tev_regid<<1)),24,8)|(_SHIFTL(color.a,12,11))|(color.r&0x7ff));
    GX_LOAD_BP_REG(reg);

    reg = (_SHIFTL((0xe1+(tev_regid<<1)),24,8)|(_SHIFTL(color.g,12,11))|(color.b&0x7ff));
    GX_LOAD_BP_REG(reg);

    //this two calls should obviously flush the Write Gather Pipe.
    GX_LOAD_BP_REG(reg);
    GX_LOAD_BP_REG(reg);
}

void GX_SetTevKColor(u8 tev_kregid,GXColor color)
{
    u32 reg;

    reg = (_SHIFTL((0xe0+(tev_kregid<<1)),24,8)|(_SHIFTL(1,23,1))|(_SHIFTL(color.a,12,8))|(color.r&0xff));
    GX_LOAD_BP_REG(reg);

    reg = (_SHIFTL((0xe1+(tev_kregid<<1)),24,8)|(_SHIFTL(1,23,1))|(_SHIFTL(color.g,12,8))|(color.b&0xff));
    GX_LOAD_BP_REG(reg);

    //this two calls should obviously flush the Write Gather Pipe.
    GX_LOAD_BP_REG(reg);
    GX_LOAD_BP_REG(reg);
}

void GX_SetTevKColorS10(u8 tev_kregid,GXColorS10 color)
{
    u32 reg;

    reg = (_SHIFTL((0xe0+(tev_kregid<<1)),24,8)|(_SHIFTL(1,23,1))|(_SHIFTL(color.a,12,11))|(color.r&0x7ff));
    GX_LOAD_BP_REG(reg);

    reg = (_SHIFTL((0xe1+(tev_kregid<<1)),24,8)|(_SHIFTL(1,23,1))|(_SHIFTL(color.g,12,11))|(color.b&0x7ff));
    GX_LOAD_BP_REG(reg);

    //this two calls should obviously flush the Write Gather Pipe.
    GX_LOAD_BP_REG(reg);
    GX_LOAD_BP_REG(reg);
}

void GX_SetTevOp(u8 tevstage,u8 mode)
{
    u8 defcolor = GX_CC_RASC;
    u8 defalpha = GX_CA_RASA;

    if(tevstage!=GX_TEVSTAGE0) {
        defcolor = GX_CC_CPREV;
        defalpha = GX_CA_APREV;
    }

    switch(mode) {
        case GX_MODULATE:
            GX_SetTevColorIn(tevstage,GX_CC_ZERO,GX_CC_TEXC,defcolor,GX_CC_ZERO);
            GX_SetTevAlphaIn(tevstage,GX_CA_ZERO,GX_CA_TEXA,defalpha,GX_CA_ZERO);
            break;
        case GX_DECAL:
            GX_SetTevColorIn(tevstage,defcolor,GX_CC_TEXC,GX_CC_TEXA,GX_CC_ZERO);
            GX_SetTevAlphaIn(tevstage,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,defalpha);
            break;
        case GX_BLEND:
            GX_SetTevColorIn(tevstage,defcolor,GX_CC_ONE,GX_CC_TEXC,GX_CC_ZERO);
            GX_SetTevAlphaIn(tevstage,GX_CA_ZERO,GX_CA_TEXA,defalpha,GX_CA_RASA);
            break;
        case GX_REPLACE:
            GX_SetTevColorIn(tevstage,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_TEXC);
            GX_SetTevAlphaIn(tevstage,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,GX_CA_TEXA);
            break;
        case GX_PASSCLR:
            GX_SetTevColorIn(tevstage,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,defcolor);
            GX_SetTevAlphaIn(tevstage,GX_CC_A2,GX_CC_A2,GX_CC_A2,defalpha);
            break;
    }
    GX_SetTevColorOp(tevstage,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
    GX_SetTevAlphaOp(tevstage,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
}

void GX_SetTevColorIn(u8 tevstage,u8 a,u8 b,u8 c,u8 d)
{
    u32 reg = (tevstage&0xf);
    __gx->tevColorEnv[reg] = (__gx->tevColorEnv[reg]&~0xF000)|(_SHIFTL(a,12,4));
    __gx->tevColorEnv[reg] = (__gx->tevColorEnv[reg]&~0xF00)|(_SHIFTL(b,8,4));
    __gx->tevColorEnv[reg] = (__gx->tevColorEnv[reg]&~0xF0)|(_SHIFTL(c,4,4));
    __gx->tevColorEnv[reg] = (__gx->tevColorEnv[reg]&~0xf)|(d&0xf);

    GX_LOAD_BP_REG(__gx->tevColorEnv[reg]);
}

void GX_SetTevAlphaIn(u8 tevstage,u8 a,u8 b,u8 c,u8 d)
{
    u32 reg = (tevstage&0xf);
    __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0xE000)|(_SHIFTL(a,13,3));
    __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0x1C00)|(_SHIFTL(b,10,3));
    __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0x380)|(_SHIFTL(c,7,3));
    __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0x70)|(_SHIFTL(d,4,3));

    GX_LOAD_BP_REG(__gx->tevAlphaEnv[reg]);
}

void GX_SetTevColorOp(u8 tevstage,u8 tevop,u8 tevbias,u8 tevscale,u8 clamp,u8 tevregid)
{
    /* set tev op add/sub*/
    u32 reg = (tevstage&0xf);
    __gx->tevColorEnv[reg] = (__gx->tevColorEnv[reg]&~0x40000)|(_SHIFTL(tevop,18,1));
    if(tevop<=GX_TEV_SUB) {
        __gx->tevColorEnv[reg] = (__gx->tevColorEnv[reg]&~0x300000)|(_SHIFTL(tevscale,20,2));
        __gx->tevColorEnv[reg] = (__gx->tevColorEnv[reg]&~0x30000)|(_SHIFTL(tevbias,16,2));
    } else {
        __gx->tevColorEnv[reg] = (__gx->tevColorEnv[reg]&~0x300000)|((_SHIFTL(tevop,19,4))&0x300000);
        __gx->tevColorEnv[reg] = (__gx->tevColorEnv[reg]&~0x30000)|0x30000;
    }
    __gx->tevColorEnv[reg] = (__gx->tevColorEnv[reg]&~0x80000)|(_SHIFTL(clamp,19,1));
    __gx->tevColorEnv[reg] = (__gx->tevColorEnv[reg]&~0xC00000)|(_SHIFTL(tevregid,22,2));

    GX_LOAD_BP_REG(__gx->tevColorEnv[reg]);
}

void GX_SetTevAlphaOp(u8 tevstage,u8 tevop,u8 tevbias,u8 tevscale,u8 clamp,u8 tevregid)
{
    /* set tev op add/sub*/
    u32 reg = (tevstage&0xf);
    __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0x40000)|(_SHIFTL(tevop,18,1));
    if(tevop<=GX_TEV_SUB) {
        __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0x300000)|(_SHIFTL(tevscale,20,2));
        __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0x30000)|(_SHIFTL(tevbias,16,2));
    } else {
        __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0x300000)|((_SHIFTL(tevop,19,4))&0x300000);
        __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0x30000)|0x30000;
    }
    __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0x80000)|(_SHIFTL(clamp,19,1));
    __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0xC00000)|(_SHIFTL(tevregid,22,2));

    GX_LOAD_BP_REG(__gx->tevAlphaEnv[reg]);
}

void GX_SetCullMode(u8 mode)
{
    static u8 cm2hw[] = { 0, 2, 1, 3 };

    __gx->genMode = (__gx->genMode&~0xC000)|(_SHIFTL(cm2hw[mode],14,2));
    __gx->dirtyState |= 0x0004;
}

void GX_SetCoPlanar(u8 enable)
{
    __gx->genMode = (__gx->genMode&~0x80000)|(_SHIFTL(enable,19,1));
    GX_LOAD_BP_REG(0xFE080000);
    GX_LOAD_BP_REG(__gx->genMode);
}

void GX_EnableTexOffsets(u8 coord,u8 line_enable,u8 point_enable)
{
    u32 reg = (coord&0x7);
    __gx->suSsize[reg] = (__gx->suSsize[reg]&~0x40000)|(_SHIFTL(line_enable,18,1));
    __gx->suSsize[reg] = (__gx->suSsize[reg]&~0x80000)|(_SHIFTL(point_enable,19,1));
    GX_LOAD_BP_REG(__gx->suSsize[reg]);
}

void GX_SetNumChans(u8 num)
{
    __gx->genMode = (__gx->genMode&~0x70)|(_SHIFTL(num,4,3));
    __gx->dirtyState |= 0x01000004;
}

void GX_SetTevOrder(u8 tevstage,u8 texcoord,u32 texmap,u8 color)
{
    u8 colid;
    u32 texm,texc,tmp;
    u32 reg = 3+(_SHIFTR(tevstage,1,3));

    __gx->tevTexMap[(tevstage&0xf)] = texmap;

    texm = (texmap&~0x100);
    if(texm>=GX_MAX_TEXMAP) texm = 0;
    if(texcoord>=GX_MAXCOORD) {
        texc = 0;
        __gx->tevTexCoordEnable &= ~(_SHIFTL(1,tevstage,1));
    } else {
        texc = texcoord;
        __gx->tevTexCoordEnable |= (_SHIFTL(1,tevstage,1));
    }

    if(tevstage&1) {
        __gx->tevRasOrder[reg] = (__gx->tevRasOrder[reg]&~0x7000)|(_SHIFTL(texm,12,3));
        __gx->tevRasOrder[reg] = (__gx->tevRasOrder[reg]&~0x38000)|(_SHIFTL(texc,15,3));

        colid = GX_ALPHA_BUMP;
        if(color!=GX_COLORNULL) colid = _gxtevcolid[color];
        __gx->tevRasOrder[reg] = (__gx->tevRasOrder[reg]&~0x380000)|(_SHIFTL(colid,19,3));

        tmp = 1;
        if(texmap==GX_TEXMAP_NULL || texmap&0x100) tmp = 0;
        __gx->tevRasOrder[reg] = (__gx->tevRasOrder[reg]&~0x40000)|(_SHIFTL(tmp,18,1));
    } else {
        __gx->tevRasOrder[reg] = (__gx->tevRasOrder[reg]&~0x7)|(texm&0x7);
        __gx->tevRasOrder[reg] = (__gx->tevRasOrder[reg]&~0x38)|(_SHIFTL(texc,3,3));

        colid = GX_ALPHA_BUMP;
        if(color!=GX_COLORNULL) colid = _gxtevcolid[color];
        __gx->tevRasOrder[reg] = (__gx->tevRasOrder[reg]&~0x380)|(_SHIFTL(colid,7,3));

        tmp = 1;
        if(texmap==GX_TEXMAP_NULL || texmap&0x100) tmp = 0;
        __gx->tevRasOrder[reg] = (__gx->tevRasOrder[reg]&~0x40)|(_SHIFTL(tmp,6,1));
    }
    GX_LOAD_BP_REG(__gx->tevRasOrder[reg]);
    __gx->dirtyState |= 0x0001;
}

void GX_SetNumTevStages(u8 num)
{
    __gx->genMode = (__gx->genMode&~0x3C00)|(_SHIFTL((num-1),10,4));
    __gx->dirtyState |= 0x0004;
}

void GX_SetAlphaCompare(u8 comp0,u8 ref0,u8 aop,u8 comp1,u8 ref1)
{
    u32 val = 0;
    val = (_SHIFTL(aop,22,2))|(_SHIFTL(comp1,19,3))|(_SHIFTL(comp0,16,3))|(_SHIFTL(ref1,8,8))|(ref0&0xff);
    GX_LOAD_BP_REG(0xf3000000|val);
}

void GX_SetTevKColorSel(u8 tevstage,u8 sel)
{
    u32 reg = (_SHIFTR(tevstage,1,3));

    if(tevstage&1)
        __gx->tevSwapModeTable[reg] = (__gx->tevSwapModeTable[reg]&~0x7C000)|(_SHIFTL(sel,14,5));
    else
        __gx->tevSwapModeTable[reg] = (__gx->tevSwapModeTable[reg]&~0x1F0)|(_SHIFTL(sel,4,5));
    GX_LOAD_BP_REG(__gx->tevSwapModeTable[reg]);
}

void GX_SetTevKAlphaSel(u8 tevstage,u8 sel)
{
    u32 reg = (_SHIFTR(tevstage,1,3));

    if(tevstage&1)
        __gx->tevSwapModeTable[reg] = (__gx->tevSwapModeTable[reg]&~0xF80000)|(_SHIFTL(sel,19,5));
    else
        __gx->tevSwapModeTable[reg] = (__gx->tevSwapModeTable[reg]&~0x3E00)|(_SHIFTL(sel,9,5));
    GX_LOAD_BP_REG(__gx->tevSwapModeTable[reg]);
}

void GX_SetTevSwapMode(u8 tevstage,u8 ras_sel,u8 tex_sel)
{
    u32 reg = (tevstage&0xf);
    __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0x3)|(ras_sel&0x3);
    __gx->tevAlphaEnv[reg] = (__gx->tevAlphaEnv[reg]&~0xC)|(_SHIFTL(tex_sel,2,2));
    GX_LOAD_BP_REG(__gx->tevAlphaEnv[reg]);
}

void GX_SetTevSwapModeTable(u8 swapid,u8 r,u8 g,u8 b,u8 a)
{
    u32 regA = 0+(_SHIFTL(swapid,1,3));
    u32 regB = 1+(_SHIFTL(swapid,1,3));

    __gx->tevSwapModeTable[regA] = (__gx->tevSwapModeTable[regA]&~0x3)|(r&0x3);
    __gx->tevSwapModeTable[regA] = (__gx->tevSwapModeTable[regA]&~0xC)|(_SHIFTL(g,2,2));
    GX_LOAD_BP_REG(__gx->tevSwapModeTable[regA]);

    __gx->tevSwapModeTable[regB] = (__gx->tevSwapModeTable[regB]&~0x3)|(b&0x3);
    __gx->tevSwapModeTable[regB] = (__gx->tevSwapModeTable[regB]&~0xC)|(_SHIFTL(a,2,2));
    GX_LOAD_BP_REG(__gx->tevSwapModeTable[regB]);
}

void GX_SetTevIndirect(u8 tevstage,u8 indtexid,u8 format,u8 bias,u8 mtxid,u8 wrap_s,u8 wrap_t,u8 addprev,u8 utclod,u8 a)
{
    u32 val = (0x10000000|(_SHIFTL(tevstage,24,4)))|(indtexid&3)|(_SHIFTL(format,2,2))|(_SHIFTL(bias,4,3))|(_SHIFTL(a,7,2))|(_SHIFTL(mtxid,9,4))|(_SHIFTL(wrap_s,13,3))|(_SHIFTL(wrap_t,16,3))|(_SHIFTL(utclod,19,1))|(_SHIFTL(addprev,20,1));
    GX_LOAD_BP_REG(val);
}

void GX_SetTevDirect(u8 tevstage)
{
    GX_SetTevIndirect(tevstage,GX_INDTEXSTAGE0,GX_ITF_8,GX_ITB_NONE,GX_ITM_OFF,GX_ITW_OFF,GX_ITW_OFF,GX_FALSE,GX_FALSE,GX_ITBA_OFF);
}

void GX_SetNumIndStages(u8 nstages)
{
    __gx->genMode = (__gx->genMode&~0x70000)|(_SHIFTL(nstages,16,3));
    __gx->dirtyState |= 0x0006;
}

void GX_SetIndTexMatrix(u8 indtexmtx,f32 offset_mtx[2][3],s8 scale_exp)
{
    u32 ma,mb;
    u32 val,s,idx;

    if(indtexmtx>0x00 && indtexmtx<0x04) indtexmtx -= 0x01;
    else if(indtexmtx>0x04 && indtexmtx<0x08) indtexmtx -= 0x05;
    else if(indtexmtx>0x08 && indtexmtx<0x0C) indtexmtx -= 0x09;
    else indtexmtx = 0x00;

    s = (scale_exp+17);
    idx = ((indtexmtx<<2)-indtexmtx);

    ma = (u32)(offset_mtx[0][0]*1024.0F);
    mb = (u32)(offset_mtx[1][0]*1024.0F);
    val = (_SHIFTL((0x06+idx),24,8)|_SHIFTL(s,22,2)|_SHIFTL(mb,11,11)|_SHIFTL(ma,0,11));
    GX_LOAD_BP_REG(val);

    ma = (u32)(offset_mtx[0][1]*1024.0F);
    mb = (u32)(offset_mtx[1][1]*1024.0F);
    val = (_SHIFTL((0x07+idx),24,8)|_SHIFTL((s>>2),22,2)|_SHIFTL(mb,11,11)|_SHIFTL(ma,0,11));
    GX_LOAD_BP_REG(val);

    ma = (u32)(offset_mtx[0][2]*1024.0F);
    mb = (u32)(offset_mtx[1][2]*1024.0F);
    val = (_SHIFTL((0x08+idx),24,8)|_SHIFTL((s>>4),22,2)|_SHIFTL(mb,11,11)|_SHIFTL(ma,0,11));
    GX_LOAD_BP_REG(val);
}

void GX_SetTevIndBumpST(u8 tevstage,u8 indstage,u8 mtx_sel)
{
    u8 sel_s,sel_t;

    switch(mtx_sel) {
        case GX_ITM_0:
            sel_s = GX_ITM_S0;
            sel_t = GX_ITM_T0;
            break;
        case GX_ITM_1:
            sel_s = GX_ITM_S1;
            sel_t = GX_ITM_T1;
            break;
        case GX_ITM_2:
            sel_s = GX_ITM_S2;
            sel_t = GX_ITM_T2;
            break;
        default:
            sel_s = GX_ITM_OFF;
            sel_t = GX_ITM_OFF;
            break;
    }

    GX_SetTevIndirect((tevstage+0),indstage,GX_ITF_8,GX_ITB_ST,sel_s,GX_ITW_0,GX_ITW_0,GX_FALSE,GX_FALSE,GX_ITBA_OFF);
    GX_SetTevIndirect((tevstage+1),indstage,GX_ITF_8,GX_ITB_ST,sel_t,GX_ITW_0,GX_ITW_0,GX_TRUE,GX_FALSE,GX_ITBA_OFF);
    GX_SetTevIndirect((tevstage+2),indstage,GX_ITF_8,GX_ITB_NONE,GX_ITM_OFF,GX_ITW_OFF,GX_ITW_OFF,GX_TRUE,GX_FALSE,GX_ITBA_OFF);
}

void GX_SetTevIndBumpXYZ(u8 tevstage,u8 indstage,u8 mtx_sel)
{
    GX_SetTevIndirect(tevstage,indstage,GX_ITF_8,GX_ITB_STU,mtx_sel,GX_ITW_OFF,GX_ITW_OFF,GX_FALSE,GX_FALSE,GX_ITBA_OFF);
}

void GX_SetTevIndRepeat(u8 tevstage)
{
    GX_SetTevIndirect(tevstage,GX_INDTEXSTAGE0,GX_ITF_8,GX_ITB_NONE,GX_ITM_OFF,GX_ITW_0,GX_ITW_0,GX_TRUE,GX_FALSE,GX_ITBA_OFF);
}

void GX_SetIndTexCoordScale(u8 indtexid,u8 scale_s,u8 scale_t)
{
    switch(indtexid) {
        case GX_INDTEXSTAGE0:
            __gx->tevRasOrder[0] = (__gx->tevRasOrder[0]&~0x0f)|(scale_s&0x0f);
            __gx->tevRasOrder[0] = (__gx->tevRasOrder[0]&~0xF0)|(_SHIFTL(scale_t,4,4));
            GX_LOAD_BP_REG(__gx->tevRasOrder[0]);
            break;
        case GX_INDTEXSTAGE1:
            __gx->tevRasOrder[0] = (__gx->tevRasOrder[0]&~0xF00)|(_SHIFTL(scale_s,8,4));
            __gx->tevRasOrder[0] = (__gx->tevRasOrder[0]&~0xF000)|(_SHIFTL(scale_t,12,4));
            GX_LOAD_BP_REG(__gx->tevRasOrder[0]);
            break;
        case GX_INDTEXSTAGE2:
            __gx->tevRasOrder[1] = (__gx->tevRasOrder[1]&~0x0f)|(scale_s&0x0f);
            __gx->tevRasOrder[1] = (__gx->tevRasOrder[1]&~0xF0)|(_SHIFTL(scale_t,4,4));
            GX_LOAD_BP_REG(__gx->tevRasOrder[1]);
            break;
        case GX_INDTEXSTAGE3:
            __gx->tevRasOrder[1] = (__gx->tevRasOrder[1]&~0xF00)|(_SHIFTL(scale_s,8,4));
            __gx->tevRasOrder[1] = (__gx->tevRasOrder[1]&~0xF000)|(_SHIFTL(scale_t,12,4));
            GX_LOAD_BP_REG(__gx->tevRasOrder[1]);
            break;
    }
}

void GX_SetTevIndTile(u8 tevstage,u8 indtexid,u16 tilesize_x,u16 tilesize_y,u16 tilespacing_x,u16 tilespacing_y,u8 indtexfmt,u8 indtexmtx,u8 bias_sel,u8 alpha_sel)
{
    s32 wrap_s,wrap_t;
    f32 offset_mtx[2][3];
    f64 fdspace_x,fdspace_y;
    u32 fbuf_x[2] = { 0x43300000,tilespacing_x };
    u32 fbuf_y[2] = { 0x43300000,tilespacing_y };

    wrap_s = GX_ITW_OFF;
    if(tilesize_x==0x0010) wrap_s = GX_ITW_16;
    else if(tilesize_x==0x0020) wrap_s = GX_ITW_32;
    else if(tilesize_x==0x0040) wrap_s = GX_ITW_64;
    else if(tilesize_x==0x0080) wrap_s = GX_ITW_128;
    else if(tilesize_x==0x0100) wrap_s = GX_ITW_256;

    wrap_t = GX_ITW_OFF;
    if(tilesize_y==0x0010) wrap_t = GX_ITW_16;
    else if(tilesize_y==0x0020) wrap_t = GX_ITW_32;
    else if(tilesize_y==0x0040) wrap_t = GX_ITW_64;
    else if(tilesize_y==0x0080) wrap_t = GX_ITW_128;
    else if(tilesize_y==0x0100) wrap_t = GX_ITW_256;

    fdspace_x = *(f64*)((void*)fbuf_x);
    fdspace_y = *(f64*)((void*)fbuf_y);

    offset_mtx[0][0] = (f32)((fdspace_x - 4503599627370496.0F)*0.00097656250F);
    offset_mtx[0][1] = 0.0F;
    offset_mtx[0][2] = 0.0F;
    offset_mtx[1][0] = 0.0F;
    offset_mtx[1][1] = (f32)((fdspace_y - 4503599627370496.0F)*0.00097656250F);
    offset_mtx[1][2] = 0.0F;

    GX_SetIndTexMatrix(indtexmtx,offset_mtx,10);
    GX_SetTevIndirect(tevstage,indtexid,indtexfmt,bias_sel,indtexmtx,wrap_s,wrap_t,GX_FALSE,GX_TRUE,alpha_sel);
}

void GX_SetFog(u8 type,f32 startz,f32 endz,f32 nearz,f32 farz,GXColor col)
{
    f32 A, B, B_mant, C, A_f;
    u32 b_expn, b_m, a_hex, c_hex,val,proj = 0;
    union ieee32 { f32 f; u32 i; } v;

    proj = _SHIFTR(type,3,1);

    // Calculate constants a, b, and c (TEV HW requirements).
    if(proj) { // Orthographic Fog Type
        if((farz==nearz) || (endz==startz)) {
            // take care of the odd-ball case.
            A_f = 0.0f;
            C = 0.0f;
        } else {
            A = 1.0f/(endz-startz);
            A_f = (farz-nearz) * A;
            C = (startz-nearz) * A;
        }

        b_expn	= 0;
        b_m		= 0;
    } else { // Perspective Fog Type
      // Calculate constants a, b, and c (TEV HW requirements).
        if((farz==nearz) || (endz==startz)) {
            // take care of the odd-ball case.
            A = 0.0f;
            B = 0.5f;
            C = 0.0f;
        } else {
            A = (farz*nearz)/((farz-nearz)*(endz-startz));
            B = farz/(farz-nearz);
            C = startz/(endz-startz);
        }

        B_mant = B;
        b_expn = 1;
        while(B_mant>1.0f) {
            B_mant /= 2.0f;
            b_expn++;
        }

        while((B_mant>0.0f) && (B_mant<0.5f)) {
            B_mant *= 2.0f;
            b_expn--;
        }

        A_f   = A/(1<<(b_expn));
        b_m   = (u32)(B_mant * 8388638.0f);
    }
    v.f = A_f;
    a_hex = v.i;

    v.f = C;
    c_hex = v.i;

    val = 0xee000000|(_SHIFTR(a_hex,12,20));
    GX_LOAD_BP_REG(val);

    val = 0xef000000|(b_m&0x00ffffff);
    GX_LOAD_BP_REG(val);

    val = 0xf0000000|(b_expn&0x1f);
    GX_LOAD_BP_REG(val);

    val = 0xf1000000|(_SHIFTL(type,21,3))|(_SHIFTL(proj,20,1))|(_SHIFTR(c_hex,12,20));
    GX_LOAD_BP_REG(val);

    val = 0xf2000000|(_SHIFTL(col.r,16,8))|(_SHIFTL(col.g,8,8))|(col.b&0xff);
    GX_LOAD_BP_REG(val);
}

void GX_InitFogAdjTable(GXFogAdjTbl *table,u16 width,f32 projmtx[4][4])
{
    u32 i,val7;
    f32 val0,val1,val2,val4,val5,val6;

    if(projmtx[3][3]==0.0f) {
        val0 = projmtx[2][3]/(projmtx[2][2] - 1.0f);
        val1 = val0/projmtx[0][0];
    } else {
        val1 = 1.0f/projmtx[0][0];
        val0 = val1*1.7320499f;
    }

    val2 = val0*val0;
    val4 = 2.0f/(f32)width;
    for(i=0;i<10;i++) {
        val5 = (i+1)*32.0f;
        val5 *= val4;
        val5 *= val1;
        val5 *= val5;
        val5 /= val2;
        val6 = sqrtf(val5 + 1.0f);
        val7 = (u32)(val6*256.0f);
        table->r[i] = (val7&0x0fff);
    }
}

void GX_SetFogRangeAdj(u8 enable,u16 center,GXFogAdjTbl *table)
{
    u32 val;

    if(enable) {
        val = 0xe9000000|(_SHIFTL(table->r[1],12,12))|(table->r[0]&0x0fff);
        GX_LOAD_BP_REG(val);

        val = 0xea000000|(_SHIFTL(table->r[3],12,12))|(table->r[2]&0x0fff);
        GX_LOAD_BP_REG(val);

        val = 0xeb000000|(_SHIFTL(table->r[5],12,12))|(table->r[4]&0x0fff);
        GX_LOAD_BP_REG(val);

        val = 0xec000000|(_SHIFTL(table->r[7],12,12))|(table->r[6]&0x0fff);
        GX_LOAD_BP_REG(val);

        val = 0xed000000|(_SHIFTL(table->r[9],12,12))|(table->r[8]&0x0fff);
        GX_LOAD_BP_REG(val);
    }
    val = 0xe8000000|(_SHIFTL(enable,10,1))|((center + 342)&0x03ff);
    GX_LOAD_BP_REG(val);
}

void GX_SetFogColor(GXColor color)
{
    GX_LOAD_BP_REG(0xf2000000|(_SHIFTL(color.r,16,8)|_SHIFTL(color.g,8,8)|(color.b&0xff)));
}

void GX_SetColorUpdate(u8 enable)
{
    __gx->peCMode0 = (__gx->peCMode0&~0x8)|(_SHIFTL(enable,3,1));
    GX_LOAD_BP_REG(__gx->peCMode0);
}

void GX_SetAlphaUpdate(u8 enable)
{
    __gx->peCMode0 = (__gx->peCMode0&~0x10)|(_SHIFTL(enable,4,1));
    GX_LOAD_BP_REG(__gx->peCMode0);
}

void GX_SetZCompLoc(u8 before_tex)
{
    __gx->peCntrl = (__gx->peCntrl&~0x40)|(_SHIFTL(before_tex,6,1));
    GX_LOAD_BP_REG(__gx->peCntrl);
}

void GX_SetIndTexOrder(u8 indtexstage,u8 texcoord,u8 texmap)
{
    switch(indtexstage) {
        case GX_INDTEXSTAGE0:
            __gx->tevRasOrder[2] = (__gx->tevRasOrder[2]&~0x7)|(texmap&0x7);
            __gx->tevRasOrder[2] = (__gx->tevRasOrder[2]&~0x38)|(_SHIFTL(texcoord,3,3));
            break;
        case GX_INDTEXSTAGE1:
            __gx->tevRasOrder[2] = (__gx->tevRasOrder[2]&~0x1C0)|(_SHIFTL(texmap,6,3));
            __gx->tevRasOrder[2] = (__gx->tevRasOrder[2]&~0xE00)|(_SHIFTL(texcoord,9,3));
            break;
        case GX_INDTEXSTAGE2:
            __gx->tevRasOrder[2] = (__gx->tevRasOrder[2]&~0x7000)|(_SHIFTL(texmap,12,3));
            __gx->tevRasOrder[2] = (__gx->tevRasOrder[2]&~0x38000)|(_SHIFTL(texcoord,15,3));
            break;
        case GX_INDTEXSTAGE3:
            __gx->tevRasOrder[2] = (__gx->tevRasOrder[2]&~0x1C0000)|(_SHIFTL(texmap,18,3));
            __gx->tevRasOrder[2] = (__gx->tevRasOrder[2]&~0xE00000)|(_SHIFTL(texcoord,21,3));
            break;
    }
    GX_LOAD_BP_REG(__gx->tevRasOrder[2]);
    __gx->dirtyState |= 0x0003;
}



#undef LARGE_NUMBER
