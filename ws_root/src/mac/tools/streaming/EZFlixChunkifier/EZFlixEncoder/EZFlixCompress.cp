/*
 **
 **  @(#) EZFlixCompress.cp 96/03/06 1.4
 **
	File:		EZFlixCompress.cp

	Written by:	Donn Denman and Greg Wallace

	To Do:
*/

/* EZFlixCompress.cp -- Internal implementation of EZFlix compressor.
 *  formerly Greg.c
 *
 *	File:		EZFlixCompress.c
 *				by Greg Wallace
 *
 *	Copyright:	© 1994 by The 3DO Company. All rights reserved.
 *				This material constitutes confidential and proprietary
 *				information of the 3DO Company and shall not be used by
 *				any Person or for any purpose except as expressly
 *				authorized in writing by the 3DO Company.
 *
 *	History:
 *	 12/14/94	gkw		Changed UVMAX from 126 to 119. (overflows on MD2 logo).
 *	 					Changed WTEST from 80 to 84. (red->yellow wraps on logo).
 *						Removed all the entropy calc stuff. Was for dev't only.
 *	 12/05/94	gkw		Added more Compress() tests to prevent R/G/B wraparound.
 *						Changed Downsample() filter from 16 to 8 taps.
 *	 11/15/94	crm		Introduced verbose flag for low-level compressor.
 *						Cleaned up error handling.
 *	 11/09/94	crm		Compression info removed from first data chunk and
 *						placed in EZFL header chunk.
 *
 */

#include <Types.h>
#include <Memory.h>
#include <StandardFile.h>
#include <errors.h>
#include <Quickdraw.h>
#include <lowmem.h>
#if ! GENERATINGPOWERPC
#include <Sane.h>
#endif
#include <Math.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "EZFlixCompress.h"
#include "EZFlixEncoder.h"
#include "EZFlixCodec.h"
#include "Quantizers.h"



/* Following includes & defs are for GW encoding stuff */

#include <stdlib.h>		/* needed for malloc */

/* CONTROLS */

#define kStuck 4
#define DO_WRAP_TEST

/* MACROS */

/* Returns true if x is an even multiple of n */
#define IsMultipleOf(x,n)	(((long)(x) & (long)(~(-(n)))) ? false : true)

/* When compiling for debugging, error messages are printed */ 
#if DEBUG && MOVIETOEZFLIX
#define DIAGNOSE(x)			{									\
							printf("Error ("__FILE__"): ");		\
							printf x;							\
							}
#define NOTIFY(x)												\
							{									\
							printf x;							\
							}
#else
#define DIAGNOSE(x)
#define NOTIFY(x)
#endif

#define ABS(x)				(x<0?(-x):(x))


/* Function forward declarations */

static void RGBtoYUV( EZFlixParamsPtr params, unsigned char *Ybase,
					  unsigned char *Ubase, unsigned char *Vbase,
					  unsigned char encodingShape );
static void Downsample( long height, long width, unsigned char *UVbase, unsigned char *Filtbase,
						Boolean IsClassic );

static void Compress( EZFlixParamsPtr params, // globals
				const unsigned char* sourcebase, // input pixels 
				unsigned char* decompbase, // temp yMap
			  	unsigned long* streambase, // output bytes
			  	unsigned long** outStream,
			  	long width, long height, 
			  	MultiQuantizer* quantizer,
			  	long layerNumber);


long GWcompress_frame(EZFlixParamsPtr params, 
						unsigned long* compressedData, 
						unsigned long* resultingSize)
	{
		/* ============== IMPORTANT NOTES ===============
		(1) This code is presently hard-wired to assume that:
			(a) source->PPMWidth  = source->width,
			(b) source->PPMHeight = source->height,
			(c) source->depth     = 255.
		============================================== */
	
		PPMHeader*		source	 = &params->pixelMapHeader;
		long			destHeight   = source->height;
		long			destWidth    = source->width;
		long			height = (destHeight + 15) & ~15;
		long			width = (destWidth + 15) & ~15;
		long			uvheight = height/4;
		long			uvwidth  = width/4;
		long			buffsize;
		FlixFrameHeader* frameHeader = (FlixFrameHeader*) compressedData;
		unsigned long * frameHeaderPtr = (unsigned long *) frameHeader;
		unsigned char *	tbuff = NULL;
		unsigned char	*Ybase,*Ubase,*Vbase,*Filtbase,*Decbase,*UDecbase,*VDecbase;
		unsigned long	*Sbase, *USbase, *VSbase;
		long			result;

		result = kEZFlixNoError;
		*resultingSize = 0;
		
		// Fill in the Frame Header with everything we know
		frameHeader->headerVersion = kHeaderVersion0;
		frameHeader->Y2HuffmanTree = kUnUsedEncoding;

		if (params->quality == kFixedTwoBitQuality) /* 2Bit Opera */
		{
			frameHeader->UVHuffmanTree = kFixedTwoBit;
			frameHeader->Y1HuffmanTree = kFixedTwoBit;
		}
		else if (params->quality == kHuffman3Quality) /* Huffman3 */
		{
		#ifdef DO_DITHERING
			frameHeader->UVHuffmanTree = kUnballanced4Tree;
			frameHeader->Y1HuffmanTree = kDithered3x2Tree;
		#else
			frameHeader->UVHuffmanTree = kUnballanced3Tree;
			frameHeader->Y1HuffmanTree = kUnballanced3Tree;
		#endif
		}
		else if (params->quality == kHuffman4Quality) /* Huffman4 */
		{
			frameHeader->UVHuffmanTree = kUnballanced4Tree;
			frameHeader->Y1HuffmanTree = kUnballanced4Tree;
		}
		else if (params->quality == kHuffman5Quality) /* Huffman5 */
		{
			frameHeader->UVHuffmanTree = kUnballanced5p5Tree;
			frameHeader->Y1HuffmanTree = kUnballanced5p5Tree;
		}
		else if (params->quality == kHuffman6Quality) /* Huffman6 */
		{
			frameHeader->UVHuffmanTree = kMostlyBallanced6p5Tree;
			frameHeader->Y1HuffmanTree = kMostlyBallanced6p5Tree;
		}
		
		// create a quantizer for our quality level of compression.
		MultiQuantizer quantizer(frameHeader->UVHuffmanTree);
		
		// stuff the symbols used by the quantizer into the frame header.
		unsigned long* compressedDataOut = quantizer.StuffSymbolMap(frameHeader->UVHuffmanTree,
																	frameHeader->Y1HuffmanTree,
																	frameHeader->Y2HuffmanTree,
																	frameHeader->QuantizeSymbols);
		frameHeader->chunk1Offset =  compressedDataOut - frameHeaderPtr; // offset in longs
				
		
		// It's not so easy to calculate the compressed size
		//	since the bits could be Huffman encoded.
		// Make a buffer big enough for the worst case.
		// Then output the compressed blocks into the buffer.
		// This will require us to do U and V first.
		// We will have to switch the decode ordering in the
		//  decompressor to make this work.

		/* Allocate working space if it has not already been done for us */
		if (params->tempBuf == NULL) 					/* Provide enough space for:	*/
		{
			buffsize  = 3*height*width;					/*   - Y,U,V frames;			*/
			buffsize += (height+TAPS-4)*(width+TAPS-4);	/*   - U/V-filtering frame;		*/
			buffsize += (height+1)*width;				/*   - copy of Y decoder fbuffer.	*/
			buffsize += 2*(uvheight+1)*uvwidth;			/*   - copy of U,V decoder fbuffers.	*/
			buffsize += (height*width)+(2*uvheight*uvwidth);
														/* - non-FLC stream buffer */
			tbuff = (unsigned char*) NewPtrSysClear( buffsize );
			if (tbuff == NULL) {
				tbuff = (unsigned char*) NewPtrClear( buffsize );
				if (tbuff == NULL) {
					NOTIFY(("Cannot allocate low-level compressor buffer (%ld bytes)\n", buffsize));
					result = kEZFlixNoMemory;
					goto Return;
				}
			}
		}

		{
			/* Set up working space pointers */
			if (tbuff == NULL)						/* if we were given space, then use it */
				Ybase    = params->tempBuf;
			else
				Ybase    = tbuff;					/* else use the temporary buffer */
			Ubase    = Ybase    + (height*width);
			Vbase    = Ubase    + (height*width);
			Filtbase = Vbase    + (height*width);
			Decbase  = Filtbase + ((height+TAPS-4)*(width+TAPS-4));
			UDecbase = Decbase  + ((height+1)*width);
			VDecbase = UDecbase + ((uvheight+1)*uvwidth);
			Sbase    = (unsigned long *)( VDecbase  + ((uvheight+1)*uvwidth) );
		
			/* Do processing */
			RGBtoYUV( params, Ybase, Ubase, Vbase, frameHeader->Y1HuffmanTree );
			Boolean isClassic = (frameHeader->UVHuffmanTree == kFixedTwoBit);
			Downsample( height, width, Ubase, Filtbase, isClassic );
			Downsample( height, width, Vbase, Filtbase, isClassic );
	
			USbase = Sbase + (width*height/4);
			params->compCount = 1;
			params->uDecBase = UDecbase;
			params->vDecBase = VDecbase;
			Compress( params, Ubase, UDecbase, USbase, &compressedDataOut, uvwidth, 
						uvheight, &quantizer, 1 );
			frameHeader->chunk2Offset = compressedDataOut - frameHeaderPtr; // chunk2Offset in longs
			VSbase = USbase + (uvwidth*uvheight/4);
			params->compCount = 2;
			Compress( params, Vbase, VDecbase, VSbase, &compressedDataOut, uvwidth, 
						uvheight, &quantizer, 2 );
			frameHeader->chunk3Offset = compressedDataOut - frameHeaderPtr; // chunk3Offset in longs
			// create a quantizer for Y compression.
			MultiQuantizer quantizerY(frameHeader->Y1HuffmanTree, true);
			params->compCount = 3;
			Compress( params, Ybase, Decbase, Sbase, &compressedDataOut, width, 
						height, &quantizerY, 3 );
	
			*resultingSize = (char*) compressedDataOut - (char*) compressedData; // size output
			result = noErr;
		}
Return:
		/* Return number of bytes in compressed frame (or error value) */
		if (tbuff != NULL) 
			DisposePtr((Ptr) tbuff);
		return result;
	}



	static void
	RGBtoYUV( EZFlixParamsPtr params,
			  unsigned char *Ybase, unsigned char *Ubase, unsigned char *Vbase,
			  unsigned char /*encodingShape*/ )
	{
		PPMHeader*		source	 = &params->pixelMapHeader;
		long 			height   = (source->height + 15) & ~15;
		long 			width    = (source->width + 15) & ~15;
		RGBTriple 		*rgb;
		long			i,j, samples=height*width;
		
#ifndef __MWERKS__
		const long		preproc[256] = {	
											12,13,14,14,15,16,17,17,18,19,19,20,20,21,22,22,
											23,23,24,24,25,26,26,27,27,28,28,29,29,30,30,31,
											31,32,32,33,33,34,34,35,35,35,36,36,37,37,38,38,
											39,39,40,40,41,41,41,42,42,43,43,44,44,44,45,45,
											46,46,47,47,47,48,48,49,49,50,50,50,51,51,52,52,
											52,53,53,54,54,54,55,55,56,56,56,57,57,58,58,58,
											59,59,60,60,60,61,61,62,62,62,63,63,64,64,64,65,
											65,65,66,66,67,67,67,68,68,69,69,69,70,70,70,71,
											71,72,72,72,73,73,73,74,74,74,75,75,76,76,76,77,
											77,77,78,78,78,79,79,80,80,80,81,81,81,82,82,82,
											83,83,83,84,84,85,85,85,86,86,86,87,87,87,88,88,
											88,89,89,89,90,90,91,91,91,92,92,92,93,93,93,94,
											94,94,95,95,95,96,96,96,97,97,97,98,98,98,99,99,
											99,100,100,100,101,101,101,102,102,102,103,103,103,104,104,104,
											105,105,105,106,106,106,107,107,107,108,108,108,109,109,109,110,
											110,110,111,111,111,112,112,112,113,113,113,114,114,114,115,115
										};
#else
		long			preproc[256];
		#include		"preprocArray.c"
#endif

		unsigned char	*Y_ptr,*U_ptr,*V_ptr;
	  #if FP_RGB	
		double			r,g,b,y,u,v;
	  #else
		long			r,g,b,y,u,v;
	  #endif
		
		Y_ptr = Ybase;	U_ptr = Ubase;	V_ptr = Vbase;

		SpinCursorOnce();

#ifdef	USE_POW_FUNCTION
		/* Build pre-processing look-up table */
		for (i=0; i<256; i++) {	
			temp = i;
			temp = (pow(temp/255.0, GAMMA)) * 255.0;	/* gamma pre-emphasis */
			temp = ( (CEILING-FLOOR) * temp / 255.0 ) + FLOOR;	/* dynamic range compression */
			preproc[i] = temp/2.0;						/* quantize to 7 bits */
		}
#else

#endif
		
		/* Convert the pixels */
		rgb = source->pixels;
		for (j=0; j<height; j++) {
			SpinCursorOnce();
			for (i=0; i<width; i++) {
				r = preproc[ rgb->part.red   ];
				rgb->part.red = (unsigned char) r;
				g = preproc[ rgb->part.green ];
				rgb->part.green = (unsigned char) g;
				b = preproc[ rgb->part.blue  ];
				rgb->part.blue = (unsigned char) b;
				rgb++;
		
				/* Cheapo (7-bit) YUV */
				
			  #if FP_RGB
				y = ((2.0*r)+(4.0*g)+b)/7.0;		/* range = [FLOOR,CEILING]/2 */
				u = (b - y);						/* range = [-6/7,6/7]*(CEILING-FLOOR)/2 */
				v = (r - y);						/* range = [-5/7,5/7]*(CEILING-FLOOR)/2 */
				
				if ( (u>63.0) || (u<-63.0) ) {
					if (params->verbose) {
						NOTIFY(("\n\t\t->Limiting color saturation from Val1=%.3f to 63 at pixel (%ld,%ld) ",
								u, i, j));
					}
					if (u > 63.0) u =  63.0;
					if (u <-63.0) u = -63.0;
				}
				if ( (v>63.0) || (v<-63.0) ) {
					if (params->verbose) {
						NOTIFY(("\n\t\t->Limiting color saturation from Val2=%.3f to 63 at pixel (%ld,%ld) ",
								v, i, j));
					}
					if (v > 63.0) v =  63.0;
					if (v <-63.0) v = -63.0;
				}
				*(Y_ptr++) = y + 0.5;				/* Convert to positive integers */
				*(U_ptr++) = u + 0.5 + 64.0;		/* 0.5 is for rounding */
				*(V_ptr++) = v + 0.5 + 64.0;
		
			  #else
				y = ( ( ((r<<1)+(g<<2)+b)<<1 ) + 7 )/14;	/* integer-arith rounding */
				u = (b - y);
				v = (r - y);
				
				if ( (u > 63) || (u < -63) ) {
					if (params->verbose) {
						NOTIFY(("\n\t\t->Limiting color saturation from Val1=%ld to 63 at pixel (%ld,%ld) ",
								u, i, j));
					}
					if (u > 63) u =  63;
					if (u <-63) u = -63;
				}
				if ( (v > 63) || (v < -63) ) {
					if (params->verbose) {
						NOTIFY(("\n\t\t->Limiting color saturation from Val2=%ld to 63 at pixel (%ld,%ld) ",
								v, i, j));
					}
					if (v > 63) v =  63;
					if (v <-63) v = -63;
				}	
				*(Y_ptr++) = (unsigned char) y;						/* Convert to positive integers */
				*(U_ptr++) = (unsigned char) u + 64;
				*(V_ptr++) = (unsigned char) v + 64;
			  #endif			
			}
		}
	}


	static void
	Downsample( long height, long width, unsigned char *UVbase, unsigned char *Filtbase, 
				Boolean IsClassic )
	{
		unsigned char	*UVptr, *Fptr1, *Fptr2, *ptr;
		long			bigwidth = width+TAPS-4;	/* padded width */
		long			i,j,m,n;
		long			temp, vbuff[TAPS];
		long			norm = 256;					/* filter divisor */
#ifdef __MWERKS__
		long			tap[TAPS];
		tap[0] = 1;
		tap[1] = 13;
		tap[2] = 43;
		tap[3] = 71;
		tap[4] = 71;
		tap[5] = 43;
		tap[6] = 13;
		tap[7] = 1;
#else
		const long		tap[TAPS] = {1,13,43,71,71,43,13,1};
#endif		
		
		SpinCursorOnce();

		/* Initialize pointers */
		UVptr = UVbase;
		Fptr1 = Filtbase + ( (TAPS-4)/2*bigwidth );
		
		/* Copy U or V image to larger buffer with replicated edges */
		for (j=0; j<height; j++) {
			for (n=0; n<(TAPS-4)/2; n++)	/* replicate left column */
				*Fptr1++ = *UVptr;
			for (i=0; i<width; i++)			/* copy row */
				*Fptr1++ = *UVptr++;
			for (n=0; n<(TAPS-4)/2; n++)	/* replicate right column */
				*Fptr1++ = *(UVptr-1);
		}
		for (j=0; j<(TAPS-4)/2; j++)		/* replicate bottom rows */
			for (i=0; i<bigwidth; i++) {
				*Fptr1 = *(Fptr1 - bigwidth);
				Fptr1++;
			}
		Fptr2 = Filtbase;
		for (j=0; j<(TAPS-4)/2; j++) {		/* replicate top rows */
			Fptr1 = Filtbase + ((TAPS-4)/2*bigwidth);
			for (i=0; i<bigwidth; i++)
				*Fptr2++ = *Fptr1++;
		}
	
		/* Filter and downsample */
		UVptr = UVbase;
		long widthDiv4 = width>>2;
		for (j=0; j<height; j+=4) {			/* For each set of 4 rows */
			Fptr1 = Filtbase + (j*bigwidth);
			for (i=0; i<widthDiv4; i++) {			/* For each 4x4 region */
				
				/* Apply filter to each row to get intermediate vertical column */
				ptr = Fptr1;
				for (m=0; m<TAPS; m++) {
					vbuff[m]=0;
					for (n=0; n<TAPS; n++) 
						vbuff[m] += tap[n] * (*ptr++);
					ptr += bigwidth - TAPS;
					// vbuff[m] /= norm;  // Compiler bug in SC, SCpp causes this to fail
					vbuff[m] = vbuff[m] / norm;
				}
				Fptr1 += 4;
				/* Apply filter to intermediate column to get output sample */
				temp=0;
				for (n=0; n<TAPS; n++) 
					temp += tap[n] * vbuff[n];
				temp /= norm;
				if (IsClassic) {
					if (temp > UVMAX) {
					//	printf("\t\tFilter value %ld at j=%ld i=%ld.\n", temp, j, i);
					//	fprintf(fp_t, "\t\tFilter value %ld at j=%ld i=%ld.\n", temp, j, i);
						temp = UVMAX;
					}
					if (temp < UVMIN) {
					//	printf("\t\tFilter value %ld at j=%ld i=%ld.\n", temp, j, i);
					//	fprintf(fp_t, "\t\tFilter value %ld at j=%ld i=%ld.\n", temp, j, i);
						temp = UVMIN;
					}
				} else {
					if (temp > UVMAXM2) {
						temp = UVMAXM2;
					}
					if (temp < UVMINM2) {
						temp = UVMINM2;
					}
				}
				*(UVptr++) = (unsigned char) temp;
			}
		}	
	}


	//============================
	// Compress - Compressor Main
	//============================
	
	static void
	Compress( EZFlixParamsPtr params, // globals
				const unsigned char* sourcebase, // input pixels 
				unsigned char* decompbase, // temp yMap to decompress into
			  	unsigned long* streambase, // output values as bytes
			  	unsigned long** outStream, // encoded output bitstream
			  	long width, long height,
			  	MultiQuantizer* quantizer,
			  	long layerNumber)
	{
		PPMHeader*		source	 = &params->pixelMapHeader;
		unsigned char*	UDecbase = params->uDecBase;
		unsigned char*	VDecbase = params->vDecBase;
		unsigned char	*py;

		long			i,k,m,n;
		unsigned char	a[4],b[4],c[4],p[4];
		long			ps[4],e[4],y[4],ys[4];
		unsigned char	eq[4];
		unsigned long	Av,Bv,Cv,EQv,Pv,T1,T2,Yv;
		long			samples=height*width;
		
		RGBTriple		*rgb;
		unsigned long	uvRowNum, uvRowPel0, uvCurrPel;
		long			r, g, bl, u, v, rs, gs, bs;
		long			flag1, flag2, Uflow, Oflow, save=0;
	
		/* DO_EQ_EXPANSION */
		long expansionCounter = 0;
		unsigned long	minTrigger = MIN_EQ_H3Y_VALUE;
		unsigned long	maxTrigger = MAX_EQ_H3Y_VALUE;
		if (layerNumber != 3) {
			minTrigger = MIN_EQ_H3UV_VALUE;
			maxTrigger = MAX_EQ_H3UV_VALUE;
		}
		/* DO_EQ_EXPANSION */

		/* DO_DITHERING */
		long ditherCounter = 0;
		long ditherExtras[3];
		/* DO_DITHERING */

		unsigned char encodingShape = quantizer->EncodingShape();
		Boolean IsClassic = (encodingShape == kFixedTwoBit);

		uint32			decodeMask = DECODEMASK;
		uint32			shiftFactor = 1;
		
		/* Compression setup */
		py = decompbase;
		for (i=0; i<width; i++) 				/* Write imaginary reference row */
			*(py+i) = REFVAL;
		py += width;
		
		/* Main compression Loop */
		i=-1;
		for (m=0; m<height/4; m++) {			/** Compute next wave-row of vectors */
			
			SpinCursorOnce();

			Yv  = REFVAL;							/* Initializations */
			Yv |= Yv <<  8;
			Yv |= Yv << 16;
			Cv  = Yv;
			
			// four-line beginning
			for (n=0; n<3; n++) {				/** Compute first 3 partial IDPCM vectors **/
				i++;
				Av = Yv;							/* Compute partial p vector */
				Bv = Cv;
				Cv = *(py+i-width);
				Cv = (Av>>8) | (Cv<<24);
				T1 = Av + Cv;
				T2 = Av + Cv - Bv;
				Pv = (T1>>1) + T2;
				
				/* DO_DITHERING */
				ditherCounter = ~ditherCounter;
				ditherExtras[n] = ditherCounter;
				/* DO_DITHERING */

				for (k=0; k<n+1; k++) {				/* Compute eq for each vector element */
				
					a[k] = (unsigned char) ( Av >> ((3-k)*8) ) & 0x000000FF;
					b[k] = (unsigned char) ( Bv >> ((3-k)*8) ) & 0x000000FF;
					c[k] = (unsigned char) ( Cv >> ((3-k)*8) ) & 0x000000FF;
					p[k] = (unsigned char) ( Pv >> ((3-k)*8) ) & 0x000000FF;
				
					ps[k] = ( (a[k]+c[k]) >>1 ) + a[k]+c[k]-b[k];

						/* compute the error value */
					e[k] = *( sourcebase+i+(k*(width-1)) ) - (ps[k]>>shiftFactor);
					
						/* quantize the error value */
					eq[k] = quantizer->Quantize(e[k], ditherCounter, layerNumber);					
				}

				if (n==0) {							/* Create & save partial eq vectors */
					EQv  = eq[0];
					*(streambase+(m*width)) = EQv;
					EQv <<= 24;
				}
				if (n==1) {
					EQv  = eq[0] <<  8;
					EQv |= eq[1];
					*(streambase+(m*width)+1) = EQv;
					EQv <<= 16;
				}
				if (n==2) {
					EQv  = eq[0] << 16;
					EQv |= eq[1] <<  8;
					EQv |= eq[2];
					*(streambase+(m*width)+2) = EQv;
					EQv <<= 8;
				}

				Yv  = (Pv + EQv) >> shiftFactor;				/* Compute decompressed-sample vector */
				Yv &= decodeMask;
				
				for (k=0; k<n+1; k++) {				/* Store & compare to scalar value */
					
					y[k]  = ( Yv >> ((3-k)*8) ) & 0x000000FF;
					*(py+i+(k*(width-1))) = (unsigned char) y[k];
					ys[k] = (ps[k] + (long) eq[k]) >> shiftFactor;
		
					if (ys[k] < 0) {
						if (params->verbose) {
							NOTIFY(("\n\t\t->Possible underflow at pixel (%ld,%ld): scal=%ld, vec=%ld ",
								(i%width)-k,(i/width)+k,ys[k],y[k]));
						}
					}
					else if (ys[k] > 127) {
						if (params->verbose) {
							NOTIFY(("\n\t\t->Possible overflow at pixel (%ld,%ld): scal=%ld, vec=%ld ",
								(i%width)-k,(i/width)+k,ys[k],y[k]));
						}
					}
					else if ( ABS(y[k]-ys[k]) > 2) {
						if (params->verbose) {
							NOTIFY(("\n\t\t->Possible mismatch at pixel (%ld,%ld): scal=%ld, vec=%ld ",
								(i%width)-k,(i/width)+k,ys[k],y[k]));
						}
					}
				
				#ifdef DO_WRAP_TEST
				#ifndef IGNORE_OVERFLOW_AT_START_OF_LINE

					// Check for overflow and underflow problems
	
					if (! IsClassic)  // all M2 compressors
					{
						// M2 Wrap Detection
						{
							long adjusting=1; Uflow=0; Oflow=0; uint32 pinMax = 0; uint32 pinMin = 0;
							long terminate;
							uint32 stuck = 0;		// Make sure we never get stuck in the while loop;
							while (adjusting) {
								
								stuck++;
								
								// assume we are finally done with our adjustments
								adjusting = 0;
								
								// Use the classic check for rgb mismatch in Y values
								if (params->compCount == 3) {
								
									/* compute current u/v pixel index */
									uvRowNum  = m;
									uvRowPel0 = (uvRowNum*width)>>2;
									uvCurrPel = uvRowPel0 + (((i%width)-k)>>2);
								
									/* compute current r, g, b */
									u  = *(UDecbase + (width>>2) + uvCurrPel);
									u += 64;
									v  = *(VDecbase + (width>>2) + uvCurrPel);
									v += 64;
									r  = y[k] + v;
									r  = (r & decodeMask);
									g  = y[k] - (v>>1) - (u>>2) + 96;
									g  = (g & decodeMask);
									bl = y[k] + u;
									bl = (bl & decodeMask);
	
									/* fetch the RGB source values */
									rgb = source->pixels + i + (k*(width-1));
									rs  = rgb->part.red;
									gs  = rgb->part.green;
									bs  = rgb->part.blue;
								
									if ( (bl-bs>WTEST) || (g-gs>WTEST) || (r-rs>WTEST) ) { /* U'flow case */
										if ( ABS(e[k]) < FTEST) {
											eq[k] = quantizer->BumpQuantUp(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
										}
									}
									else if ( (bs-bl>WTEST) || (gs-g>WTEST) || (rs-r>WTEST) ) { /* O'flow case */
										if ( ABS(e[k]) < FTEST) { 
											eq[k] = quantizer->BumpQuantDown(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
										}
									}
									else if ((g<=YMINM2) || (r<=YMINM2) || (bl<=YMINM2)) {
											eq[k] = quantizer->BumpQuantUp(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
									}
									else if ((g>=YMAXM2) || (r>=YMAXM2)) {
											eq[k] = quantizer->BumpQuantDown(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
									}
								}
								
								// check for overflow in Y, U and V values
								{
									unsigned char pixel = (unsigned char) ((e[k] + (ps[k]>>shiftFactor)) & decodeMask);
									unsigned char approx = (unsigned char) ((eq[k] + (ps[k]>>shiftFactor)) & decodeMask);
									
									// approximation overflowed?
									if (pixel - approx > WTEST) {
										eq[k] = quantizer->BumpQuantDown(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
									}
									
									// approximation underflowed?
									else if (approx - pixel > WTEST) {
										eq[k] = quantizer->BumpQuantUp(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
									}

									// Handle the multiple-problems case -- we exit with a corrected RGB
									if (stuck > kStuck) break;  
								}
	
								if (adjusting) {	/* if any eq[k] changed, redo EQv, Yv, and y[k] */
									EQv = *(streambase+(m*width)+n);	/* Retrieve, merge, & save partial eq vectors */
									if (n==0) {				/* Update partial eq vectors */
										EQv  = eq[0];
										*(streambase+(m*width)) = EQv;
										EQv <<= 24;
									}
									if (n==1) {
										EQv  = eq[0] <<  8;
										EQv |= eq[1];
										*(streambase+(m*width)+1) = EQv;
										EQv <<= 16;
									}
									if (n==2) {
										EQv  = eq[0] << 16;
										EQv |= eq[1] <<  8;
										EQv |= eq[2];
										*(streambase+(m*width)+2) = EQv;
										EQv <<= 8;
									}
									Yv  = (Pv + EQv) >> shiftFactor;
									Yv &= decodeMask;
									y[k]  = ( Yv >> ((3-k)*8) ) & 0x000000FF;
									*(py+i+(k*(width-1))) = (unsigned char) y[k];
								}
							}
						}
					}
				#endif // IGNORE_OVERFLOW_AT_START_OF_LINE
				#endif // DO_WRAP_TEST
				}
			}
			
			// INNER LOOP OF THE COMPRESSOR
			// four-line middle
			for (n=3; n<width; n++) {			/** Compute next IDPCM vector **/
				i++;
				Av = Yv;							/* Compute next p vector */
				Bv = Cv;
				Cv = *(py+i-width);
				Cv = (Av>>8) | (Cv<<24);
				T1 = Av + Cv;
				T2 = Av + Cv - Bv;
				Pv = (T1>>1) + T2;
				
				/* DO_DITHERING */
				ditherCounter = ~ditherCounter;
				/* DO_DITHERING */

				for (k=0; k<4; k++) {				/* Compute eq for each vector element */
				
					a[k] = (unsigned char) ( Av >> ((3-k)*8) ) & 0x000000FF;
					b[k] = (unsigned char) ( Bv >> ((3-k)*8) ) & 0x000000FF;
					c[k] = (unsigned char) ( Cv >> ((3-k)*8) ) & 0x000000FF;
					p[k] = (unsigned char) ( Pv >> ((3-k)*8) ) & 0x000000FF;
				
					ps[k] = ( (a[k]+c[k]) >>1 ) + a[k]+c[k]-b[k];

						/* compute the error value */
					e[k] = *( sourcebase+i+(k*(width-1)) ) - (ps[k]>>shiftFactor);
					
						/* quantize the error value */
					eq[k] = quantizer->Quantize(e[k], ditherCounter, layerNumber);					
				}
				
				EQv  = eq[0] << 24;					/* Create & save eq vector */
				EQv |= eq[1] << 16;
				EQv |= eq[2] <<  8;
				EQv |= eq[3];
				*(streambase+(m*width)+n) = EQv;	// output resulting eq.
				Yv  = (Pv + EQv) >> shiftFactor;	/* Compute decompressed-sample vector */
				Yv &= decodeMask;
				
				// Check for overflow and underflow problems
				for (k=0; k<4; k++) {				/* Store & compare to scalar value */
					
					y[k]  = ( Yv >> ((3-k)*8) ) & 0x000000FF;
					*(py+i+(k*(width-1))) = (unsigned char) y[k]; // remember quantized value
		
					if (params->verbose) {
						ys[k] = (ps[k] + (long) eq[k]) >> shiftFactor;
						if (ys[k] < 0) {
							NOTIFY(("\n\t\t->Possible underflow at pixel (%ld,%ld): scal=%ld, vec=%ld ",
								(i%width)-k,(i/width)+k,ys[k],y[k]));
						}
						else if (ys[k] > 127) {
							NOTIFY(("\n\t\t->Possible overflow at pixel (%ld,%ld): scal=%ld, vec=%ld ",
								(i%width)-k,(i/width)+k,ys[k],y[k]));
						}
						else if ( ABS(y[k]-ys[k]) > 2) {
							NOTIFY(("\n\t\t->Possible mismatch at pixel (%ld,%ld): scal=%ld, vec=%ld ",
								(i%width)-k,(i/width)+k,ys[k],y[k]));
						}
					}
					
					#ifdef DO_WRAP_TEST
					//
					// Wrap Detection
					//
					// Since we are producing approximations to the desired color, our
					//	generated color will be off in one direction or another.
					// If any color wraps from light to dark, in any color component
					//	we will get a nasty looking artifact.  This wrap-detection
					//	code is designed to detect a wrap, and then choose a more
					//	desireable approximation.
					// Wrap detection is complicated by a couple of things:
					//	1) We have to avoid deadlock -- when we switch to a new approximation
					//		to avoid wrap in one component that may cause a wrap in another
					//		component.
					//	2) We are working in YUV space, not the RGB space.
					//	3) We do U and V first, then calculate the Y.  This leaves us
					//		unable to adjust the U and V components by the time the
					//		final YUV is available.  We handle this by preventing overflow
					//		in U and V separately, so we have a reasonable color to deal
					//		with by the time the brightness is being done.
					//
					// See also the UVMin and UVMax, UVMINM2, UVMAXM2, settings
					//	used in Downsample().
					//
					// Classic Wrap Detection refers to the wrap detection originally written for
					//	Opera.  It does a very good job in most cases.
					// Modern Wrap Detection is a newer version designed to be faster and
					//	less restrictive of the color space.
					// 
					if (IsClassic)
					{
						if (params->compCount == 3)  // check for overflow in Y values
						{
							flag1=1; flag2=0; Uflow=0; Oflow=0;
							uint32 stuck = 0;		// Make sure we never get stuck in the while loop;
							while (flag1) {
								
								stuck++;
								if (stuck > kStuck) 
								{
									break;
								}
						
								/* compute current u/v pixel index */
								uvRowNum  = m;
								uvRowPel0 = (uvRowNum*width)>>2; // Pixel 0 of this line
								uvCurrPel = uvRowPel0 + (((i%width)-k)>>2);
								
								/* compute current r, g, b */
								u  = *(UDecbase + (width>>2) + uvCurrPel);
								v  = *(VDecbase + (width>>2) + uvCurrPel);
								u += 64;
								v += 64;
								r  = y[k] + v;
								r  = (r & decodeMask);
								g  = y[k] - (v>>1) - (u>>2) + 96;
								g  = (g & decodeMask);
								bl = y[k] + u;
								bl = (bl & decodeMask);
								
								/* fetch the RGB source values */
								rgb = source->pixels + i + (k*(width-1));
								rs  = rgb->part.red;
								gs  = rgb->part.green;
								bs  = rgb->part.blue;
								
								if ( (bl-bs>WTEST) || (g-gs>WTEST) || (r-rs>WTEST) ) { /* U'flow case */
									if ( ABS(e[k]) >= FTEST) 
										flag1=0;				/* fall through if false positive */
									else if (Oflow) 
										{flag1=0; eq[k]=save;}
									else {
										if ((!Oflow) && (!Uflow))	save = eq[k];
										Uflow=1;
										eq[k] = quantizer->BumpQuantUp(eq[k], ditherCounter, layerNumber, &flag1, &flag2);
									}
								}
								else if ( (bs-bl>WTEST) || (gs-g>WTEST) || (rs-r>WTEST) ) { /* O'flow case */
									if ( ABS(e[k]) >= FTEST) 
										flag1=0;				/* fall through if false positive */
									else if (Uflow) 
										{flag1=0; eq[k]=save;}
									else {
										if ((!Oflow) && (!Uflow))	save = eq[k];
										Oflow=1;
										eq[k] = quantizer->BumpQuantDown(eq[k], ditherCounter, layerNumber, &flag1, &flag2);
									}
								}
								else if ((g==0) || (r==0) || (bl==0)) {
									if (Oflow) flag1=0;
									else {
										eq[k] = quantizer->BumpQuantUp(eq[k], ditherCounter, layerNumber, &flag1, &flag2);
									}
								}
								else if ((g>=YMAX) || (r>=YMAX)) {
									if (Uflow) flag1=0;
									else {
										eq[k] = quantizer->BumpQuantDown(eq[k], ditherCounter, layerNumber, &flag1, &flag2);
									}
								}
								else flag1 = 0;
								if (flag2) {	/* if any eq[k] changed, redo EQv, Yv, and y[k] */
									EQv  = eq[0] << 24;
									EQv |= eq[1] << 16;
									EQv |= eq[2] <<  8;
									EQv |= eq[3];
									*(streambase+(m*width)+n) = EQv;
									Yv  = (Pv + EQv) >> shiftFactor;
									Yv &= decodeMask;
									y[k]  = ( Yv >> ((3-k)*8) ) & 0x000000FF;
									*(py+i+(k*(width-1))) = (unsigned char) y[k];
								}
							}
						}
					}
					else
					//
					// M2 Wrap Detection of the inner loop
					//
					{
						long adjusting=1; Uflow=0; Oflow=0; uint32 pinMax = 0; uint32 pinMin = 0;
						long terminate;
						uint32 stuck = 0;		// Make sure we never get stuck in the while loop;
						while (adjusting) {
							
							// count our iterations, so we won't get stuck.
							stuck++;
							
							// assume we are finally done with our adjustments
							adjusting = 0;
							
							// If computing Y, use the classic check for rgb mismatch in Y values
							if (params->compCount == 3) {
							
								/* compute current u/v pixel index */
								uvRowNum  = m;
								uvRowPel0 = (uvRowNum*width)>>2;
								uvCurrPel = uvRowPel0 + (((i%width)-k)>>2);
							
								/* compute current r, g, b */
								u  = *(UDecbase + (width>>2) + uvCurrPel);
								u += 64;
								v  = *(VDecbase + (width>>2) + uvCurrPel);
								v += 64;
								r  = y[k] + v;
								r  = (r & decodeMask);
								g  = y[k] - (v>>1) - (u>>2) + 96;
								g  = (g & decodeMask);
								bl = y[k] + u;
								bl = (bl & decodeMask);

								/* fetch the RGB source values */
								rgb = source->pixels + i + (k*(width-1));
								rs  = rgb->part.red;
								gs  = rgb->part.green;
								bs  = rgb->part.blue;
							
								if ( (bl-bs>WTEST) || (g-gs>WTEST) || (r-rs>WTEST) ) { // U'flow case
									if ( ABS(e[k]) < FTEST) {
										eq[k] = quantizer->BumpQuantUp(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
									}
								}
								else if ( (bs-bl>WTEST) || (gs-g>WTEST) || (rs-r>WTEST) ) { // O'flow case
									if ( ABS(e[k]) < FTEST) { 
										eq[k] = quantizer->BumpQuantDown(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
									}
								}
								else if ((g<=YMINM2) || (r<=YMINM2) || (bl<=YMINM2)) {
										eq[k] = quantizer->BumpQuantUp(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
								}
								else if ((g>=YMAXM2) || (r>=YMAXM2)) {
										eq[k] = quantizer->BumpQuantDown(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
								}
							}
							
							#define WRAP_TEST_UV
							#ifdef WRAP_TEST_UV
							// check for overflow in Y, U and V values
							if (! adjusting)
							{
								unsigned char pixel = (unsigned char) ((e[k] + (ps[k]>>shiftFactor)) & decodeMask);
								unsigned char approx = (unsigned char) ((eq[k] + (ps[k]>>shiftFactor)) & decodeMask);
								
								// approximation overflowed?
								if (pixel - approx > WTEST) {
									eq[k] = quantizer->BumpQuantDown(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
								}
								
								// approximation underflowed?
								else if (approx - pixel > WTEST) {
									eq[k] = quantizer->BumpQuantUp(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
								}
							}
							#endif

							if (adjusting) {	/* if any eq[k] changed, redo EQv, Yv, and y[k] */
								EQv  = eq[0] << 24;
								EQv |= eq[1] << 16;
								EQv |= eq[2] <<  8;
								EQv |= eq[3];
								*(streambase+(m*width)+n) = EQv;
								Yv  = (Pv + EQv) >> shiftFactor;
								Yv &= decodeMask;
								y[k]  = ( Yv >> ((3-k)*8) ) & 0x000000FF;
								*(py+i+(k*(width-1))) = (unsigned char) y[k];
							}
							
							// Handle the multiple-problems case -- quit adjusting
							if (stuck > kStuck)	
								adjusting = 0;  
						}
					}
					#endif // DO_WRAP_TEST
				}
			}
			
			// four-line end
			for (n=0; n<3; n++) {		/** Compute last 3 partial IDPCM vectors **/
				i++;					// temporarily wrap i beyond the width of the line.  Will fix after this loop.
				Av = Yv;				/* Compute partial p vector */
				Bv = Cv;
				Cv = (Av>>8);
				T1 = Av + Cv;
				T2 = Av + Cv - Bv;
				Pv = (T1>>1) + T2;

				/* DO_DITHERING */						// These last three vectors are shared
				// ditherCounter = ~ditherCounter;		//  with the first three, so we need to 
				/* DO_DITHERING */						//	use the same ditherCounter as for the first.

				for (k=n+1; k<4; k++) {				/* Compute eq for each vector element */
				
					a[k] = (unsigned char) ( Av >> ((3-k)*8) ) & 0x000000FF;
					b[k] = (unsigned char) ( Bv >> ((3-k)*8) ) & 0x000000FF;
					c[k] = (unsigned char) ( Cv >> ((3-k)*8) ) & 0x000000FF;
					p[k] = (unsigned char) ( Pv >> ((3-k)*8) ) & 0x000000FF;
				
					ps[k] = ( (a[k]+c[k]) >>1 ) + a[k]+c[k]-b[k];

						/* compute the error value */
					e[k] = *( sourcebase+i+(k*(width-1)) ) - (ps[k]>>shiftFactor);

						/* quantize the error value */
					eq[k] = quantizer->Quantize(e[k], ditherExtras[n], layerNumber);					
				}

				EQv = *(streambase+(m*width)+n);	/* Retrieve, merge, & save partial eq vectors */
				if (n==0) {
					EQv |= eq[1] << 24;
					EQv |= eq[2] << 16;
					EQv |= eq[3] <<  8;
					*(streambase+(m*width)) = EQv;
					EQv >>= 8;
				}
				if (n==1) {
					EQv |= eq[2] << 24;
					EQv |= eq[3] << 16;
					*(streambase+(m*width)+1) = EQv;
					EQv >>= 16;
				}
				if (n==2) {
					EQv |= eq[3] << 24;
					*(streambase+(m*width)+2) = EQv;
					EQv >>= 24;
				}

				Yv  = (Pv + EQv) >> shiftFactor;	/* Compute decompressed-sample vector */
				Yv &= decodeMask;
				
				for (k=n+1; k<4; k++) {			/* Store & compare to scalar value */
					
					y[k]  = ( Yv >> ((3-k)*8) ) & 0x000000FF;
					*(py+i+(k*(width-1))) = (unsigned char) y[k]; // stuff quantized value back.
					ys[k] = (ps[k] + (long) eq[k]) >> shiftFactor;
		
					if (params->verbose) {
						if (ys[k] < 0) {
							NOTIFY(("\n\t\t->Possible underflow at pixel (%ld,%ld): scal=%ld, vec=%ld ",
								(i%width)-k,(i/width)+k,ys[k],y[k]));
						}
						else if (ys[k] > 127) {
							NOTIFY(("\n\t\t->Possible overflow at pixel (%ld,%ld): scal=%ld, vec=%ld ",
								(i%width)-k,(i/width)+k,ys[k],y[k]));
						}
						else if ( ABS(y[k]-ys[k]) > 2) {
							NOTIFY(("\n\t\t->Possible mismatch at pixel (%ld,%ld): scal=%ld, vec=%ld ",
								(i%width)-k,(i/width)+k,ys[k],y[k]));
						}
					}

				#ifdef DO_WRAP_TEST
				#ifndef IGNORE_OVERFLOW_AT_END_OF_LINE

					// Check for overflow and underflow problems

					if (! IsClassic)  // all M2 compressors
					{
						// M2 Wrap Detection
						{
							long adjusting=1; Uflow=0; Oflow=0; uint32 pinMax = 0; uint32 pinMin = 0;
							long terminate;
							uint32 stuck = 0;		// Make sure we never get stuck in the while loop;
							while (adjusting) {
								
								stuck++;
								
								// assume we are finally done with our adjustments
								adjusting = 0;
								
								// Use the classic check for rgb mismatch in Y values
								if (params->compCount == 3) {
								
									// compute current u/v pixel index
									uvRowNum  = m;
									uvRowPel0 = (uvRowNum*width)>>2;
									// This line is different, because i isn't quite right -- we let it wrap.
									uvCurrPel = uvRowPel0 + ((((i-n-1)%width)-k)>>2);
									
									/* compute current r, g, b */
									u  = *(UDecbase + (width>>2) + uvCurrPel);
									u += 64;
									v  = *(VDecbase + (width>>2) + uvCurrPel);
									v += 64;
									r  = y[k] + v;
									r  = (r & decodeMask);
									g  = y[k] - (v>>1) - (u>>2) + 96;
									g  = (g & decodeMask);
									bl = y[k] + u;
									bl = (bl & decodeMask);
	
									/* fetch the RGB source values */
									rgb = source->pixels + i + (k*(width-1));
									rs  = rgb->part.red;
									gs  = rgb->part.green;
									bs  = rgb->part.blue;
								
									if ( (bl-bs>WTEST) || (g-gs>WTEST) || (r-rs>WTEST) ) { // U'flow case
										if ( ABS(e[k]) < FTEST) {
											eq[k] = quantizer->BumpQuantUp(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
										}
									}
									else if ( (bs-bl>WTEST) || (gs-g>WTEST) || (rs-r>WTEST) ) { // O'flow case
										if ( ABS(e[k]) < FTEST) { 
											eq[k] = quantizer->BumpQuantDown(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
										}
									}
									else if ((g<=YMINM2) || (r<=YMINM2) || (bl<=YMINM2)) {
											eq[k] = quantizer->BumpQuantUp(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
									}
									else if ((g>=YMAXM2) || (r>=YMAXM2)) {
											eq[k] = quantizer->BumpQuantDown(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
									}
								}
								
								// check for overflow in Y, U and V values
								{
									unsigned char pixel = (unsigned char) ((e[k] + (ps[k]>>shiftFactor)) & decodeMask);
									unsigned char approx = (unsigned char) ((eq[k] + (ps[k]>>shiftFactor)) & decodeMask);
									
									// approximation overflowed?
									if (pixel - approx > WTEST) {
										eq[k] = quantizer->BumpQuantDown(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
									}
									
									// approximation underflowed?
									else if (approx - pixel > WTEST) {
										eq[k] = quantizer->BumpQuantUp(eq[k], ditherCounter, layerNumber, &terminate, &adjusting);
									}
								
									// Handle the multiple-problems case -- we exit with a corrected RGB
									if (stuck > kStuck) break;  
								}
	
								if (adjusting) {	/* if any eq[k] changed, redo EQv, Yv, and y[k] */
									EQv = *(streambase+(m*width)+n);	/* Retrieve, merge, & save partial eq vectors */
									if (n==0) {
										EQv &= 0xFF;
										EQv |= eq[1] << 24;
										EQv |= eq[2] << 16;
										EQv |= eq[3] <<  8;
										*(streambase+(m*width)) = EQv;
										EQv >>= 8;
									}
									if (n==1) {
										EQv &= 0xFFFF;
										EQv |= eq[2] << 24;
										EQv |= eq[3] << 16;
										*(streambase+(m*width)+1) = EQv;
										EQv >>= 16;
									}
									if (n==2) {
										EQv &= 0xFFFFFF;
										EQv |= eq[3] << 24;
										*(streambase+(m*width)+2) = EQv;
										EQv >>= 24;
									}
									Yv  = (Pv + EQv) >> shiftFactor;
									Yv &= decodeMask;
									y[k]  = ( Yv >> ((3-k)*8) ) & 0x000000FF;
									*(py+i+(k*(width-1))) = (unsigned char) y[k]; // stuff adjusted quantized value back
								}
							}
						}
					}
				#endif // IGNORE_OVERFLOW_AT_END_OF_LINE
				#endif // DO_WRAP_TEST
				}
			}
			// "i" is our pixel index.  We have bumped it once for each diagonal of a group
			//	of four scan lines, plus three for the three extras at the end that we
			//	treat specially.  Now that four lines have been processed, we need
			//	to adjust i for the three scan lines processed along with line one
			//	in our group of four, and for the three extra pixels at the end of the line. 
			i += (3*width)-3; // Plus three scan lines, minus 3 pixels at the end of line. 
		}
	
		quantizer->QuantizeEncode(streambase, 
							height * width, 
							*outStream);
	} // Compress(...)

	/****************** END: GW encoding stuff ****************/

