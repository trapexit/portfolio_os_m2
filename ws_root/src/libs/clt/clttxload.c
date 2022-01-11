
/******************************************************************************
**
**  @(#) clttxload.c 96/07/09 1.40
**
******************************************************************************/

#include <graphics/clt/clt.h>
#include <graphics/clt/clttxdblend.h>
#include <assert.h>
#include <stdio.h>

/* these should really be defined in clt.h, but are currently in texblend.h */

#define TX_WrapModeClamp            0
#define TX_WrapModeTile             1


Err CLT_CreateTxLoadCommandListMMDMA(CltTxLoadControlBlock *txLoadCB);
Err CLT_CreateTxLoadCommandListTexLoad(CltTxLoadControlBlock *txLoadCB);


/********************************************************************************
 * Pip Load Command Related Functions						*
 ********************************************************************************/
#define PIPLOADCMDLISTSIZE	(2 * 5)	/* in words */
#define PIPLOAD			0x00000200
#define TEPIPRAM		0x00046000



/*
 * Function:
 *      If txPipCB->pipCommandList is Null, a new command list buffer is allocated
 *      for loading the PIP. Otherwise, it is assumed the buffer to fill in the
 *      command list is provided.
 *      The command list is then filled in with the supplied data
 * Return Value:
 *      0 if there are no errors
 *      a negative number on error
 */
Err CLT_CreatePipCommandList(CltPipControlBlock* txPipCB)
{
	uint32 *cmdPtr;
	Err result;

	if (txPipCB->pipCommandList.data == NULL) {
		result = CLT_AllocSnippet(&txPipCB->pipCommandList, PIPLOADCMDLISTSIZE);
		if (result < 0)
			return result;

		txPipCB->pipCommandList.size = PIPLOADCMDLISTSIZE;
	}

	cmdPtr = txPipCB->pipCommandList.data;
	CLT_TXTLDCNTL(&cmdPtr, 0, 2, 0);
	CLT_TXTLDSRCADDR(&cmdPtr, (uint32)txPipCB->pipData);
	CLT_TXTLODBASE0(&cmdPtr, ((uint32)(TEPIPRAM) + (4*txPipCB->pipIndex)));
	CLT_TXTCOUNT(&cmdPtr, txPipCB->pipNumEntries<<2);
	CLT_TxLoad(&cmdPtr);

	assert (cmdPtr-txPipCB->pipCommandList.data == PIPLOADCMDLISTSIZE);

	return(0);
}


int32 CLT_ComputePipLoadCmdListSize(CltPipControlBlock* txPipCB)
{
	(void)&txPipCB;
	return PIPLOADCMDLISTSIZE;
}


/*****************************************************************************
 * Texture Load Command Related Functions					*
 *****************************************************************************/

#define MMDMA_SIZEOFTXTLOADCMDLIST	24 /* in words */

#define EXPANSIONFORMATMASK	0x00001FFF

#define TXTUVMAXMASK		0x03FF03FF

#define TXUVMASKMASK		0x000003ff

#define TXTUSHIFT	   		16

/*
 * Function:
 *      If txLoadCB->lcbCommandList is Null, a new command list buffer is allocated
 *      for loading the Texture. Otherwise, it is assumed the buffer to fill in the
 *      command list is provided.
 *      The command list is then filled in with the supplied data.  Runtime carving
 *      and loading of compressed textures are not supported by MMDMA loads.
 * Return Value:
 *      0 if there are no errors
 *      a negative number on error
 */
Err CLT_CreateTxLoadCommandListMMDMA(CltTxLoadControlBlock *txLoadCB)
{
	uint32 txSrcAddress, totalSize, lodBase[12], txMask, txSize;
	uint32 w,h;
	uint32 *cmdPtr;
	CltTxData *txdata = (CltTxData *)(txLoadCB->textureBlock);
	int32 i;
	uint32 umax, umask, vmax, vmask;
	Err result;

	if (txLoadCB->lcbCommandList.data == NULL) {
		result = CLT_AllocSnippet(&txLoadCB->lcbCommandList, MMDMA_SIZEOFTXTLOADCMDLIST);
		if (result < 0){
			return result;
		}
	}

	txLoadCB->lcbCommandList.size = MMDMA_SIZEOFTXTLOADCMDLIST;

	totalSize = 0;
	for ( i = 0; i < txLoadCB->numLOD; i++ ) {
		lodBase[i] = totalSize+txLoadCB->tramOffset;
		totalSize += txdata->texelData[i+txLoadCB->firstLOD].texelDataSize;
	}

	txSrcAddress = (uint32)txdata->texelData[txLoadCB->firstLOD].texelData;

	/* compute width and height of coarsest LOD in TRAM */

	w = (txdata->minX)<<(txdata->maxLOD-txLoadCB->numLOD-txLoadCB->firstLOD);
	h = (txdata->minY)<<(txdata->maxLOD-txLoadCB->numLOD-txLoadCB->firstLOD);

	/* compute width and height of finest LOD in TRAM */

	if ( txLoadCB->XWrap == TX_WrapModeClamp ) {
		umax = (w - 1);
		umask = 0x3FF;   /* Replicating texture masks */
	} else {
		umax = ((0x3FF) >> (txLoadCB->numLOD - 1));
		umask = ((w << (txLoadCB->numLOD - 1)) - 1);
	}

	if ( txLoadCB->YWrap  == TX_WrapModeClamp ) {
        vmax = (h - 1);
        vmask = 0x3FF;   /* Replicating texture masks */
    } else {
        vmax = ((0x3FF) >> (txLoadCB->numLOD - 1));
        vmask = ((h << (txLoadCB->numLOD - 1)) - 1);
    }

	txSize = ((umax << TXTUSHIFT) | vmax ) & TXTUVMAXMASK ;

	txMask = ((umask << TXTUSHIFT) | vmask ) & TXTUVMAXMASK ;

	cmdPtr = txLoadCB->lcbCommandList.data;

	*cmdPtr++ = CLT_WriteRegistersHeader(TXTEXPTYPE,1);
	*cmdPtr++ = (((CltTxData *)(txLoadCB->textureBlock))->expansionFormat) & EXPANSIONFORMATMASK;

	*cmdPtr++ = CLT_WriteRegistersHeader(TXTLDCNTL,1);
	*cmdPtr++ = CLT_SetConst(TXTLDCNTL,LOADMODE,MMDMA);

	*cmdPtr++ = CLT_WriteRegistersHeader(TXTLDSRCADDR,1);
	*cmdPtr++ = txSrcAddress;

	*cmdPtr++ = CLT_WriteRegistersHeader(TXTLODBASE0, 1);
	*cmdPtr++ = txLoadCB->tramOffset;

	*cmdPtr++ = CLT_WriteRegistersHeader(TXTCOUNT, 1);
	txLoadCB->tramSize = (totalSize+3) & ~3;	/* round size up to nearest word */
	*cmdPtr++ = txLoadCB->tramSize;

	CLT_TxLoad(&cmdPtr);

	/* Texture is loaded at this point */
	/* Now point the use command list to the current pointer */
	txLoadCB->useCommandList.data = cmdPtr;
	txLoadCB->useCommandList.size = txLoadCB->lcbCommandList.size -
	  (cmdPtr-txLoadCB->lcbCommandList.data);

	CLT_TxLoad4LOD(&cmdPtr, lodBase[0], lodBase[1], lodBase[2], lodBase[3]);

	*cmdPtr++ = CLT_WriteRegistersHeader(TXTUVMAX, 2);
	*cmdPtr++ = txSize;
	*cmdPtr++ = txMask;

	CLT_ClearRegister(cmdPtr, TXTADDRCNTL,
					   CLT_Bits(TXTADDRCNTL, LODMAX, CLT_Mask(TXTADDRCNTL, LODMAX)));
	CLT_SetRegister(cmdPtr, TXTADDRCNTL,
					 CLT_Bits(TXTADDRCNTL, LODMAX, txLoadCB->numLOD-1));

	assert (cmdPtr-txLoadCB->lcbCommandList.data == txLoadCB->lcbCommandList.size);

	return 0;
}


#define TEXLD_SIZEOFCMDSPERLOD		12	/* in words */
#define TEXLD_SIZEOFSHAREDCMDS		9
#define TEXLD_SIZEOFCOMPRESSIONCMDS	6

/*
 * Function:
 *      If txLoadCB->lcbCommandList is Null, a new command list buffer is allocated
 *      for loading the Texture. Otherwise, it is assumed the buffer to fill in the
 *      command list is provided.
 *      The command list is then filled in with the supplied data.  This cmd list
 *      supports runtime carving of textures
 * Return Value:
 *      0 if there are no errors
 *      a negative number on error
 */
Err CLT_CreateTxLoadCommandListTexLoad(CltTxLoadControlBlock *txLoadCB)
{
	uint32 txSrcAddress, totalSize, lodBase[12], txMask, txSize;
	uint32 startOffsetBits;
	uint32 totalLoadSize;
	uint32 w,h,lw,lh;
	uint32 *cmdPtr, *loadPtr, *usePtr=NULL, *startLoadPtr;
	CltTxData *txdata = (CltTxData *)(txLoadCB->textureBlock);
	int i;
	uint32 umax, umask, vmax, vmask;
	uint32 expFormat, formatSize;
	uint32 sizeShift;
	bool usingCompression;
	Err result;

	usingCompression = txdata->dci != NULL;
	if ( usingCompression &&
		 ( (txLoadCB->XSize != txdata->minX) ||
		   (txLoadCB->YSize != txdata->minY) ) )
        {
#ifdef UNIXSIM
            return(-1);
#else
            return CLT_ERR_NOSUPPORT;		/* Can't load sub-rect of compressed texture */
#endif
        }

	totalLoadSize = TEXLD_SIZEOFSHAREDCMDS +
		txLoadCB->numLOD * TEXLD_SIZEOFCMDSPERLOD +
		(usingCompression ? TEXLD_SIZEOFCOMPRESSIONCMDS : 0);

	if (txLoadCB->lcbCommandList.data == NULL){
		result = CLT_AllocSnippet(&txLoadCB->lcbCommandList, totalLoadSize);
		if (result < 0) {
			return result;
		}
		txLoadCB->lcbCommandList.size = totalLoadSize;
	} else {
		if (CLT_GetSize(&txLoadCB->lcbCommandList) < totalLoadSize) {
			CLT_FreeSnippet(&txLoadCB->lcbCommandList);
			result = CLT_AllocSnippet(&txLoadCB->lcbCommandList,totalLoadSize);
			if (result < 0)
				return result;
		}
	}

	txLoadCB->lcbCommandList.size = totalLoadSize;

	txLoadCB->useCommandList.allocated = 0;	/* shares mem. of load list */

	expFormat = ((CltTxData *)(txLoadCB->textureBlock))->expansionFormat;
	if (CLT_GetBits(TXTEXPTYPE,ISLITERAL,expFormat)) {
		formatSize = (CLT_GetBits(TXTEXPTYPE,CDEPTH,expFormat)*3 +
					  CLT_GetBits(TXTEXPTYPE,ADEPTH,expFormat) +
					  CLT_GetBits(TXTEXPTYPE,HASSSB,expFormat));
	} else {
		formatSize = (CLT_GetBits(TXTEXPTYPE,CDEPTH,expFormat) +
					  CLT_GetBits(TXTEXPTYPE,ADEPTH,expFormat) +
					  CLT_GetBits(TXTEXPTYPE,HASSSB,expFormat));
	}

	/* compute width and height of coarsest LOD in TRAM */
	w=(txdata->minX)<<(txdata->maxLOD-txLoadCB->numLOD-txLoadCB->firstLOD);
	h=(txdata->minY)<<(txdata->maxLOD-txLoadCB->numLOD-txLoadCB->firstLOD);
	if (usingCompression) {
		lw = w;
		lh = h;
	} else {
		lw = (txLoadCB->XSize) <<
			(txdata->maxLOD-txLoadCB->numLOD-txLoadCB->firstLOD);
		lh = (txLoadCB->YSize) <<
			(txdata->maxLOD-txLoadCB->numLOD-txLoadCB->firstLOD);
	}

	totalSize = 0;
	for ( i = 0; i < txLoadCB->numLOD; i++ ) {
		lodBase[i] = totalSize+txLoadCB->tramOffset;
		if (usingCompression) {
			uint32 pixelCount;
			sizeShift = txLoadCB->numLOD - i - 1;
			pixelCount = (h<<sizeShift) * (w<<sizeShift);
			totalSize += (( pixelCount*formatSize + 31 ) & ~31L) / 8;
		} else {
			totalSize += txdata->texelData[i+txLoadCB->firstLOD].texelDataSize;
		}
	}

	cmdPtr = txLoadCB->lcbCommandList.data;

	/* Describe the format of the texture */
	/* Note order of texelFormats is 1|0 3|2, not 0|1 2|3 !!! */
	if (usingCompression) {
		*cmdPtr++ = CLT_WriteRegistersHeader(TXTSRCTYPE01,7);
		*cmdPtr++ = ( ((((uint32)txdata->dci->texelFormat[1]) &
						EXPANSIONFORMATMASK) << 16) +
					  ((((uint32)(txdata->dci->texelFormat[0])) &
						EXPANSIONFORMATMASK)) );
		*cmdPtr++ = ( ((((uint32)txdata->dci->texelFormat[3]) &
						EXPANSIONFORMATMASK) << 16) +
					  ((((uint32)(txdata->dci->texelFormat[2])) &
						EXPANSIONFORMATMASK)) );
		*cmdPtr++ = expFormat & EXPANSIONFORMATMASK;
		*cmdPtr++ = txdata->dci->expColor[0];
		*cmdPtr++ = txdata->dci->expColor[1];
		*cmdPtr++ = txdata->dci->expColor[2];
		*cmdPtr++ = txdata->dci->expColor[3];
	} else {
		*cmdPtr++ = CLT_WriteRegistersHeader(TXTEXPTYPE,1);
		*cmdPtr++ = expFormat & EXPANSIONFORMATMASK;
	}

	loadPtr = cmdPtr;
	startLoadPtr = loadPtr;

	/* for each LOD, the following few writes are necessary */
	for (i=txLoadCB->firstLOD; i<txLoadCB->numLOD+txLoadCB->firstLOD; i++) {
		sizeShift = txLoadCB->numLOD - i - 1;
		startOffsetBits = ((txLoadCB->YOffset<<sizeShift) *
						   (txdata->minX<<sizeShift) +
						   (txLoadCB->XOffset<<sizeShift)) * formatSize;
		txSrcAddress = (uint32)txdata->texelData[i].texelData +
			(startOffsetBits >> 3);

		cmdPtr = loadPtr;

		*cmdPtr++ = CLT_WriteRegistersHeader(TXTLDCNTL,1);
		*cmdPtr++ = CLT_SetConst(TXTLDCNTL,LOADMODE,TEXTURE) |
			CLT_Bits(TXTLDCNTL,COMPRESSED,usingCompression) |
			CLT_Bits(TXTLDCNTL,SRCBITOFF,startOffsetBits & 0x7);

		*cmdPtr++ = CLT_WriteRegistersHeader(TXTLODBASE0, 1);
		*cmdPtr++ = lodBase[i];

		*cmdPtr++ = CLT_WriteRegistersHeader(TXTLDSRCADDR,3);
		*cmdPtr++ = txSrcAddress;

		/* Support for carving/compression is done with the next 2 writes */
		*cmdPtr++ = ( usingCompression ?
					  (h<<sizeShift)*(w<<sizeShift) :
					  (lh<<sizeShift) );
		*cmdPtr++ = ((((lw<<sizeShift) * formatSize) << TXTUSHIFT) |
					 ((w<<sizeShift) * formatSize));

		CLT_TxLoad(&cmdPtr);

		/* Now point the use command list to the current pointer */
		if (i==0) {
			usePtr = cmdPtr + (cmdPtr - startLoadPtr) * (txLoadCB->numLOD-1);
			txLoadCB->useCommandList.data = usePtr;
		}

		if (i==txLoadCB->numLOD-1) {
			assert (cmdPtr == txLoadCB->useCommandList.data);
		} else {
			loadPtr = cmdPtr;	/* Save spot in load list */
		}

		cmdPtr = usePtr;

		/* Next line is icky, but it lets us use the CLT macros */
		*cmdPtr++ = CLT_WriteRegistersHeader(TXTLODBASE0, 1) + i*4;
		*cmdPtr++ = lodBase[i];

		usePtr = cmdPtr;
	}	/* For loop done for each LOD */

	/* compute width and height of finest LOD in TRAM */
	if ( txLoadCB->XWrap == TX_WrapModeClamp ) {
		umax = lw - 1;
		umask = 0x3FF;   /* Replicating texture masks */
	} else {
		umax = ((0x3FF) >> (txLoadCB->numLOD - 1));
		umask = ((lw << (txLoadCB->numLOD - 1)) - 1);
	}

	if ( txLoadCB->YWrap  == TX_WrapModeClamp ) {
        vmax = lh - 1;
        vmask = 0x3FF;   /* Replicating texture masks */
    } else {
        vmax = ((0x3FF) >> (txLoadCB->numLOD - 1));
        vmask = ((lh << (txLoadCB->numLOD - 1)) - 1);
    }

	txSize = ((umax << TXTUSHIFT) | vmax ) & TXTUVMAXMASK ;
	txMask = ((umask << TXTUSHIFT) | vmask ) & TXTUVMAXMASK ;

	*cmdPtr++ = CLT_WriteRegistersHeader(TXTUVMAX, 2);
	*cmdPtr++ = txSize;
	*cmdPtr++ = txMask;

	CLT_ClearRegister(cmdPtr, TXTADDRCNTL,
					   CLT_Bits(TXTADDRCNTL, LODMAX, CLT_Mask(TXTADDRCNTL, LODMAX)));
	CLT_SetRegister(cmdPtr, TXTADDRCNTL,
					 CLT_Bits(TXTADDRCNTL, LODMAX, txLoadCB->numLOD-1));

	txLoadCB->useCommandList.size = cmdPtr - txLoadCB->useCommandList.data;

	assert (cmdPtr-txLoadCB->lcbCommandList.data == totalLoadSize);

	return(0);
}


Err CLT_CreateTxLoadCommandList(CltTxLoadControlBlock* txLoadCB)
{
	CltTxData *txdata = (CltTxData *)(txLoadCB->textureBlock);

	if ( ((txLoadCB->XSize != txdata->minX) ||
		  (txLoadCB->YSize != txdata->minY)) ||
		 (((CltTxData*)txLoadCB->textureBlock)->dci != NULL) ) {
#ifdef NUPUPSIM
#ifdef BUILD_BDA1_1
		if (KB_FIELD(kb_Flags) & KB_BDA1_1)
			return GFX_ErrorNotImplemented;
#endif
#endif
		return CLT_CreateTxLoadCommandListTexLoad(txLoadCB);
	} else
		return CLT_CreateTxLoadCommandListMMDMA(txLoadCB);
}


/* Returns the size that will be needed to do a texture load for a given
 * texture block.  Returns a negative number if an error occurred.
 */
int32 CLT_ComputeTxLoadCmdListSize(CltTxLoadControlBlock* txLoadCB)
{
	CltTxData *txdata = (CltTxData *)(txLoadCB->textureBlock);

	if ( (txdata && ((txLoadCB->XSize != txdata->minX) ||
                         (txLoadCB->YSize != txdata->minY))) ||
             (txLoadCB->textureBlock &&
              (((CltTxData*)txLoadCB->textureBlock)->dci != NULL)) ) {
#ifdef NUPUPSIM
#ifdef BUILD_BDA1_1
		if (KB_FIELD(kb_Flags) & KB_BDA1_1)
			return GFX_ErrorNotImplemented;
#endif
#endif
		return TEXLD_SIZEOFSHAREDCMDS +
			txLoadCB->numLOD * TEXLD_SIZEOFCMDSPERLOD +
			( ((CltTxData*)txLoadCB->textureBlock)->dci ?
			  TEXLD_SIZEOFCOMPRESSIONCMDS : 0 );
	} else
		return MMDMA_SIZEOFTXTLOADCMDLIST;
}
