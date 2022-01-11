/* @(#) autodocs.c 96/01/26 1.10 */

/**
|||	AUTODOC -class Compression -name CreateCompressor
|||	Creates a compression engine.
|||
|||	  Synopsis
|||
|||	    Err CreateCompressor(Compressor **comp, CompFunc cf,
|||	                         const TagArg *tags);
|||
|||	    Err CreateCompressorVA(Compressor **comp, CompFunc cf,
|||	                           uint32 tags, ...);
|||
|||	  Description
|||
|||	    Creates a compression engine. Once the engine is created, you can call
|||	    FeedCompressor() to have the engine compress the data you supply.
|||
|||	  Arguments
|||
|||	    comp
|||	        A pointer to a compressor variable, where a
|||	        handle to the compression engine can be put.
|||
|||	    cf
|||	        A data output function. Every word of
|||	        compressed data is sent to this function.
|||	        This function is called with two parameters:
|||	        one is a user-data value as supplied with
|||	        the COMP_TAG_USERDATA tag. The other is the
|||	        word of compressed data being output by the
|||	        compressor.
|||
|||	    tags
|||	        A pointer to an array of tag arguments
|||	        containing extra data for this function. See
|||	        below for a description of the tags
|||	        supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    COMP_TAG_WORKBUFFER (void *)
|||	        A pointer to a work buffer. This buffer is used by the
|||	        compressor to store state information. If this tag is not
|||	        supplied, the buffer is automatically allocated and freed by
|||	        the folio. To obtain the required size for the buffer, call the
|||	        GetCompressorWorkBufferSize() function. The buffer you supply
|||	        must remain valid until DeleteCompressor() is called. When you
|||	        supply a work buffer, this routine allocates no memory of its
|||	        own.
|||
|||	    COMP_TAG_USERDATA (void *)
|||	        A value that the compressor will pass to cf when it is called.
|||	        This value can be anything you want. For example, it can be a
|||	        pointer to a private data structure containing some context
|||	        such as a file handle. If this tag is not supplied, then
|||	        NULL is passed to cf when it is called.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code if the compression
|||	    engine could not be created. Possible error codes include:
|||
|||	    COMP_ERR_BADPTR
|||	        An invalid output function pointer or work buffer was supplied.
|||
|||	    COMP_ERR_BADTAG
|||	        An unknown tag was supplied.
|||
|||	    COMP_ERR_NOMEM
|||	        There was not enough memory to initialize the compressor.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Compression folio V24.
|||
|||	  Associated Files
|||
|||	    <misc/compression.h>, System.m2/Modules/compression
|||
|||	  See Also
|||
|||	    FeedCompressor(), DeleteCompressor(), GetCompressorWorkBufferSize(),
|||	    CreateDecompressor()
|||
**/

/**
|||	AUTODOC -class Compression -name CreateDecompressor
|||	Creates a decompression engine.
|||
|||	  Synopsis
|||
|||	    Err CreateDecompressor(Decompressor **comp, CompFunc cf,
|||	                           const TagArg *tags);
|||
|||	    Err CreateDecompressorVA(Decompressor **comp, CompFunc cf,
|||	                             uint32 tags, ...);
|||
|||	  Description
|||
|||	    Creates a decompression engine. Once the engine is created, you can call
|||	    FeedDecompressor() to have the engine decompress the data you supply.
|||
|||	  Arguments
|||
|||	    decomp
|||	        A pointer to a decompressor variable, where
|||	        a handle to the decompression engine can be put.
|||
|||	    cf
|||	        A data output function. Every word of
|||	        decompressed data is sent to this function.
|||	        This function is called with two parameters:
|||	        one is a user-data value as supplied with
|||	        the COMP_TAG_USERDATA tag. The other is the
|||	        word of decompressed data being output by
|||	        the decompressor.
|||
|||	    tags
|||	        A pointer to an array of tag arguments
|||	        containing extra data for this function. See
|||	        below for a description of the tags
|||	        supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    COMP_TAG_WORKBUFFER (void *)
|||	        A pointer to a work buffer. This buffer is used by the
|||	        decompressor to store state information. If this tag is not
|||	        supplied, the buffer is automatically allocated and freed by
|||	        the folio. To obtain the required size for the buffer, call the
|||	        GetDecompressorWorkBufferSize() function. The buffer you supply
|||	        must remain valid until DeleteDecompressor() is called. When you
|||	        supply a work buffer, this routine allocates no memory of its
|||	        own.
|||
|||	    COMP_TAG_USERDATA (void *)
|||	        A value that the decompressor will pass to cf when it is called.
|||	        This value can be anything you want. For example, it can be a
|||	        pointer to a private data structure containing some context
|||	        such as a file handle. If this tag is not supplied, then
|||	        NULL is passed to cf when it is called.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code if the decompression
|||	    engine could not be created. Possible error codes include:
|||
|||	    COMP_ERR_BADPTR
|||	        An invalid output function pointer or work buffer was supplied.
|||
|||	    COMP_ERR_BADTAG
|||	        An unknown tag was supplied.
|||
|||	    COMP_ERR_NOMEM
|||	        There was not enough memory to initialize the decompressor.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Compression folio V24.
|||
|||	  Associated Files
|||
|||	    <misc/compression.h>, System.m2/Modules/compression
|||
|||	  See Also
|||
|||	    FeedDecompressor(), DeleteDecompressor(), GetDecompressorWorkBufferSize(),
|||	    CreateCompressor()
|||
**/

/**
|||	AUTODOC -class Compression -name DeleteCompressor
|||	Deletes a compression engine.
|||
|||	  Synopsis
|||
|||	    Err DeleteCompressor(Compressor *comp);
|||
|||	  Description
|||
|||	    Deletes a compression engine previously created by CreateCompressor().
|||	    This flushes any data left to be output by the compressor and generally
|||	    cleans things up.
|||
|||	  Arguments
|||
|||	    comp
|||	        An active compression handle, as obtained
|||	        from CreateCompressor(). Once this call is
|||	        made, the compression handle becomes invalid
|||	        and can no longer be used.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if successful, or a negative error code if it fails.
|||	    Possible error codes include:
|||
|||	    COMP_ERR_BADPTR
|||	        An invalid compression handle was supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Compression folio V24.
|||
|||	  Associated Files
|||
|||	    <misc/compression.h>, System.m2/Modules/compression
|||
|||	  See Also
|||
|||	    CreateCompressor()
|||
**/

/**
|||	AUTODOC -class Compression -name DeleteDecompressor
|||	Deletes a decompression engine.
|||
|||	  Synopsis
|||
|||	    Err DeleteDecompressor(Decompressor *decomp);
|||
|||	  Description
|||
|||	    Deletes a decompression engine previously created by CreateDecompressor().
|||	    This flushes any data left to be output by the decompressor and generally
|||	    cleans things up.
|||
|||	  Arguments
|||
|||	    decomp
|||	        An active decompression handle, as obtained
|||	        from CreateDecompressor(). Once this call is
|||	        made, the decompression handle becomes
|||	        invalid and can no longer be used.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code if it fails. Possible
|||	    error codes include:
|||
|||	    COMP_ERR_BADPTR
|||	        An invalid decompression handle was supplied.
|||
|||	    COMP_ERR_DATAREMAINS
|||	        The decompressor thinks it is finished, but
|||	        there remains extra data in its buffers.
|||	        This happens when the compressed data is
|||	        somehow corrupt.
|||
|||	    COMP_ERR_DATAMISSING
|||	        The decompressor thinks that not all of the
|||	        compressed data was given to be
|||	        decompressed. This happens when the
|||	        compressed data is somehow corrupt.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Compression folio V24.
|||
|||	  Associated Files
|||
|||	    <misc/compression.h>, System.m2/Modules/compression
|||
|||	  See Also
|||
|||	    CreateCompressor()
|||
**/

/**
|||	AUTODOC -class Compression -name FeedCompressor
|||	Gives data to a compression engine.
|||
|||	  Synopsis
|||
|||	    Err FeedCompressor(Compressor *comp, const void *data,
|||	                       uint32 numDataWords);
|||
|||	  Description
|||
|||	    Gives data to a compressor engine for compression. As data is compressed,
|||	    the call back function supplied when the compressor was created is called
|||	    for every word of compressed data generated.
|||
|||	  Arguments
|||
|||	    comp
|||	        An active compression handle, as obtained from CreateCompressor().
|||
|||	    data
|||	        A pointer to the data to compress.
|||
|||	    numDataWords
|||	        The number of words of data being given to the compressor.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code if it fails. Possible
|||	    error codes include:
|||
|||	    COMP_ERR_BADPTR
|||	        An invalid compression handle was supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Compression folio V24.
|||
|||	  Associated Files
|||
|||	    <misc/compression.h>, System.m2/Modules/compression
|||
|||	  See Also
|||
|||	    CreateCompressor(), DeleteCompressor()
|||
**/

/**
|||	AUTODOC -class Compression -name FeedDecompressor
|||	Gives data to a decompression engine.
|||
|||	  Synopsis
|||
|||	    Err FeedDecompressor(Decompressor *decomp, const void *data,
|||	                         uint32 numDataWords);
|||
|||	  Description
|||
|||	    Gives data to the decompressor engine for decompression. As data is
|||	    decompressed, the call back function supplied when the decompressor was
|||	    created is called for every word of decompressed data generated.
|||
|||	  Arguments
|||
|||	    decomp
|||	        An active decompression handle, as obtained from CreateDecompressor().
|||
|||	    data
|||	        A pointer to the data to decompress.
|||
|||	    numDataWords
|||	        The number of words of compressed data being given to the decompressor.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code if it fail. Possible
|||	    error codes include:
|||
|||	    COMP_ERR_BADPTR
|||	        An invalid decompression handle was supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Compression folio V24.
|||
|||	  Associated Files
|||
|||	    <misc/compression.h>, System.m2/Modules/compression
|||
|||	  See Also
|||
|||	    CreateDecompressor(), DeleteDecompressor()
|||
**/

/**
|||	AUTODOC -class Compression -name GetCompressorWorkBufferSize
|||	Gets the size of the work buffer needed by a
|||	    compression engine.
|||
|||	  Synopsis
|||
|||	    int32 GetCompressorWorkBufferSize(const TagArg *tags);
|||
|||	    int32 GetCompressorWorkBufferSizeVA(uint32 tags, ...);
|||
|||	  Description
|||
|||	    Returns the size of the work buffer needed by a compression engine. You
|||	    can then allocate a buffer of that size and supply the pointer with the
|||	    COMP_TAG_WORKBUFFER tag when creating a compression engine. If the
|||	    COMP_TAG_WORKBUFFER tag is not supplied when creating a compressor, the
|||	    folio automatically allocates the memory needed for the compression
|||	    engine.
|||
|||	  Arguments
|||
|||	    tags
|||	        A pointer to an array of tag arguments
|||	        containing extra data for this function.
|||	        This must currently always be NULL.
|||
|||	  Return Value
|||
|||	    A positive value indicates the size of the work buffer needed in bytes,
|||	    while a negative value indicates an error. Possible error codes currently
|||	    include:
|||
|||	    COMP_ERR_BADTAG
|||	        An unknown tag was supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Compression folio V24.
|||
|||	  Associated Files
|||
|||	    <misc/compression.h>, System.m2/Modules/compression
|||
|||	  See Also
|||
|||	    CreateCompressor(), DeleteCompressor()
|||
**/

/**
|||	AUTODOC -class Compression -name GetDecompressorWorkBufferSize
|||	Gets the size of the work buffer needed by
|||	                               a decompression engine.
|||
|||	  Synopsis
|||
|||	    int32 GetDecompressorWorkBufferSize(const TagArg *tags);
|||
|||	    int32 GetDecompressorWorkBufferSizeVA(uint32 tags, ...);
|||
|||	  Description
|||
|||	    Returns the size of the work buffer needed by a decompression engine. You
|||	    can then allocate a buffer of that size and supply the pointer with the
|||	    COMP_TAG_WORKBUFFER tag when creating a decompression engine. If the
|||	    COMP_TAG_WORKBUFFER tag is not supplied when creating a decompressor, the
|||	    folio automatically allocates the memory needed for the decompression
|||	    engine.
|||
|||	  Arguments
|||
|||	    tags
|||	        A pointer to an array of tag arguments
|||	        containing extra data for this function.
|||	        This must currently always be NULL.
|||
|||	  Return Value
|||
|||	    A positive value indicates the size of the work buffer needed in bytes,
|||	    while a negative value indicates an error. Possible error codes currently
|||	    include:
|||
|||	    COMP_ERR_BADTAG
|||	        An unknown tag was supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Compression folio V24.
|||
|||	  Associated Files
|||
|||	    <misc/compression.h>, System.m2/Modules/compression
|||
|||	  See Also
|||
|||	    CreateDecompressor(), DeleteDecompressor()
|||
**/

/**
|||	AUTODOC -class Compression -name SimpleCompress
|||	Compresses some data in memory.
|||
|||	  Synopsis
|||
|||	    Err SimpleCompress(const void *source, uint32 sourceWords,
|||	                       void *result, uint32 resultWords);
|||
|||	  Description
|||
|||	    Compresses a chunk of memory to a different chunk of memory.
|||
|||	  Arguments
|||
|||	    source
|||	        A pointer to memory containing the data to be compressed.
|||
|||	    sourceWords
|||	        The number of words of data to compress.
|||
|||	    result
|||	        A pointer to where the compressed data is to be deposited.
|||
|||	    resultWords
|||	        The number of words available in the result buffer. If the
|||	        compressed data is larger than this size, an overflow will be
|||	        reported.
|||
|||	  Return Value
|||
|||	    If the return value is positive, it indicates the number of words
|||	    copied to the result buffer. If the return value is negative, it
|||	    indicates an error code. Possible error codes include:
|||
|||	    COMP_ERR_NOMEM
|||	        There was not enough memory to initialize the compressor.
|||
|||	    COMP_ERR_OVERFLOW
|||	        There was not enough room in the result buffer to hold all of the
|||	        compressed data.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in System.m2/Modules/compression V24.
|||
|||	  Associated Files
|||
|||	    <misc/compression.h>, System.m2/Modules/compression
|||
|||	  See Also
|||
|||	    SimpleDecompress()
|||
**/

/**
|||	AUTODOC -class Compression -name SimpleDecompress
|||	Decompresses some data in memory.
|||
|||	  Synopsis
|||
|||	    Err SimpleDecompress(const void *source, uint32 sourceWords,
|||	                         void *result, uint32 resultWords);
|||
|||	  Description
|||
|||	    Decompresses a chunk of memory to a different chunk of memory.
|||
|||	  Arguments
|||
|||	    source
|||	        A pointer to memory containing the data to be decompressed.
|||
|||	    sourceWords
|||	        The number of words of data to decompress.
|||
|||	    result
|||	        A pointer to where the decompressed data is to be deposited.
|||
|||	    resultWords
|||	        The number of words available in the result buffer. If the
|||	        decompressed data is larger than this size, an overflow will be
|||	        reported.
|||
|||	  Return Value
|||
|||	    If positive, returns the number of words copied to the result
|||	    buffer. If negative, returns an error code. Possible error codes
|||	    include:
|||
|||	    COMP_ERR_NOMEM
|||	        There was not enough memory to initialize the decompressor.
|||
|||	    COMP_ERR_OVERFLOW
|||	        There was not enough room in the result buffer to hold all of
|||	        the decompressed data.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in System.m2/Modules/compression V24.
|||
|||	  Associated Files
|||
|||	    <misc/compression.h>, System.m2/Modules/compression
|||
|||	  See Also
|||
|||	    SimpleCompress()
|||
**/

/* keep the compiler happy... */
extern int foo;
