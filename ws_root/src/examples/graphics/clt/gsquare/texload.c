
/******************************************************************************
**
**  @(#) texload.c 96/05/01 1.15
**
******************************************************************************/

#include <kernel/types.h>
#include <kernel/mem.h>
#include <file/fileio.h>
#include <graphics/clt/clt.h>
#include <graphics/clt/clttxdblend.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "texload.h"

static bool doTextureLoad;

/* load new IFF texture chunks */

static Err parse_utf_chunk(tx_chunk_data *txc)
{
	uint32 			chunk_type ;
	uint32 			chunk_length, l ;
	char 			*p, *txcbuf;
	uint32 			form_type ;

	txc->hdr_data = NULL; txc->pip_data = NULL; txc->tex_data = NULL;
	txc->hdr_size = 0; txc->pip_size = 0; txc->tex_size = 0;
	txc->tab_data = NULL; txc->dab_data = NULL;
	txc->dci_data = NULL; txc->ldr_data = NULL;
	txc->tab_size = 0; txc->dab_size = 0; txc->dci_size = 0; txc->ldr_size = 0;

	chunk_type = *((int32 *)txc->buff);

	if ( chunk_type != ID_FORM ) {
		printf("ERROR: Illegal form type.  Exiting\n");
		exit (1);
    }

	txcbuf = txc->buff;
	txcbuf += 4;
	chunk_length = *((int32 *)(txcbuf)); txcbuf += 4;

	if ( chunk_length == 0xfffffffe ) {
		chunk_length = *((int32 *)(txcbuf+4));
		txcbuf += 8;
	}

	form_type = *((uint32 *)txcbuf);
	if ( form_type != ID_TXTR ) {
		printf("ERROR: Not a texture chunk.  Exiting.\n");
		exit(1);
	}

	txcbuf += 4; chunk_length -= 4;

	for ( p = txcbuf; p-txcbuf<chunk_length; ) {
		chunk_type = *((int32 *)p);
		p += 4;
		l = *((uint32 *)(p)); p += 4;
		if ( l == 0xfffffffe ) {
        	l = *((int32 *)(p+4));
        	p += 8;
    	}
		switch ( chunk_type ) {
		case ID_M2TX:
			txc->hdr_data = p;
			txc->hdr_size = l;
			break;
		case ID_M2PI:
			txc->pip_data = p;
			txc->pip_size = l;
			break;
		case ID_M2TD:
			txc->tex_data = p;
			txc->tex_size = l;
			break;
		case ID_M2TA:
			txc->tab_data = p;
			txc->tab_size = l;
			break;
		case ID_M2DB:
			txc->dab_data = p;
			txc->dab_size = l;
			break;
		case ID_M2LR:
			txc->ldr_data = p;
			txc->ldr_size = l;
			break;
		case ID_M2CI:
			txc->dci_data = p;
			txc->dci_size = l;
			break;
		}

		p += l;
	}

	return 0;
}

static Err process_dci(tx_chunk_data *txc, TextureSnippets* snips)
{
	CltTxLoadControlBlock* lcb = snips->lcb;
	CltTxDCI*	dci;
	char*		p;

	if (((CltTxData*)(snips->lcb->textureBlock))->dci == NULL) {
		((CltTxData*)(snips->lcb->textureBlock))->dci =
			AllocMem(sizeof(CltTxDCI),MEMTYPE_ANY);
		if (((CltTxData*)snips->lcb->textureBlock)->dci == NULL)
			return -1;
	}

	dci = ((CltTxData*)snips->lcb->textureBlock)->dci;

	if (lcb == NULL) {
		printf("ERROR: No lcb in process_dci\n");
		return -1;
	}

	p = txc->dci_data;

	GET_FIELD(p, dci->texelFormat[0], uint16);
	GET_FIELD(p, dci->texelFormat[1], uint16);
	GET_FIELD(p, dci->texelFormat[2], uint16);
	GET_FIELD(p, dci->texelFormat[3], uint16);
	GET_FIELD(p, dci->expColor[0], uint32);
	GET_FIELD(p, dci->expColor[1], uint32);
	GET_FIELD(p, dci->expColor[2], uint32);
	GET_FIELD(p, dci->expColor[3], uint32);
	TOUCH(p);

	doTextureLoad = 1;

	return 0;
}

static Err process_load_rect(tx_chunk_data *txc, TextureSnippets* snips)
{
	CltTxLoadControlBlock* lcb = snips->lcb;
    uint32 *p = (uint32 *)txc->ldr_data;
	uint32 numLoadRects;
	M2TXRect *prect;

	numLoadRects = *p++;

	if ( numLoadRects > 1 ) {
		printf("Support for more than 1 loadrect is not implemented.\n");
		return -1;
	}

	if (lcb == NULL) {
		snips->lcb = AllocMem(sizeof(CltTxLoadControlBlock), MEMTYPE_ANY);
		lcb = snips->lcb;
		if (lcb == NULL) {
			printf("ERROR: Couldn't allocate CltTxLoadControlBlock\n");
			return -1;
		}
		CLT_InitSnippet( &lcb->lcbCommandList );
		CLT_InitSnippet( &lcb->useCommandList );
	}

	prect = (M2TXRect *)p;

	lcb->textureBlock = snips->txdata->texelData;
	lcb->XWrap = prect->XWrapMode;
	lcb->YWrap = prect->YWrapMode;
	lcb->firstLOD = prect->FirstLOD;
	lcb->numLOD = prect->NLOD;
	lcb->XOffset = prect->XOff;
	lcb->YOffset = prect->YOff;
	lcb->XSize = prect->XSize;
	lcb->YSize = prect->YSize;
	lcb->tramOffset = 0;
	lcb->tramSize = 16384;
	doTextureLoad = 1;

	return 0;
}

static Err process_tab_attributes(tx_chunk_data *txc, TextureSnippets* snips)
{
	uint32 *p = (uint32 *)txc->tab_data;
	uint32 n = (txc->tab_size>>2);
	uint32 attr, val;
	Err err;

	/* ignore reserved word */

	p++; n--;

	while ( n > 0 ) {
		attr = *p++;
		val = *p++;
		n -= 2;
		err = CLT_SetTxAttribute(&snips->tab, attr, val);
		if ( err != 0 )
			return err;
	}
	return 0;
}

static Err process_dab_attributes(tx_chunk_data *txc, TextureSnippets* snips)
{
	uint32 *p = (uint32 *)txc->dab_data;
	uint32 n = (txc->dab_size>>2);
	uint32 attr, val;
	Err err;

	/* ignore reserved word */

	p++; n--;

	while ( n > 0 ) {
		attr = *p++;
		val = *p++;
		n -= 2;
		err = CLT_SetDblAttribute(&(snips->dab), attr, val);
		if ( err != 0 )
            return err;
	}

    return 0;
}

static Err process_texture(tx_chunk_data *txc, TextureSnippets* snips)
{
	char 			*p;
    unsigned char   numLOD;
    uint32          lodDataCnt;
	uint32			*lodoff;
	char			*pd;
    int32           i;
	uint32			sz;
	CltTxData*		txdata;
	CltTxLoadControlBlock* lcb = snips->lcb;

	/* now create texture */

	if ( txc->hdr_data == NULL ) {
		printf("ERROR: No header data\n");
		return -1;
	}
	p = txc->hdr_data;

	if (snips->txdata == NULL) {
		snips->txdata = AllocMem( sizeof(CltTxData), MEMTYPE_ANY );
		if (snips->txdata == NULL) {
			printf("ERROR: Couldn't allocate CltTxData\n");
			return -1;
		}
	}
	txdata = snips->txdata;

	p += 4; /* Reserved field */
    GET_FIELD(p, txdata->flags, uint32);
    GET_FIELD(p, txdata->minX, uint16);
    GET_FIELD(p, txdata->minY, uint16);
    GET_FIELD(p, txdata->expansionFormat, uint16);
	if ( txHasColor(txdata->expansionFormat)) {
		if (txIsLiteral(txdata->expansionFormat)){
        	txdata->bitsPerPixel = 3 * txGetColorDepth(txdata->expansionFormat);
    	} else {
            txdata->bitsPerPixel = txGetColorDepth(txdata->expansionFormat);
        }
    } else txdata->bitsPerPixel = 0;
    if (txHasAlpha(txdata->expansionFormat))
        txdata->bitsPerPixel += txGetAlphaDepth(txdata->expansionFormat);
    if (txHasSSB(txdata->expansionFormat))
        txdata->bitsPerPixel++;
	txdata->dci = NULL;
    numLOD = *p;

	/* get texture data */
    if ( txc->tex_data == NULL )
		return 0; /* no texture data, invalid texture */
	p = txc->tex_data;
    GET_FIELD(p, lodDataCnt, uint16);
    p += 2; /* reserved area */
	lodoff = (uint32 *)p;
    if (lodDataCnt != numLOD) {
		printf("WARNING: # LOD in hdr = %d, in texel data = %d\n",
			   numLOD, lodDataCnt);
    }
	txdata->maxLOD = numLOD;
	pd = AllocMem( txc->tex_size+numLOD*sizeof(uint32)-4, MEMTYPE_ANY );
	if ( pd == NULL ) {
		printf("ERROR: Couldn't allocate buffer for texel data\n");
		return -1;
	}
	txdata->texelData = (CltTxLOD*)pd;
	pd += numLOD*sizeof(CltTxLOD);
	p += numLOD*sizeof(uint32);
	/* copy texel data */
	memcpy(pd, p, txc->tex_size-numLOD*sizeof(uint32)-4);
	/* set LOD info */

	for ( i = 0; i < numLOD-1; i++ ) {
		sz = lodoff[i+1]-lodoff[i];
		txdata->texelData[i].texelDataSize = sz;
		txdata->texelData[i].texelData = pd;
		pd += sz;
	}

	txdata->texelData[numLOD-1].texelDataSize = txc->tex_size-lodoff[numLOD-1];
	txdata->texelData[numLOD-1].texelData = pd;

	if (lcb == NULL) {
		snips->lcb = AllocMem(sizeof(CltTxLoadControlBlock), MEMTYPE_ANY);
		lcb = snips->lcb;
		if (lcb == NULL) {
			printf("ERROR: Couldn't allocate CltTxLoadControlBlock\n");
			return -1;
		}
		CLT_InitSnippet( &lcb->lcbCommandList );
		CLT_InitSnippet( &lcb->useCommandList );
		lcb->textureBlock = txdata;
		lcb->firstLOD = 0;
		lcb->numLOD = txdata->maxLOD;
		lcb->XWrap = 0;		/* clamp */
		lcb->YWrap = 0;		/* clamp */
		lcb->XSize = txdata->minX;
		lcb->YSize = txdata->minY;
		lcb->XOffset = 0;
		lcb->YOffset = 0;
		lcb->tramOffset = 0;
		lcb->tramSize = 0;
		doTextureLoad = 1;
	}

	return 0;
}

static Err process_pip(tx_chunk_data *txc, TextureSnippets* snips)
{
	CltPipControlBlock*		pip = snips->pip;
	uint32 					pipNumEntries, pipIndex;
	char		 			*p;

	/* process pip if it has one */

	p = txc->pip_data;
	GET_FIELD(p, pipIndex, uint32);
	TOUCH(p);
	pipNumEntries = (txc->pip_size-8)>>2;

	if ( pip == NULL ) {
		snips->pip = AllocMem( sizeof(CltPipControlBlock), MEMTYPE_ANY );
		pip = snips->pip;
		if (pip == NULL) {
			printf("ERROR: Couldn't allocate CltPipControlBlock\n");
			return -1;
		}
	}
	pip->pipData = AllocMem(pipNumEntries*sizeof(uint32), MEMTYPE_ANY);
	if (pip->pipData == NULL) {
		printf("Couldn't allocate a %d-entry PIP\n", pipNumEntries);
		return -1;
	}
	bcopy( txc->pip_data+8, pip->pipData, txc->pip_size-8 );
	pip->pipNumEntries = pipNumEntries;
	pip->pipIndex = pipIndex;
	CLT_InitSnippet(&snips->pip->pipCommandList);
	CLT_CreatePipCommandList(snips->pip);

	return 0;
}

static Err process_texchunkdata(tx_chunk_data *txc, TextureSnippets* snips)
{
	Err				err;

	doTextureLoad = 0;

	if ( txc->tex_data ) {
		if ((err = process_texture(txc, snips)) != 0) {
			return err;
		}
	}

	if ( txc->pip_data ) {
		if ((err = process_pip(txc, snips)) != 0) {
			return err;
		}
	}

	if ( txc->tab_data ) {
		if ((err = process_tab_attributes(txc, snips)) != 0)
			return err;
	}

	if ( txc->dab_data ) {
        if ((err = process_dab_attributes(txc, snips)) != 0)
			return err;
    }

	if ( txc->ldr_data ) {
        if ((err = process_load_rect(txc, snips)) != 0)
			return err;
	}

	if ( txc->dci_data ) {
		if ((err = process_dci(txc, snips)) != 0)
            return err;
    }

	if (doTextureLoad)
		CLT_CreateTxLoadCommandList(snips->lcb);

	return 0;
}

static Err parse_utf_file(tx_chunk_data *txc, char* filename)
{
	Err			    err;
	RawFile*		rawfile;
	FileInfo		info;
	uint32			filesize, bytes_read;

	txc->buff = NULL;

    if ((err = OpenRawFile(&rawfile,filename,FILEOPEN_READ)) < 0) {
		printf("ERROR: Couldn't open texture file.\n");
		exit(1);
    }

	GetRawFileInfo( rawfile, &info, sizeof(info) );
	filesize = info.fi_ByteCount;

    txc->buff = (char *) AllocMem(filesize,MEMTYPE_ANY);
	txc->buffsize = filesize;

    if (txc->buff == NULL) {
		printf("WARNING: Can't allocate buffer for texture file: Out of memory\n");
		err = -1;
		goto error;
    }

    bytes_read = ReadRawFile(rawfile, txc->buff, filesize);

    if (bytes_read < filesize) {
		printf("WARNING: Unexpected end of file.\n");
        err = -1;
		goto error;
    }

	CloseRawFile(rawfile);

	if ( parse_utf_chunk(txc) == 0 )
		return 0;

error:

	if ( txc->buff ) {
		FreeMem(txc->buff, filesize);
		txc->buff = NULL;
	}

	return err;
}

Err tex_load(TextureSnippets* snips, char* filename)
{
	tx_chunk_data  txc;
	Err             err;

	if ((err = parse_utf_file(&txc, filename)) != 0 )
		return err;

	if ((err = process_texture(&txc, snips)) != 0 ) {
		return err;
	}

	return 0;
}

Err LoadTexture(TextureSnippets* snips, char* filename)
{
	Err			    err;
	tx_chunk_data  txc;

	if ( (err = parse_utf_file(&txc, filename)) != 0 )
		goto error;

	if ( (err = process_texchunkdata(&txc, snips)) != 0 )
		goto error;

error:

	if ( txc.buff ) {
		FreeMem(txc.buff, txc.buffsize);
		txc.buff = NULL;
	}

	return err;
}

void UseTxLoadCmdLists( GState* gs, TextureSnippets* snips )
{
	uint32 dataSize;
	uint32* p;

	/* Load PIP if one's there */
	if (snips->pip) {
		dataSize = CLT_GetSize(&snips->pip->pipCommandList);
		GS_Reserve(gs, dataSize);
		p = (uint32*)gs->gs_ListPtr;
		CLT_CopySnippetData(&p, &snips->pip->pipCommandList);
		gs->gs_ListPtr += dataSize;
	}
	/* Load texel data into TRAM */
	if (snips->lcb) {
		dataSize = CLT_GetSize(&snips->lcb->lcbCommandList);
		GS_Reserve(gs, dataSize);
		p = (uint32*)gs->gs_ListPtr;
		CLT_CopySnippetData(&p, &snips->lcb->lcbCommandList);
		gs->gs_ListPtr += dataSize;
	}
	/* Load texture mapper attribs and dest blend attribs (TAB & DAB) */
	dataSize = CLT_GetSize(&snips->tab) + CLT_GetSize(&snips->dab);
	GS_Reserve(gs, dataSize);
	p = (uint32*)gs->gs_ListPtr;
	CLT_CopySnippetData(&p, &snips->tab);
	CLT_CopySnippetData(&p, &snips->dab);
	gs->gs_ListPtr += dataSize;
}
