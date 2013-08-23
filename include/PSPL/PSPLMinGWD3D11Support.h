#ifndef _PSPL_MINGW_D3D11_
#define _PSPL_MINGW_D3D11_
#ifdef _WIN32

#define __in
#define __out
#define __inout
#define __in_bcount(x)
#define __out_bcount(x)
#define __in_ecount(x)
#define __out_ecount(x)
#define __in_ecount_opt(x)
#define __out_ecount_opt(x)
#define __in_bcount_opt(x)
#define __out_bcount_opt(x)
#define __in_opt
#define __inout_opt
#define __out_opt
#define __out_ecount_part_opt(x,y)
#define __deref_out
#define __deref_out_opt
#define __RPC__deref_out
 
#include <d3d11.h> //from D3D SDK
 
#ifdef __uuidof
	#undef __uuidof
#endif
#define __uuidof(x) IID_##x


#endif
#endif