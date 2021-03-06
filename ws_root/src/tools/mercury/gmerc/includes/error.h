/****
 *
 *  @(#) error.h 95/12/04 1.19
 *  Copyright 1994, The 3DO Company
 *
 * List of possible errors generated by 3d libraries
 * 
 * TBD: This needs to be integrated with Portfolio error number scheme 
 * 
 ****/

#ifndef _GFXERROR
#define _GFXERROR

#define GFX_OK 		  		 	0
#define GFX_ErrorNoMemory 		(-1)
#define GFX_ErrorNotImplemented (-2)
#define GFX_ErrorInternal  		(-3)
#define GFX_ErrorInFrame  		(-4)
#define GFX_ErrorDrawMode  		(-5)
#define GFX_ErrorMinFilter  	(-6)
#define GFX_ErrorMagFilter  	(-7)
#define GFX_ErrorWrapMode  		(-8)
#define GFX_ErrorTextureMode 	(-9)
#define GFX_ErrorAxis 			(-10)
#define GFX_ErrorAspectRatio	(-11)
#define GFX_ErrorClipPlanes		(-12)
#define GFX_ErrorIndex			(-13)
#define GFX_ErrorStackUndeflow 	(-14)
#define GFX_ErrorFormat   		(-15)
#define GFX_ErrorDoesNotExist 	(-16)
#define GFX_ErrorFileOpen   	(-17)
#define GFX_ErrorOutOfRange		(-18)
#define GFX_ErrorBadArg			(-19)
#define GFX_ErrorChildUsed		(-20)
#define GFX_BoundsEmpty			(-21)
#define GFX_BadWrite			(-22)
#define GFX_BadRead				(-23)
#define GFX_InvalidChunkType	(-24)
#define GFX_InvalidTableSize	(-25)
#define GFX_ErrorNotFound		(-26)
#define GFX_ErrorFailed			(-27)
#define GFX_MiscError			(-28)

#endif /* _GFXERROR */
