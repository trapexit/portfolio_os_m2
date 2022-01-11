/*
 **
 **  @(#) EZFlixDecodeFrame.h 96/03/04 1.7
 **
	File:		EZFlixDecodeFrame.h

	Contains:	Header for Low-level Frame Decoder

	Written by:	Donn Denman and Greg Wallace

	To Do:
*/

/* */
/*	EZFlixDecodeFrame.h */
/* */

#ifndef __EZFlixDecodeFrame__
#define __EZFlixDecodeFrame__

#include "EZFlixXPlat.h"
#include "EZFlixDecodeSymbols.h"

#ifdef __cplusplus
extern "C" {
#endif
extern psErr EZFlixDecodeFrame(Ptr inEZFlixFrame,
						uint32 destRowBytes,
						Ptr destBaseAddr,
						uint32 destWidth,
						uint32 destHeight,
						Boolean use16BitPixels,
					    DecoderBuffers* buffers);
						
extern psErr AllocateBuffers(DecoderBuffers* buffers, uint32 imageWidth);
extern void FreeBuffers(DecoderBuffers* buffers);

extern void DecodeFlixLayer(uint32* pYAddress, 
					uint32 widthInLongs,
					uint32 height,
					uint32 layerNumber,
					unsigned char* stripBuffer,
					HuffmanDecodeState* decoderState);

#ifdef __cplusplus
}
#endif

#endif /* __EZFlixDecodeFrame__ */
