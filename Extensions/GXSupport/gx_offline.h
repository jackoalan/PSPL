#ifndef _GX_OFFLINE_H
#define _GX_OFFLINE_H

#include "gctypes.h"

/*! \typedef struct _gx_texreg GXTexRegion
 * \brief Object containing information on a texture cache region.
 *
 * \details This structure contains precompiled register state setting commands and data. The application must use the
 * GX_InitTexCacheRegion() function to initialize or change this object. The proper size of the object is returned by
 *
 * \code sizeof(GXTexRegion) \endcode
 *
 * \details but the internal data representation is not visible to the application.
 */
typedef struct _gx_texreg {
	u32 val[4];
} GXTexRegion;

/*! \typedef struct _gx_tlutreg GXTlutRegion
 * \brief Object containing information on a TLUT cache region.
 *
 * \details This structure contains precompiled register state setting commands and data. The application must use the GX_InitTlutRegion()
 * function to initialize or change this object. The proper size of the object is returned by
 *
 * \code sizeof(GXTlutRegion) \endcode
 *
 * \details but the internal data representation is not visible to the application.
 */
typedef struct _gx_tlutreg {
	u32 val[4];
} GXTlutRegion;

#include "gx_regdef.h"

#define GX_FALSE			0
#define GX_TRUE				1
#define GX_DISABLE			0
#define GX_ENABLE			1

/*! \addtogroup texgentyp Texture coordinate generation type
 * @{
 */
#define GX_TG_MTX3x4		0			/*!< 2x4 matrix multiply on the input attribute and generate S,T texture coordinates. */
#define GX_TG_MTX2x4		1			/*!< 3x4 matrix multiply on the input attribute and generate S,T,Q coordinates; S,T are then divided
* by Q to produce the actual 2D texture coordinates. */
#define GX_TG_BUMP0			2			/*!< Use light 0 in the bump map calculation. */
#define GX_TG_BUMP1			3			/*!< Use light 1 in the bump map calculation. */
#define GX_TG_BUMP2			4			/*!< Use light 2 in the bump map calculation. */
#define GX_TG_BUMP3			5			/*!< Use light 3 in the bump map calculation. */
#define GX_TG_BUMP4			6			/*!< Use light 4 in the bump map calculation. */
#define GX_TG_BUMP5			7			/*!< Use light 5 in the bump map calculation. */
#define GX_TG_BUMP6			8			/*!< Use light 6 in the bump map calculation. */
#define GX_TG_BUMP7			9			/*!< Use light 7 in the bump map calculation. */
#define GX_TG_SRTG			10			/*!< Coordinates generated from vertex lighting results; one of the color channel results is converted
* into texture coordinates. */
/*! @} */

/*! \addtogroup texgensrc Texture coordinate source
 * @{
 */
#define GX_TG_POS			0
#define GX_TG_NRM			1
#define GX_TG_BINRM			2
#define GX_TG_TANGENT		3
#define GX_TG_TEX0			4
#define GX_TG_TEX1			5
#define GX_TG_TEX2			6
#define GX_TG_TEX3			7
#define GX_TG_TEX4			8
#define GX_TG_TEX5			9
#define GX_TG_TEX6			10
#define GX_TG_TEX7			11
#define GX_TG_TEXCOORD0		12
#define GX_TG_TEXCOORD1		13
#define GX_TG_TEXCOORD2		14
#define GX_TG_TEXCOORD3		15
#define GX_TG_TEXCOORD4		16
#define GX_TG_TEXCOORD5		17
#define GX_TG_TEXCOORD6		18
#define GX_TG_COLOR0		19
#define GX_TG_COLOR1		20
/*! @} */

/*! \addtogroup compare Compare type
 * @{
 */
#define GX_NEVER			0
#define GX_LESS				1
#define GX_EQUAL			2
#define GX_LEQUAL			3
#define GX_GREATER			4
#define GX_NEQUAL			5
#define GX_GEQUAL			6
#define GX_ALWAYS			7
/*! @} */

/* Text Wrap Mode */
#define GX_CLAMP			0
#define GX_REPEAT			1
#define GX_MIRROR			2
#define GX_MAXTEXWRAPMODE	3

/*! \addtogroup blendmode Blending type
 * @{
 */
#define GX_BM_NONE			0			/*!< Write input directly to EFB */
#define GX_BM_BLEND			1			/*!< Blend using blending equation */
#define GX_BM_LOGIC			2			/*!< Blend using bitwise operation */
#define GX_BM_SUBTRACT		3			/*!< Input subtracts from existing pixel */
#define GX_MAX_BLENDMODE	4
/*! @} */

/*! \addtogroup blendfactor Blending control
 * \details Each pixel (source or destination) is multiplied by any of these controls.
 * @{
 */
#define GX_BL_ZERO			0			/*!< 0.0 */
#define GX_BL_ONE			1			/*!< 1.0 */
#define GX_BL_SRCCLR		2			/*!< source color */
#define GX_BL_INVSRCCLR		3			/*!< 1.0 - (source color) */
#define GX_BL_SRCALPHA		4			/*!< source alpha */
#define GX_BL_INVSRCALPHA	5			/*!< 1.0 - (source alpha) */
#define GX_BL_DSTALPHA		6			/*!< framebuffer alpha */
#define GX_BL_INVDSTALPHA	7			/*!< 1.0 - (FB alpha) */
#define GX_BL_DSTCLR		GX_BL_SRCCLR
#define GX_BL_INVDSTCLR		GX_BL_INVSRCCLR
/*! @} */

/*! \addtogroup logicop Logical operation type
 * \details Destination (dst) acquires the value of one of these operations, given in C syntax.
 * @{
 */
#define GX_LO_CLEAR			0			/*!< 0 */
#define GX_LO_AND			1			/*!< src & dst */
#define GX_LO_REVAND		2			/*!< src & ~dst */
#define GX_LO_COPY			3			/*!< src */
#define GX_LO_INVAND		4			/*!< ~src & dst */
#define GX_LO_NOOP			5			/*!< dst */
#define GX_LO_XOR			6			/*!< src ^ dst */
#define GX_LO_OR			7			/*!< src | dst */
#define GX_LO_NOR			8			/*!< ~(src | dst) */
#define GX_LO_EQUIV			9			/*!< ~(src ^ dst) */
#define GX_LO_INV			10			/*!< ~dst */
#define GX_LO_REVOR			11			/*!< src | ~dst */
#define GX_LO_INVCOPY		12			/*!< ~src */
#define GX_LO_INVOR			13			/*!< ~src | dst */
#define GX_LO_NAND			14			/*!< ~(src & dst) */
#define GX_LO_SET			15			/*!< 1 */
/*! @} */

/*! \addtogroup texoff Texture offset value
 * \brief Used for texturing points or lines.
 * @{
 */
#define GX_TO_ZERO			0
#define GX_TO_SIXTEENTH		1
#define GX_TO_EIGHTH		2
#define GX_TO_FOURTH		3
#define GX_TO_HALF			4
#define GX_TO_ONE			5
#define GX_MAX_TEXOFFSET	6
/*! @} */

/*! \addtogroup tevdefmode TEV combiner operation
 * \brief Color/Alpha combiner modes for GX_SetTevOp().
 *
 * \details For these equations, <i>Cv</i> is the output color for the stage, <i>Cr</i> is the output color of previous stage, and <i>Ct</i> is the texture color. <i>Av</i> is the output
 * alpha for a stage, <i>Ar</i> is the output alpha of previous stage, and <i>At</i> is the texture alpha. As a special case, rasterized color
 * (<tt>GX_CC_RASC</tt>) is used as <i>Cr</i> and rasterized alpha (<tt>GX_CA_RASA</tt>) is used as <i>Ar</i> at the first TEV stage because there is no previous stage.
 *
 * @{
 */

#define GX_MODULATE			0			/*!< <i>Cv</i>=<i>CrCt</i>; <i>Av</i>=<i>ArAt</i> */
#define GX_DECAL			1			/*!< <i>Cv</i>=(1-<i>At</i>)<i>Cr</i> + <i>AtCt</i>; <i>Av</i>=<i>Ar</i> */
#define GX_BLEND			2			/*!< <i>Cv=(1-<i>Ct</i>)<i>Cr</i> + <i>Ct</i>; <i>Av</i>=<i>AtAr</i> */
#define GX_REPLACE			3			/*!< <i>Cv=<i>Ct</i>; <i>Ar=<i>At</i> */
#define GX_PASSCLR			4			/*!< <i>Cv=<i>Cr</i>; <i>Av=<i>Ar</i> */

/*! @} */


/*! \addtogroup tevcolorarg TEV color combiner input
 * @{
 */

#define GX_CC_CPREV			0				/*!< Use the color value from previous TEV stage */
#define GX_CC_APREV			1				/*!< Use the alpha value from previous TEV stage */
#define GX_CC_C0			2				/*!< Use the color value from the color/output register 0 */
#define GX_CC_A0			3				/*!< Use the alpha value from the color/output register 0 */
#define GX_CC_C1			4				/*!< Use the color value from the color/output register 1 */
#define GX_CC_A1			5				/*!< Use the alpha value from the color/output register 1 */
#define GX_CC_C2			6				/*!< Use the color value from the color/output register 2 */
#define GX_CC_A2			7				/*!< Use the alpha value from the color/output register 2 */
#define GX_CC_TEXC			8				/*!< Use the color value from texture */
#define GX_CC_TEXA			9				/*!< Use the alpha value from texture */
#define GX_CC_RASC			10				/*!< Use the color value from rasterizer */
#define GX_CC_RASA			11				/*!< Use the alpha value from rasterizer */
#define GX_CC_ONE			12
#define GX_CC_HALF			13
#define GX_CC_KONST			14
#define GX_CC_ZERO			15				/*!< Use to pass zero value */

/*! @} */


/*! \addtogroup tevalphaarg TEV alpha combiner input
 * @{
 */

#define GX_CA_APREV			0				/*!< Use the alpha value from previous TEV stage */
#define GX_CA_A0			1				/*!< Use the alpha value from the color/output register 0 */
#define GX_CA_A1			2				/*!< Use the alpha value from the color/output register 1 */
#define GX_CA_A2			3				/*!< Use the alpha value from the color/output register 2 */
#define GX_CA_TEXA			4				/*!< Use the alpha value from texture */
#define GX_CA_RASA			5				/*!< Use the alpha value from rasterizer */
#define GX_CA_KONST			6
#define GX_CA_ZERO			7				/*!< Use to pass zero value */

/*! @} */


/*! \addtogroup tevstage TEV stage
 * \details The GameCube's Graphics Processor (GP) can use up to 16 stages to compute a texel for a particular surface.
 * By default, each texture will use two stages, but it can be configured through various functions calls.
 *
 * \note This is different from \ref texmapid s, where textures are loaded into.
 * @{
 */

#define GX_TEVSTAGE0		0
#define GX_TEVSTAGE1		1
#define GX_TEVSTAGE2		2
#define GX_TEVSTAGE3		3
#define GX_TEVSTAGE4		4
#define GX_TEVSTAGE5		5
#define GX_TEVSTAGE6		6
#define GX_TEVSTAGE7		7
#define GX_TEVSTAGE8		8
#define GX_TEVSTAGE9		9
#define GX_TEVSTAGE10		10
#define GX_TEVSTAGE11		11
#define GX_TEVSTAGE12		12
#define GX_TEVSTAGE13		13
#define GX_TEVSTAGE14		14
#define GX_TEVSTAGE15		15
#define GX_MAX_TEVSTAGE		16

/*! @} */


/*! \addtogroup tevop TEV combiner operator
 * @{
 */

#define GX_TEV_ADD				0
#define GX_TEV_SUB				1
#define GX_TEV_COMP_R8_GT		8
#define GX_TEV_COMP_R8_EQ		9
#define GX_TEV_COMP_GR16_GT		10
#define GX_TEV_COMP_GR16_EQ		11
#define GX_TEV_COMP_BGR24_GT	12
#define GX_TEV_COMP_BGR24_EQ	13
#define GX_TEV_COMP_RGB8_GT		14
#define GX_TEV_COMP_RGB8_EQ		15
#define GX_TEV_COMP_A8_GT		GX_TEV_COMP_RGB8_GT	 // for alpha channel
#define GX_TEV_COMP_A8_EQ		GX_TEV_COMP_RGB8_EQ  // for alpha channel

/*! @} */

/*! \addtogroup tevbias TEV bias value
 * @{
 */

#define GX_TB_ZERO				0
#define GX_TB_ADDHALF			1
#define GX_TB_SUBHALF			2
#define GX_MAX_TEVBIAS			3

/*! @} */

/*! \addtogroup tevscale TEV scale value
 * @{
 */

#define GX_CS_SCALE_1			0
#define GX_CS_SCALE_2			1
#define GX_CS_SCALE_4			2
#define GX_CS_DIVIDE_2			3
#define GX_MAX_TEVSCALE			4

/*! @} */

/*! \addtogroup texcoordid texture coordinate slot
 * @{
 */
#define GX_TEXCOORD0		0x0
#define GX_TEXCOORD1		0x1
#define GX_TEXCOORD2		0x2
#define GX_TEXCOORD3		0x3
#define GX_TEXCOORD4		0x4
#define GX_TEXCOORD5		0x5
#define GX_TEXCOORD6		0x6
#define GX_TEXCOORD7		0x7
#define GX_MAXCOORD			0x8
#define GX_TEXCOORDNULL		0xff
/*! @} */

/*! \addtogroup texmapid texture map slot
 * \brief Texture map slots to hold textures in.
 *
 * \details The GameCube's Graphics Processor (GP) can apply up to eight textures to a single surface. Those textures
 * are assigned one of these slots. Various operations used on or with a particular texture will also take one of these
 * items, including operations regarding texture coordinate generation (although not necessarily on the same slot).
 *
 * \note This is different from \ref tevstage s, which are the actual quanta for work with textures.
 * @{
 */
#define GX_TEXMAP0				0			/*!< Texture map slot 0 */
#define GX_TEXMAP1				1			/*!< Texture map slot 1 */
#define GX_TEXMAP2				2			/*!< Texture map slot 2 */
#define GX_TEXMAP3				3			/*!< Texture map slot 3 */
#define GX_TEXMAP4				4			/*!< Texture map slot 4 */
#define GX_TEXMAP5				5			/*!< Texture map slot 5 */
#define GX_TEXMAP6				6			/*!< Texture map slot 6 */
#define GX_TEXMAP7				7			/*!< Texture map slot 7 */
#define GX_MAX_TEXMAP			8
#define GX_TEXMAP_NULL			0xff			/*!< No texmap */
#define GX_TEXMAP_DISABLE		0x100			/*!< Disable texmap lookup for this texmap slot (use bitwise OR with a texture map slot). */
/*! @} */

/*! \addtogroup tevcoloutreg TEV color/output register
 * @{
 */

#define GX_TEVPREV				0			/*!< Default register for passing results from one stage to another. */
#define GX_TEVREG0				1
#define GX_TEVREG1				2
#define GX_TEVREG2				3
#define GX_MAX_TEVREG			4

/*! @} */

typedef struct _gx_color {
 	u8 r;			/*!< Red color component. */
 	u8 g;			/*!< Green color component. */
 	u8 b;			/*!< Blue alpha component. */
	u8 a;			/*!< Alpha component. If a function does not use the alpha value, it is safely ignored. */
} GXColor;

/*! \typedef struct _gx_colors10 GXColorS10
 * \brief Structure used to pass signed 10-bit colors to some GX functions.
 */
typedef struct _gx_colors10 {
 	s16 r;			/*!< Red color component. */
 	s16 g;			/*!< Green color component. */
 	s16 b;			/*!< Blue color component. */
	s16 a;			/*!< Alpha component. If a function does not use the alpha value, it is safely ignored. */
} GXColorS10;

/*! \struct GXFogAdjTbl
 * \brief Fog range adjustment parameter table.
 */
typedef struct {
	u16 r[10];			/*!< u4.8 format range parameter. */
} GXFogAdjTbl;


/*! \addtogroup indtexstage Indirect texture stage
 * @{
 */
#define GX_INDTEXSTAGE0					0
#define GX_INDTEXSTAGE1					1
#define GX_INDTEXSTAGE2					2
#define GX_INDTEXSTAGE3					3
#define GX_MAX_INDTEXSTAGE				4
/*! @} */

/*! \addtogroup channelid Color channel ID
 * @{
 */
#define GX_COLOR0			0
#define GX_COLOR1			1
#define GX_ALPHA0			2
#define GX_ALPHA1			3
#define GX_COLOR0A0			4
#define GX_COLOR1A1			5
#define GX_COLORZERO		6
#define GX_ALPHA_BUMP		7
#define GX_ALPHA_BUMPN		8
#define GX_COLORNULL		0xff
/*! @} */

/*! \addtogroup lightid Light ID
 * @{
 */
#define GX_LIGHT0			0x001			/*!< Light 0 */
#define GX_LIGHT1			0x002			/*!< Light 2 */
#define GX_LIGHT2			0x004			/*!< Light 3 */
#define GX_LIGHT3			0x008			/*!< Light 4 */
#define GX_LIGHT4			0x010			/*!< Light 5 */
#define GX_LIGHT5			0x020			/*!< Light 6 */
#define GX_LIGHT6			0x040			/*!< Light 7 */
#define GX_LIGHT7			0x080			/*!< Light 8 */
#define GX_MAXLIGHT			0x100			/*!< All lights */
#define GX_LIGHTNULL		0x000			/*!< No lights */
/*! @} */

/*! \addtogroup difffn Diffuse function
 * @{
 */
#define GX_DF_NONE			0
#define GX_DF_SIGNED		1
#define GX_DF_CLAMP			2
/*! @} */

/*! \addtogroup attenfunc Attenuation function
 * @{
 */
#define GX_AF_SPEC			0			/*!< Specular computation */
#define GX_AF_SPOT			1			/*!< Spot light attenuation */
#define GX_AF_NONE			2			/*!< No attenuation */
/*! @} */

/*! \addtogroup indtexformat Indirect texture format
 * \details Bits for the indirect offsets are extracted from the high end of each component byte. Bits for the bump alpha
 * are extraced off the low end of the byte. For <tt>GX_ITF_8</tt>, the byte is duplicated for the offset and the bump alpha.
 * @{
 */
#define GX_ITF_8						0
#define GX_ITF_5						1
#define GX_ITF_4						2
#define GX_ITF_3						3
#define GX_MAX_ITFORMAT					4
/*! @} */

/*! \addtogroup indtexbias Indirect texture bias select
 * \brief Indicates which components of the indirect offset should receive a bias value.
 *
 * \details The bias is fixed at -128 for <tt>GX_ITF_8</tt> and +1 for the other formats. The bias happens prior to the indirect matrix multiply.
 * @{
 */
#define GX_ITB_NONE						0
#define GX_ITB_S						1
#define GX_ITB_T						2
#define GX_ITB_ST						3
#define GX_ITB_U						4
#define GX_ITB_SU						5
#define GX_ITB_TU						6
#define GX_ITB_STU						7
#define GX_MAX_ITBIAS					8
/*! @} */

/*! \addtogroup indtexmtx Indirect texture matrix
 * @{
 */
#define GX_ITM_OFF						0			/*!< Specifies a matrix of all zeroes. */
#define GX_ITM_0						1			/*!< Specifies indirect matrix 0, indirect scale 0. */
#define GX_ITM_1						2			/*!< Specifies indirect matrix 1, indirect scale 1. */
#define GX_ITM_2						3			/*!< Specifies indirect matrix 2, indirect scale 2. */
#define GX_ITM_S0						5			/*!< Specifies dynamic S-type matrix, indirect scale 0. */
#define GX_ITM_S1						6			/*!< Specifies dynamic S-type matrix, indirect scale 1. */
#define GX_ITM_S2						7			/*!< Specifies dynamic S-type matrix, indirect scale 2. */
#define GX_ITM_T0						9			/*!< Specifies dynamic T-type matrix, indirect scale 0. */
#define GX_ITM_T1						10			/*!< Specifies dynamic T-type matrix, indirect scale 1. */
#define GX_ITM_T2						11			/*!< Specifies dynamic T-type matrix, indirect scale 2. */
/*! @} */

/*! \addtogroup indtexwrap Indirect texture wrap value
 * \brief Indicates whether the regular texture coordinate should be wrapped before being added to the offset.
 *
 * \details <tt>GX_ITW_OFF</tt> specifies no wrapping. <tt>GX_ITW_0</tt> will zero out the regular texture coordinate.
 * @{
 */
#define GX_ITW_OFF						0
#define GX_ITW_256						1
#define GX_ITW_128						2
#define GX_ITW_64						3
#define GX_ITW_32						4
#define GX_ITW_16						5
#define GX_ITW_0						6
#define GX_MAX_ITWRAP					7
/*! @} */

/*! \addtogroup indtexalphasel Indirect texture bump alpha select
 * \brief Indicates which offset component should provide the "bump" alpha output for the given TEV stage.
 *
 * \note Bump alpha is not available for TEV stage 0.
 * @{
 */
#define GX_ITBA_OFF						0
#define GX_ITBA_S						1
#define GX_ITBA_T						2
#define GX_ITBA_U						3
#define GX_MAX_ITBALPHA					4
/*! @} */

/*! \addtogroup indtexscale Indirect texture scale
 * \brief Specifies an additional scale value that may be applied to the texcoord used for an indirect initial lookup (not a TEV stage regular lookup).
 *
 * \details The scale value is a fraction; thus <tt>GX_ITS_32</tt> means to divide the texture coordinate values by 32.
 * @{
 */
#define GX_ITS_1						0
#define GX_ITS_2						1
#define GX_ITS_4						2
#define GX_ITS_8						3
#define GX_ITS_16						4
#define GX_ITS_32						5
#define GX_ITS_64						6
#define GX_ITS_128						7
#define GX_ITS_256						8
#define GX_MAX_ITSCALE					9
/*! @} */

/*! \addtogroup dttmtx Post-transform texture matrix index
 * @{
 */
#define GX_DTTMTX0			64
#define GX_DTTMTX1			67
#define GX_DTTMTX2			70
#define GX_DTTMTX3			73
#define GX_DTTMTX4			76
#define GX_DTTMTX5			79
#define GX_DTTMTX6			82
#define GX_DTTMTX7			85
#define GX_DTTMTX8			88
#define GX_DTTMTX9			91
#define GX_DTTMTX10			94
#define GX_DTTMTX11			97
#define GX_DTTMTX12			100
#define GX_DTTMTX13			103
#define GX_DTTMTX14			106
#define GX_DTTMTX15			109
#define GX_DTTMTX16			112
#define GX_DTTMTX17			115
#define GX_DTTMTX18			118
#define GX_DTTMTX19			121
#define GX_DTTIDENTITY		125
/*! @} */


void GX_Flush();


void GX_SetChanCtrl(s32 channel,u8 enable,u8 ambsrc,u8 matsrc,u8 litmask,u8 diff_fn,u8 attn_fn);

void GX_SetChanAmbColor(s32 channel,GXColor color);

void GX_SetChanMatColor(s32 channel,GXColor color);

void GX_SetTexCoordGen(u16 texcoord,u32 tgen_typ,u32 tgen_src,u32 mtxsrc);

void GX_SetTexCoordGen2(u16 texcoord,u32 tgen_typ,u32 tgen_src,u32 mtxsrc,u32 normalize,u32 postmtx);


void GX_SetCurrentMtx(u32 mtx);

void GX_SetNumTexGens(u32 nr);

void GX_SetZMode(u8 enable,u8 func,u8 update_enable);

void GX_SetTexCoordScaleManually(u8 texcoord,u8 enable,u16 ss,u16 ts);

void GX_SetTexCoordCylWrap(u8 texcoord,u8 s_enable,u8 t_enable);

void GX_SetTexCoordBias(u8 texcoord,u8 s_enable,u8 t_enable);

void GX_SetBlendMode(u8 type,u8 src_fact,u8 dst_fact,u8 op);

void GX_ClearVtxDesc();

void GX_SetLineWidth(u8 width,u8 fmt);

void GX_SetPointSize(u8 width,u8 fmt);

void GX_SetTevColor(u8 tev_regid,GXColor color);

void GX_SetTevColorS10(u8 tev_regid,GXColorS10 color);

void GX_SetTevKColor(u8 tev_kregid,GXColor color);

void GX_SetTevKColorS10(u8 tev_kregid,GXColorS10 color);

void GX_SetTevOp(u8 tevstage,u8 mode);

void GX_SetTevColorIn(u8 tevstage,u8 a,u8 b,u8 c,u8 d);

void GX_SetTevAlphaIn(u8 tevstage,u8 a,u8 b,u8 c,u8 d);

void GX_SetTevColorOp(u8 tevstage,u8 tevop,u8 tevbias,u8 tevscale,u8 clamp,u8 tevregid);

void GX_SetTevAlphaOp(u8 tevstage,u8 tevop,u8 tevbias,u8 tevscale,u8 clamp,u8 tevregid);

void GX_SetCullMode(u8 mode);

void GX_SetCoPlanar(u8 enable);

void GX_EnableTexOffsets(u8 coord,u8 line_enable,u8 point_enable);

void GX_SetNumChans(u8 num);

void GX_SetTevOrder(u8 tevstage,u8 texcoord,u32 texmap,u8 color);

void GX_SetNumTevStages(u8 num);

void GX_SetAlphaCompare(u8 comp0,u8 ref0,u8 aop,u8 comp1,u8 ref1);

void GX_SetTevKColorSel(u8 tevstage,u8 sel);

void GX_SetTevKAlphaSel(u8 tevstage,u8 sel);

void GX_SetTevSwapMode(u8 tevstage,u8 ras_sel,u8 tex_sel);

void GX_SetTevSwapModeTable(u8 swapid,u8 r,u8 g,u8 b,u8 a);

void GX_SetTevIndirect(u8 tevstage,u8 indtexid,u8 format,u8 bias,u8 mtxid,u8 wrap_s,u8 wrap_t,u8 addprev,u8 utclod,u8 a);

void GX_SetTevDirect(u8 tevstage);

void GX_SetNumIndStages(u8 nstages);

void GX_SetIndTexMatrix(u8 indtexmtx,f32 offset_mtx[2][3],s8 scale_exp);

void GX_SetTevIndBumpST(u8 tevstage,u8 indstage,u8 mtx_sel);

void GX_SetTevIndBumpXYZ(u8 tevstage,u8 indstage,u8 mtx_sel);

void GX_SetTevIndRepeat(u8 tevstage);

void GX_SetIndTexCoordScale(u8 indtexid,u8 scale_s,u8 scale_t);

void GX_SetTevIndTile(u8 tevstage,u8 indtexid,u16 tilesize_x,u16 tilesize_y,u16 tilespacing_x,u16 tilespacing_y,u8 indtexfmt,u8 indtexmtx,u8 bias_sel,u8 alpha_sel);

void GX_SetFog(u8 type,f32 startz,f32 endz,f32 nearz,f32 farz,GXColor col);

void GX_InitFogAdjTable(GXFogAdjTbl *table,u16 width,f32 projmtx[4][4]);

void GX_SetFogRangeAdj(u8 enable,u16 center,GXFogAdjTbl *table);

void GX_SetFogColor(GXColor color);

void GX_SetColorUpdate(u8 enable);

void GX_SetAlphaUpdate(u8 enable);

void GX_SetZCompLoc(u8 before_tex);

void GX_SetIndTexOrder(u8 indtexstage,u8 texcoord,u8 texmap);

#endif // _GX_OFFLINE_H
