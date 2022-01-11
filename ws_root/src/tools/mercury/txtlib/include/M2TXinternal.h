
/*
	File:		M2TXinternal.h

	Contains:	Prototypes for M2 Texture internal functions	 

	Written by:	Todd Allendorf, 3DO 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <6>	  9/6/95	TMA		Add autodocs for M2TX_WriteToIFF.
		 <5>	  8/7/95	TMA		Update CltSnippet to match change in Graphics Library.
		 <4>	  8/4/95	TMA		Added new types for TXTR functions (internal memory format).
		 <3>	 7/15/95	TMA		Autodocs updated.
		<1+>	 7/12/95	TMA		Remove Metrowerk's dependency.
	To Do:
*/


/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_CloneTXTR
|||	Copy a UTF texture's pointers into an M2TX structure.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_CloneTXTR(char *texPtr, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This functions takes a UTF structure in memory and copies the 
|||	    information into an M2TX structure for manipulation by the library.  
|||	    Only the data pointers to the levels of detail are copies, not the data
|||	    itself.
|||	
|||	  Caveats
|||	
|||	    If some other program frees up the levels of detail, the cloned texture
|||	    may no longer be valid.  Similarly, if user of the library frees up a 
|||	    level of details, the pointers in the original UTF texture in memory 
|||	    will no longer be valid.
|||	
|||	  Arguments
|||	
|||	    tex
|||	        The input M2TX texture.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_CopyTXTR()
**/

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_CopyTXTR
|||	Copy a UTF texture in memory to an M2TX structure.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_CopyTXTR(char *texPtr, M2TX *tex)
|||	
|||	  Description
|||	
|||	    This functions takes a UTF structure in memory and copies the
|||	    information into an M2TX structure for manipulation by the library.  A 
|||	    local copy of all the data is made and can be manipulated freely.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_CloneTXTR()
**/

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_ReadFileOld
|||	Read an old UTF file from disk into an M2TX structure.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_ReadFileOld(char *fileName, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This function reads a UTF texture into an M2TX structure. The texture is
|||	    read from a disk file specified by the file name.
|||	
|||	  Arguments
|||	    
|||	      tex
|||	          The input M2TX texture.
|||	    fileName
|||	        The name of the file to open.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_WriteMacFileOld()
**/


/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_ReadMacFileOld
|||	Reads an Old UTF texture from a Mac file.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_ReadMacFileOld(const FSSpec *spec, M2TX *tex)
|||	
|||	  Description
|||	
|||	    This function reads a UTF texture into an M2TX structure. The texture is
|||	    read from a disk file specified by the FSSpec.  This is only available 
|||	    on the Macintosh implementation of the library.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    spec
|||	        The FSSpec of a mac file whose data fork has the old UTF file.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_WriteMacFileOld()
**/

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_ReadFileNoLODsOld
|||	Reads an old UTF file into an M2TX structure except for the levels of detail.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_ReadFileNoLODsOld(char *fileName, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This function reads a UTF texture into an M2TX structure. The texture is
|||	    read from a disk file specified by the file name.  All the data is read
|||	    with the exception of the levels of detail (the actual pixel data).
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_ReadFileOld()
**/

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_WriteMacFileOld
|||	Writes a UTF to a Mac file.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_WriteMacFileOld(const FSSpec *spec, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This functions writes a UTF texture from an M2TX structure. The 
|||	    texture is written into the file specified by the FSSpec.  This is 
|||	    only available on the Macintosh implementation of the library.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_ReadMacFileOld()
**/

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_WriteFileOld
|||	Write an old UTF to disk from an M2TX structure.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_WriteFileOld(char *filename, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This functions writes a UTF texture from an M2TX structure. The texture
|||	    is written into the file specified by the file name.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    fileName
|||	        The name of the file to open.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_ReadFileOLD()
**/

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_WriteToIFF
|||	Write an old UTF to disk from an M2TX structure.
|||	
|||	  Synopsis
|||	
|||	    Err M2TX_WriteToIFF(IFFParser *iff, M2TX *tex, bool outLOD);
|||	
|||	  Description
|||	
|||	    This functions writes a UTF texture from an M2TX structure. The texture
|||	    is written into the IFF Parser stream specified by iff.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    iff
|||	        The opened IFF Parser stream.
|||	    outLOD
|||	        Whether to write out the levels of detail or not
|||	
|||	  Return Value
|||	
|||	    It returns an IFF error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
**/

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_WriteTXTR
|||	Write a M2TX structure to a UTF structure in memory.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_WriteTXTR(char **TXTRPtr, M2TX *tex)
|||	
|||	  Description
|||	
|||	    This functions takes an M2TX structure and writes a UTF structure into 
|||	    memory and returns a pointer to the newly allocated block of memory.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_CopyTXTR(), M2TX_CloneTXTR()
**/


/* I/O Functions */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef applec

#ifdef __MWERKS__
#pragma only_std_keywords off
#endif

#ifndef __FILES__
#include <Files.h>
#endif

#ifdef __MWERKS__
#pragma only_std_keywords reset
#pragma ANSI_strict reset
#endif

M2Err M2TX_ReadMacFileOld(const FSSpec *spec, M2TX *tex);
M2Err M2TX_WriteMacFileOld(const FSSpec *spec,	M2TX *tex);

#endif

/*  Here are some data types which are copied right out of Graphics Libraries */

typedef struct Tx_DCI {
  uint16  texelFormat[4];
  uint32  expColor[4];
} Tx_DCI;

typedef struct Tx_LOD {
  uint32   texelDataSize;
  void    *texelData;
} Tx_LOD;

typedef struct Tx_Data {
        uint32  flags;          /* compressed or not compressed */
        uint32  minX;           /* width in pixels of the smallest LOD */
        uint32  minY;           /* height in pixels of the smallest LOD */
        uint32  maxLOD;         /* maximum number of LODs */
        uint32  bitsPerPixel;   /* number of bits per pixel */
        uint32  expansionFormat; /* format of texels in TRAM */
        Tx_LOD  *texelData; /* pointer to each level of detail */
        Tx_DCI  *dci; /* pointer to data compression information */
}Tx_Data;

typedef struct GfxObj GfxObj;
typedef struct ObjFuncs
  {
    Err         (*Construct)(GfxObj* dst);
    void        (*DeleteAttrs)(GfxObj*);
    void        (*PrintInfo)(const GfxObj*);
    Err         (*Copy)(GfxObj* dst, GfxObj* src);
  } ObjFuncs;

struct GfxObj
   {
        ObjFuncs*       m_Funcs;                /* -> virtual function table */
        int8            m_Type;                 /* type GFX_xxx */
        int8            m_Flags;                /* memory allocation flags */
        int16           m_Use;                  /* reference count */
   };

typedef struct TexData
  {
    GfxObj          m_Base;     /* base object */
    int32           m_Flags;        /* internal flags */
    Tx_Data         m_Tex;          /* texture data */
  } TexData;

typedef struct Tx_PipControlBlock {
  void            *pipData;               /* Ptr to PIP data in memory */
  uint16          pipIndex;               /* index into the PIP (in number
					     of entries)
					     * from where th
					     e PIP data is to be loaded */
  uint16          pipNumEntries;  /* Number of entries in the PIP table */
  CltSnippet      pipCommandList; /* Command list snippet for loading the 
				     PIP*/
} Tx_PipControlBlock;

typedef struct Tx_LoadControlBlock {
        void            *textureBlock;
        uint16          firstLOD;
        uint16          numLOD;
        uint16          XWrap;
        uint16          YWrap;
        uint16          XSize;
        uint16          YSize;
        uint16          XOffset;
        uint16          YOffset;
        uint32          tramOffset;
        CltSnippet      lcbCommandList;
} Tx_LoadControlBlock;

typedef struct PipData
  {
    GfxObj    m_Base;
    int32     m_Flags;
    Tx_PipControlBlock  m_PCB;
  } PipData;

typedef struct TexData Texture;
typedef struct PipData PipTable;

typedef struct TxbData
   {
     GfxObj          m_Base;     /* base object */
     Texture*        m_Tex;          /* texture we are using */
     int32           m_Flags;        /* status flags */
     void*           m_LCB;          /* texture load information */
     PipTable*       m_PIP;          /* pip table */
     CltSnippet      m_TAB;          /* texture attributes */
     CltSnippet      m_DAB;          /* destination blender attributes */
   } TxbData;

typedef struct TxbData TexBlend;
typedef struct LCB TxLoadControlBlock;

#define Pip_GetData(t)  (((PipData*) (t))->m_PCB.pipData)
#define Pip_GetSize(t)  (((PipData*) (t))->m_PCB.pipNumEntries)
#define Pip_GetIndex(t) (((PipData*) (t))->m_PCB.pipIndex)

/*  Here are some data types which are copied right out of Graphics Libraries */


M2Err M2TX_CopyTXTR(TexBlend *texBlend, M2TX *tex);
M2Err M2TX_CloneTXTR(TexBlend *texBlend, M2TX *tex);
M2Err M2TX_WriteTXTR(Texture *texture, M2TX *tex);
M2Err M2TX_ReadFileNoLODsOld(char *fileName, M2TX *tex);
M2Err M2TX_ReadFileOld(char *fileName, M2TX *tex);
M2Err M2TX_WriteFileOld(char *fileName, M2TX *tex);

#ifdef __cplusplus
}
#endif

