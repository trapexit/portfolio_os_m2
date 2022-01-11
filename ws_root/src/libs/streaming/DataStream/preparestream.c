/******************************************************************************
**
**  @(#) preparestream.c 95/12/13 1.20
**
******************************************************************************/

#include <kernel/debug.h>		/* CHECK_NEG(), ERROR_RESULT_STATUS() */
#include <kernel/types.h>
#include <kernel/mem.h>			/* Alloc/FreeMem variations */

#include <streaming/dsblockfile.h>
#include <streaming/datastream.h>	/* DSDataBufPtr, dserror.h, dsstreamdefs.h */
#include <streaming/preparestream.h>


/* The "nice" alignment modulo needed to optimize DMA and CPU cache access.
 * [TBD] Get this value at run-time from the OS */
#define NICE_BUFFER_ALIGNMENT		32


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Startup -name CreateBufferList
|||	Allocates a stream buffer list for use by the streamer.
|||	
|||	  Synopsis
|||	
|||	    DSDataBufPtr CreateBufferList(int32 numBuffers, int32 bufferSize)
|||	
|||	  Description
|||	
|||	    CreateBufferList allocates a data streamer input buffer list. It first
|||	    allocates (via AllocMemAligned() with MEMTYPE_TRACKSIZE) one big memory
|||	    block for the array of DSDataBuf structures and the array of data
|||	    buffers. It ensures that each data buffer is aligned for optimal DMA and
|||	    CPU cache access. It then links up the DSDataBuf structures (by their
|||	    'next' fields) and points each at its aligned buffer (by its
|||	    'streamData' field).
|||	    
|||	    The resulting buffer list can then be passed to NewDataStream(). The
|||	    streamer will use these buffers for reading blocks of data from the
|||	    input device.
|||	    
|||	    To later deallocate the buffer list, call FreeMemTrack(), passing the
|||	    result of CreateBufferList--the first DSDataBufPtr.
|||	    
|||	    This procedure allocates the entire buffer list in one contiguous
|||	    memory block. You don't have to do it this way. You could allocate each
|||	    buffer separately, or use another memory manager, or whatever. You just
|||	    need to how to deallocate them, in this case: FreeMemTrack(firstBp).
|||	
|||	  Notes
|||	
|||	    Unlike previous versions of the data streamer, the DSDataBuf structures
|||	    are separate from the data buffers themselves. So anytime the streamer
|||	    is quiescent--that is, after a stop-flush operation completes--you can
|||	    temporarily reuse the buffer memory (e.g. for transition effects
|||	    buffers) as long as you don't smash the DSDataBuf structures.
|||	    
|||	    If CreateBufferList() returns non-NULL firstBp, firstBp->streamData
|||	    points to the buffer memory. The buffer memory will be at least
|||	    numBuffers * bufferSize bytes long.
|||	
|||	  Arguments
|||	
|||	    numBuffers
|||	        The number of buffers to allocate. This must be at least 2.
|||	
|||	    bufferSize
|||	        The data size of each buffer, in bytes. This MUST AGREE with the
|||	        stream's block size. Typical buffer sizes are 32K to 96K bytes.
|||	
|||	  Return Value
|||	
|||	    A pointer to the first buffer in the list, suitable for passing to
|||	    NewDataStream() and later to FreeMemTrack().
|||	    Or NULL, if numBuffers < 2.
|||	    Or NULL, if CreateBufferList() couldn't allocate the memory.
|||	
|||	  Implementation
|||	
|||	    Library code in libds.a.
|||	
|||	  Associated Files
|||	
|||	    <streaming/datastream.h>, <streaming/dserror.h>,
|||	    <streaming/preparestream.h>
|||	
|||	  See Also
|||	
|||	    AllocMemAligned(), FreeMemTrack(), NewDataStream()
|||	
 ******************************************************************************/
DSDataBufPtr	CreateBufferList(int32 numBuffers, int32 bufferSize)
	{
	int32			totalHeaderSize, eachBufferSize;
	DSDataBufPtr	firstBp;
	DSDataBufPtr	bp;
	int32			bufferNum;
	uint8			*bufferAddr;
	
	if ( numBuffers < 2 )
		return NULL;
	
	/* Compute the size needed for an array DSDataBuf[numBuffers], plus
	 * padding out to a "nice" boundary, plus an array of numBuffers
	 * data buffers, each padded out to a "nice" boundary. */
	totalHeaderSize = ALLOC_ROUND(numBuffers * sizeof(DSDataBuf),
		NICE_BUFFER_ALIGNMENT);
	eachBufferSize  = ALLOC_ROUND(bufferSize, NICE_BUFFER_ALIGNMENT);
	
	/* Try to allocate the needed memory. */
	firstBp = (DSDataBufPtr)AllocMemAligned(
		totalHeaderSize + numBuffers * eachBufferSize,
		MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL,
		NICE_BUFFER_ALIGNMENT);
	if ( firstBp == NULL )
		return NULL;

	/* Split the big allocation into an array DSDataBuf[numBuffers] and an
	 * array of data buffers with "nice" alignment. Link them all together. */
	for ( bp = firstBp, bufferAddr = (uint8 *)firstBp + totalHeaderSize,
			bufferNum = numBuffers;
			bufferNum > 0;
			++bp, bufferAddr += eachBufferSize, --bufferNum )
		{
		bp->next = bp + 1;
		bp->streamData = bufferAddr;
		}
	bp[-1].next = NULL;

	/* Return a pointer to the first buffer in the list  */
	return firstBp;
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Startup -name FindAndLoadStreamHeader
|||	Load a stream file's stream header chunk into memory.
|||	
|||	  Synopsis
|||	
|||	    Err FindAndLoadStreamHeader(DSHeaderChunkPtr headerPtr,
|||	        char *fileName)
|||	
|||	  Description
|||	
|||	    Open a stream file, check that it starts with a stream header chunk,
|||	    load the header into memory, and close the stream file. (The streamer's
|||	    DataAcq will later reopen the file.)
|||	
|||	    Despite its name, this procedure does no searching.
|||	
|||	  Arguments
|||	
|||	    headerPtr
|||	        Points to a DSHeaderChunk struct allocated by the client.
|||	        FindAndLoadStreamHeader() will store the stream's header structure
|||	        (if there is one) into *headerPtr.
|||	
|||	    fileName
|||	        The name of the DS stream file to read a header from.
|||	
|||	  Return Value
|||	
|||	    An error code. In particular, the value kDSHeaderNotFound (defined in
|||	    <streaming/dserror.h>) means FindAndLoadStreamHeader() didn't find a
|||	    stream header in this file. In that case, FindAndLoadStreamHeader() will
|||	    print a warning message and the application can substitute default
|||	    settings.
|||	
|||	  Assumes
|||	
|||	    A stream file's stream header chunk begins at the first byte of the file.
|||	
|||	  Implementation
|||	
|||	    Library code in libds.a.
|||	
|||	  Associated Files
|||	
|||	    <streaming/datastream.h>, <streaming/dserror.h>,
|||	    <streaming/preparestream.h>
|||	
 ******************************************************************************/
Err	FindAndLoadStreamHeader(DSHeaderChunkPtr headerPtr, char *fileName)
	{
	Err					status;
	BlockFile			blockFile;
	Item				ioReqItem = -1;
	DSHeaderChunkPtr	hdrBufferPtr;
	char				*buffer = NULL;
	int32				deviceBlockSize, bufferSize = 0;

	/* Init */
	blockFile.fDevice	= -1;

	/* Open the specified file */
	status = OpenBlockFile(fileName, &blockFile);
	FAIL_NEG("FindAndLoadStreamHeader OpenBlockFile", status);
	
	if ( GetBlockFileSize(&blockFile) <= 0 )
		{
		status = kDSBadStreamSizeErr;
		goto FAILED;
		}
	
	deviceBlockSize = GetBlockFileBlockSize(&blockFile);
	bufferSize = ((sizeof(*headerPtr) - 1 + deviceBlockSize) / deviceBlockSize)
		* deviceBlockSize;

	/* Allocate a buffer to hold some of the first data in the stream  */
	status = kDSNoMemErr;	/* the status code to use if alloc fails */
	buffer = AllocMemAligned(bufferSize, MEMTYPE_DMA, NICE_BUFFER_ALIGNMENT);
	FAIL_NIL("FindAndLoadStreamHeader buffer", buffer);
	
	/* Create an I/O request item */
	status = ioReqItem = CreateBlockFileIOReq(blockFile.fDevice, 0);
	FAIL_NEG("FindAndLoadStreamHeader CreateBlockFileIOReq", status);

	/* Read the first block from the stream file */
	status = AsynchReadBlockFile( 
				&blockFile, 				/* block file */
				ioReqItem, 					/* ioreq Item */
				buffer,			 			/* place to READ data into */
				bufferSize,					/* amount of data to READ */
				0);							/* offset of data in the file */
	FAIL_NEG("FindAndLoadStreamHeader AsynchReadBlockFile", status);

	/* Wait for the read to complete */
	status = WaitReadDoneBlockFile(ioReqItem);
	FAIL_NEG("FindAndLoadStreamHeader WaitReadDoneBlockFile", status);

	/* "Search" the buffer for the header information and copy it to the
	 * client's memory.
	 * ==== ASSUMES: THE HEADER IS THE FIRST CHUNK IN THE FILE. ==== */
	hdrBufferPtr = (DSHeaderChunkPtr)buffer;
	if ( hdrBufferPtr->chunkType == HEADER_CHUNK_TYPE )
		{
		*headerPtr = *hdrBufferPtr;
		
		status = kDSNoErr;
		if ( (headerPtr->streamBlockSize / deviceBlockSize) * deviceBlockSize
				!= headerPtr->streamBlockSize )
			status = kDSBadStreamSizeErr;
		}
	else
		{
		status = kDSHeaderNotFound;
		PERR(("Warning: DS stream header chunk not found in the file \"%s\". "
			"Using default settings.\n", fileName));
		}

FAILED:	/* do cleanup after failure or normal completion */
	DeleteItem(ioReqItem);
	CloseBlockFile(&blockFile);
	FreeMem(buffer, bufferSize);

	return status;
	}

