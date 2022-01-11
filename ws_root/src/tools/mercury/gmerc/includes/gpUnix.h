/****
 *
 * Basic types and constants for Unix implementation
 *
 ****/
#ifndef _GPPORT
#define _GPPORT

#include <math.h>

#ifdef applec
#include <types.h>
#endif

#include <malloc.h>
#include <bstring.h>

#define UNUSED(x)		&x

#if !defined(_SIZE_T) && !defined(_SIZE_T_)
typedef unsigned int    size_t;
#define _SIZE_T
#endif

typedef void*           Ptr;        /* pointer to anything */

/* put kernel types ifdef so that they can be defined in :kernel:types.h */
#ifndef __KERNEL_TYPES_H
typedef unsigned long   uint32;     /* 32 bit unsigned */
typedef long            int32;      /* 32 bit signed */
typedef unsigned char   uchar;      /* 8 bit unsigned */
typedef unsigned short  uint16;     /* 16 bit unsigned */
typedef short           int16;      /* 16 bit signed */
typedef char            bool;       /* true/false flag */
typedef unsigned        bits;       /* bit field */
typedef unsigned char	uint8;
typedef unsigned char	ubyte;
typedef char			int8;
typedef float			gfloat;		/* native floating point */
typedef char*			BytePtr;	/* pointer to byte */

typedef int32			Item;
typedef int32			Err;
#endif

/*
 * This is a PRIVATE structure used internally. It is platform
 * dependent and must be included here but SHOULD NOT BE
 * USED PUBLICLY. This structure is subject to change without notice.
 */
typedef struct GfxBitmap {
	uint32      bm_Width;   /*  Total width, in pixels. */
    uint32      bm_Height;  /*  Total height, in pixels.    */

    uint32      bm_XOrigin; /*  In pixels           */
    uint32      bm_YOrigin;
    uint32      bm_ClipWidth;
    uint32      bm_ClipHeight;

    uint32      bm_BufferSize;  /*  Size of buffer, in bytes    */
    uint32      bm_BufMemType;  /*  Required memory type bits   */

    void        *bm_Buffer; /*  Pointer to RAM buffer   */

    uint32      bm_Type;    /*  Type of bitmap.     */
} GfxBitmap;

#define BMTYPE_8                0
#define BMTYPE_16               1
#define BMTYPE_32               2
#define BMTYPE_16_ZBUFFER       3

typedef GfxBitmap Bitmap;
typedef GfxBitmap * BitmapPtr;

/***************************************************************************
 * Definition of the structure in the Send Buffer of an IO Request during
 * Send IO.
 */
#define GFX_CMD_LIST_VERSION	0x00000000

typedef uint32 * CmdListP ;

typedef struct CmdHeader {
	uint32	ch_Version;	/* Version Information for the Command List */
} CmdHeader;

typedef struct teRenderInfo {
	Item	 ri_Context;	/* item number of the Context Item to be used
				 * with this IO Request. If no context is required
				 * field should be set to NO_CONTEXT  */
	Item	 ri_FrameBuffer;/* item number of the Bitmap Item to be used as a
				 * frame buffer for this IO Request. If this is not
				 * specified (indicated by setting this field to
				 * NO_FRAME_BUFFER), the registers corresponding the
				 * Frame Buffer in the Triangle Engine are set
				 * so that YClip = XClip = FrameBufferWidth = 0.
				 */
	Item	 ri_ZBuffer;	/* item number of the Bitmap Item to be used as a
				 * frame buffer for this IO Request. If this is not
				 * specified, (indicated by setting this field to
				 * NO_Z_BUFFER) the registers corresponding to the
				 * Z Buffer in the Triangle Engine are set so that
				 * ZBufferWidth = ZBufferHeight = 0.
				 */
	CmdHeader ri_Header;	/* Header information for the command list */
	CmdListP  ri_CmdList;	/* Pointer to the first instruction of the
				 * triangle engine command list.
				 */
} teRenderInfo;

#endif	/* GPPORT */
