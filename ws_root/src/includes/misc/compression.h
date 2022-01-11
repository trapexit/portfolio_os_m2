#ifndef __MISC_COMPRESSION_H
#define __MISC_COMPRESSION_H


/******************************************************************************
**
**  @(#) compression.h 96/02/29 1.13
**
**  Compression folio interface definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif


/****************************************************************************/


/* kernel interface definitions */
#define COMP_FOLIONAME  "compression"


/*****************************************************************************/


typedef struct Compressor Compressor;
typedef struct Decompressor Decompressor;
typedef void (* CompFunc)(void *userData, uint32 word);


/*****************************************************************************/


/* Error codes */

#define MakeCompErr(svr,class,err) MakeErr(ER_FOLI,ER_COMP,svr,ER_E_SSTM,class,err)

/* Bad Compressor/Decompressor parameter */
#define COMP_ERR_BADPTR       MakeCompErr(ER_SEVERE,ER_C_STND,ER_BadPtr)

/* Unknown tag supplied */
#define COMP_ERR_BADTAG       MakeCompErr(ER_SEVERE,ER_C_STND,ER_BadTagArg)

/* No memory */
#define COMP_ERR_NOMEM        MakeCompErr(ER_SEVERE,ER_C_STND,ER_NoMem)

/* More data than needed */
#define COMP_ERR_DATAREMAINS  MakeCompErr(ER_SEVERE,ER_C_NSTND,1)

/* Not enough data */
#define COMP_ERR_DATAMISSING  MakeCompErr(ER_SEVERE,ER_C_NSTND,2)

/* Too much data for target buffer */
#define COMP_ERR_OVERFLOW     MakeCompErr(ER_SEVERE,ER_C_NSTND,3)


/*****************************************************************************/


/* for use with CreateCompressor() and CreateDecompressor() */
typedef enum CompressionTags
{
    COMP_TAG_WORKBUFFER = TAG_ITEM_LAST+1,
    COMP_TAG_USERDATA
} CompressionTags;


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


/* folio management */
Err OpenCompressionFolio(void);
Err CloseCompressionFolio(void);

/* compressor */
Err CreateCompressor(Compressor **compr, CompFunc cf, const TagArg *tags);
Err DeleteCompressor(Compressor *compr);
Err FeedCompressor(Compressor *compr, const void *data, uint32 numDataWords);
int32 GetCompressorWorkBufferSize(const TagArg *tags);

/* decompressor */
Err CreateDecompressor(Decompressor **decomp, CompFunc cf, const TagArg *tags);
Err DeleteDecompressor(Decompressor *decomp);
Err FeedDecompressor(Decompressor *decomp, const void *data, uint32 numDataWords);
int32 GetDecompressorWorkBufferSize(const TagArg *tags);

/* varargs variants of some of the above */
Err CreateCompressorVA(Compressor **compr, CompFunc cf, uint32 tags, ...);
int32 GetCompressorWorkBufferSizeVA(uint32 tags, ...);
Err CreateDecompressorVA(Decompressor **decomp, CompFunc cf, uint32 tags, ...);
int32 GetDecompressorWorkBufferSizeVA(uint32 tags, ...);

/* convenience routines */
Err SimpleCompress(const void *source, uint32 sourceWords, void *result, uint32 resultWords);
Err SimpleDecompress(const void *source, uint32 sourceWords, void *result, uint32 resultWords);


#ifdef __cplusplus
}
#endif


/*****************************************************************************/


#endif /* __MISC_COMPRESSION_H */
