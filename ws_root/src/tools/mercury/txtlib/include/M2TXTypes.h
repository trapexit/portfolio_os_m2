/*
	File:		M2TXTypes.h

	Contains:	Types for M2 Texture mapping library 

	Written by:	Todd Allendorf, 3DO 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <6>	  8/7/95	TMA		New header flags added.  WrapModes moved to the M2TXattr.h file.
		 <5>	  8/4/95	TMA		Fixed comments.
		 <3>	 7/11/95	TMA		Changes to accomodate the new IFF UTF format. TAB, DAB,
									LoadRects added. PIP, Texel structures changed. TX_WrapModeTile,
									TX_WrapModeClamp defines added.
		 <3>	 1/20/95	TMA		Updated error headers
		<1+>	 1/16/95	TMA		Update new types

	To Do:
*/

#ifndef	_H_M2TXTypes			/* 1.06 */
#define _H_M2TXTypes

#include "qGlobal.h"
#include "M2Err.h"
#include "clt.h"

#define MAX_LOD_NUM		12

/* Header chunk flags */
#define M2HC_IsCompressed	0x0001
#define M2HC_HasPIP		0x0002
#define M2HC_HasColorConst	0x0004
#define M2HC_HasNoDCI   	0x0008
#define M2HC_HasNoTAB  	0x0010
#define M2HC_HasNoDAB  	0x0020
#define M2HC_HasNoLR  	0x0040

/* filter types */
#define M2FILTER_BOX		0x00000001   
#define M2FILTER_TRIANGLE	0x00000002
#define M2FILTER_BELL		0x00000004
#define M2FILTER_B_SPLINE	0x00000008
#define M2FILTER_		0x00000010
#define M2FILTER_LANCZS3	0x00000020
#define M2FILTER_MICHELL	0x00000040

/* sampling types */

#define M2SAMPLE_USER		0x00000001L
#define M2SAMPLE_POINT	0x00000002L
#define M2SAMPLE_AVERAGE	0x00000004L
#define M2SAMPLE_WEIGHT	0x00000008L
#define M2SAMPLE_SINH		0x00000010L
#define M2SAMPLE_LANCZS3	0x00000020L
#define M2SAMPLE_MICHELL	0x00000040L


/* texture map flag */
#define M2HC_IsLiteral	0x1000
#define M2HC_HasAlpha		0x0800
#define M2HC_HasSSB		0x0200
#define M2HC_HasColor		0x0400
#define M2HC_ColorDepth	0x000f
#define M2HC_AlphaDepth	0x00f0
#define M2Channel_Red		0x0001   
#define M2Channel_Green	0x0002
#define M2Channel_Blue	0x0004
#define M2Channel_Alpha	0x0008
#define M2Channel_Colors	0x0007
#define M2Channel_SSB		0x0010
#define M2Channel_Index	0x0020
#define M2Channel_ALL		0x003F

/* flag shifts */
#define M2Shift_IsCompressed	0
#define M2Shift_HasPIP	1
#define M2Shift_HasColorConst	2
#define M2Shift_HasSSB	9
#define M2Shift_HasColor	10
#define M2Shift_HasAlpha	11
#define	 M2Shift_IsLiteral	12
#define M2Shift_ColorDepth	0		
#define M2Shift_AlphaDepth	4

/* texel format flag */
#define M2CI_IsTrans		0x0100
#define M2CI_IsLiteral	0x1000
#define M2CI_HasAlpha		0x0800
#define M2CI_HasSSB		0x0200
#define M2CI_HasColor		0x0400
#define M2CI_ColorDepth	0x000f
#define M2CI_AlphaDepth	0x00f0

/* compression type*/
#define M2CMP_RLE		0x0001	/* Run Length encodes */
#define	M2CMP_Auto		0x0002  /* Do your best. Right = BestPIPDCI + RLE */
#define M2CMP_BestDCIPIP	0x0004	/* Find the best DCI PIP combination */
#define M2CMP_CustomPIP	0x0008
#define M2CMP_CustomDCI	0x0010
#define M2CMP_LockPIP		0x0020
#define M2CMP_None		0x0040	


#define M2TX_WRITE_TXTR       0x0001
#define M2TX_WRITE_PIP        0x0002
#define M2TX_WRITE_TAB        0x0004
#define M2TX_WRITE_DAB        0x0008
#define M2TX_WRITE_LOD        0x0010 
#define M2TX_WRITE_M2TX       0x0020
#define M2TX_WRITE_FORM       0x0040
#define M2TX_WRITE_LR         0x0080
#define M2TX_WRITE_DCI        0x0100
#define M2TX_WRITE_M2PG       0x0200
#define M2TX_WRITE_PCLT       0x0400

#define M2TX_WRITE_ALL  M2TX_WRITE_TXTR | M2TX_WRITE_PIP | \
M2TX_WRITE_TAB | M2TX_WRITE_DAB | M2TX_WRITE_LOD | M2TX_WRITE_LR | \
 M2TX_WRITE_DCI | M2TX_WRITE_M2PG | M2TX_WRITE_PCLT



typedef struct tag_M2TXWrapper	/* texture map Wrapper Chunk Header */
{
	uint32 Signature;	/* signature for Wrapper Chunk Header */
	uint32 FileSize;	/* size of the file */
} M2TXWrapper;

typedef unsigned char *M2TXTex; /* The raw texel data minus the chunk information */

typedef struct tag_M2TXHeader	/* texture map Header Chunk */
{
	uint32 Signature;	/* signature for the Header Chunk */
	uint32 Size;		/* size of the Header Chunk */
	uint32 Version;		/* version number of the texture map file */
	uint32 Flags;		/* flag for the texture file */
	uint16 MinXSize;	/* min x value for the lod */
	uint16 MinYSize;	/* min y value for the lod */
	uint16 TexFormat;	/* flag for texture format */
	uint8  NumLOD;		/* number of lod */
	uint8  Border;		/* border */
	uint32 DCIOffset; 	/* byte offset if the data in the disk */
	uint32 PIPOffset;	/* byte offset if the data in the disk */
	uint32 LODDataOffset[12];   /* byte offset to each lod in file */
	M2TXTex LODDataPtr[12];     /* Pointer to each of LOD */
	uint32	LODDataLength[12];  /* Length of each LODDataPtr */
	uint32 ColorConst[2];	/* color constant */
} M2TXHeader;

typedef struct tag_M2TXPIP	/* texture map PIP Chunk */
{
	uint32 Signature;	/* signature for the pip chunk */
	uint32 Size;		/* size of the pip chunk */
	uint32 PIPData[256];	/* color table */
	int16  NumColors;       /* Number of colors stored in the PIP */
	uint32 IndexOffset;     /* The PIP loading Index Offset */
	uint8  SortIndex[256];	/* The translation table after a PIP sort */
} M2TXPIP;

typedef struct tag_M2TXDCI	/* Data Compression Information Chunk */
{
	uint32 Signature;	/* 'M2CI' */
	uint32 Size;		/* size of the dci chunk */
	uint16 TexelFormat[4];	/* texel format for each texel format */
	uint32 TxExpColorConst[4];   /* color constant for each texel format */
} M2TXDCI;

typedef struct tag_M2Texel	/* Texel Data Chunk */
{
  uint32 Signature;	        /* 'M2TD' */
  uint32 Size;			/* size of the Texel Data Chunk */
} M2TXTexel;


typedef struct tag_M2TA		/* Texture Attributes Chunk */
{
  uint32 Signature;		/* 'M2TA' */
  uint32 Size;			/* size of the Texture Attributes Chunk in bytes */
  uint32 Reserved;		/* Reserved Field */
  uint32 *Tx_Attr;	
} M2TXTA;

typedef struct tag_M2DB		/* Destination Blend Attributes Chunk */
{
  uint32 Signature;		/* 'M2DB' */
  uint32 Size;	                /* size of the Destination Blend Attr Chunk in bytes */
  uint32 Reserved;		/* Reserved Field */
  uint32 *DBl_Attr;
} M2TXDB;

typedef struct tag_M2Rect
{
  int16 XWrapMode;       /* Whether to repeat or clamp the texture */
  int16 YWrapMode;       /* Whether to repeat or clamp the texture */
  int16 FirstLOD;        /* The finest LOD to clip from */
  int16 NLOD;            /* The number of LODs to clip from */
  int16 XOff;            /* The clippping X Offset of the coarset LOD */
  int16 YOff;            /* The clippping Y Offset of the coarset LOD */
  int16 XSize;           /* The X Size of the clip rect on the coarsest LOD */
  int16 YSize;           /* The Y Size of the clip rect on the coarsest LOD */
} M2TXRect;

typedef struct tag_M2LR         /* Load Rects Chunk */
{
  uint32 Signature;		/* 'M2LR' */
  uint32 Size;			/* size of the Load Rects Chunk in bytes */
  uint32 NumLoadRects;		/* the number of Load Rectangles */
  uint32 Reserved;              /* Reserved field */
  M2TXRect *LRData;             /* The load Rectangles */
} M2TXLR;


#define M2PG_FLAGS_XWrapMode     0x00000001
#define M2PG_FLAGS_YWrapMode     0x00000002
#define M2PG_FLAGS_HasTexFormat  0x00000004
#define M2PG_FLAGS_HasPIP        0x00000008
#define M2PG_FLAGS_HasTAB        0x00000010
#define M2PG_FLAGS_HasDAB        0x00000020
#define M2PG_FLAGS_HasLR         0x00000040
#define M2PG_FLAGS_IsCompressed  0x00000080

typedef struct tag_M2TXPgHeader
{
  uint32   Offset;
  uint32   PgFlags;
  uint16   MinXSize;
  uint16   MinYSize;
  uint16   TexFormat;
  uint8    NumLOD;
  uint8    PIPIndexOffset;
  uint8    PgPIPIndex;         /*Note: Not used as of Mercury 3.0 */
  uint8    PgTABIndex;         /*Note: Not used as of Mercury 3.0 */
  uint8    PgDABIndex;         /*Note: Not used as of Mercury 3.0 */
  uint8    PgLRIndex;          /*Note: Not used as of Mercury 3.0 */

  /*Note; New for Mercury 3.0 */
  uint16   TexelFormat[4];
  uint32   TxExpColorConst[4];
  uint32   LODLength[11];      /* Must be <11, will only write out NumLOD */
} M2TXPgHeader;

typedef struct tag_M2TXPg
{
  uint32       Signature;      /* 'M2PG' */
  uint32       Size;
  uint32       NumTex;         /* Number of sub-textures within a page */
  uint32       Version;        /* Version field, needed for Mercury 3.0 */
  M2TXPgHeader *PgData;     
} M2TXPg;



typedef struct tag_PgCLT
{
  uint32       Signature;      /* 'PCLT' */
  uint32       Size;
  uint32       NumTex;         /* Number of sub-textures within a page */
  uint32       Version;        /* Version field */
  uint32       NumCompressed;  /* Number of Compressed LODs */
  uint32       PageCLTOff;
  uint32       PIPCLTOff;
  uint32       *TexCLTOff;
  CltSnippet   PageLoadCLT;
  CltSnippet   PIPLoadCLT;
  CltSnippet   *TexRefCLT;
  /* New for Mercury 3.0 */
  float        *UVScale;       /* 32-bit float */
  uint32       *PatchOffset;   /* Offset in PageCLT for Compr Tex Loads (each LOD) */
} PgCLT;

typedef struct tag_M2TX
{
  M2TXWrapper	Wrapper;
  M2TXHeader	Header;
  M2TXPIP	PIP;
  M2TXDCI	DCI;
  M2TXTexel	Texel;
  M2TXTA        TexAttr;
  M2TXDB        DBl;
  M2TXLR        LoadRects;
  M2TXPg        Page;
  PgCLT         PCLT;
} M2TX;

typedef uint32  M2TXColor;
typedef uint16 M2TXFormat;

/* Seen by the API user
  These are temporary structures for easy access */

typedef struct tag_M2TXImageRaw
{
	uint8 *Red;
	uint8 *Green;
	uint8 *Blue;
	uint8 *Alpha;
	uint8 *SSB;
	bool HasColor, HasAlpha, HasSSB;
	uint32 XSize, YSize;
} M2TXRaw;

typedef struct tag_M2TXImageIndex  
{
	uint8 *Index;
	uint8 *Alpha;
	uint8 *SSB;
	bool HasColor, HasAlpha, HasSSB;
	uint32 XSize, YSize;
} M2TXIndex;

#endif					/* 1.06 */









