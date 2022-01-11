/******************************************************************************
**
**  @(#) decode.c 96/11/25 1.6
**
**	MPEG audio decoding functions
**
******************************************************************************/
#ifndef __STDIO_H
#include <stdio.h>
#endif

#ifndef __STRING_H
#include <string.h>
#endif

#ifndef __MISC_MPEG_H
#include <misc/mpeg.h>
#endif

#ifndef __KERNEL_DEBUG_H
#include <kernel/debug.h>
#endif

#ifndef _MPEGAUDIOTYPES_H
#include "mpegaudiotypes.h"
#endif

#include "bitstream.h"
#include "mpegdebug.h"
#include "decode.h"
#include "decodeTables.h"

/****************************
 * Local function prototype *
 ****************************/

static Err pickTable(FrameInfo *fi)
{
	int16	bpc;
	int8	sf;
	Err		status = 0;

#ifdef DEBUG_AUDIO
  PRNT(("pickTable(): entered\n"));
#endif

  /*  bpc = bitrate[fi->header.layer - 1][fi->header.bitrate_index] / fi->nch;*/
	bpc = bitrate[fi->header->layer - 1][fi->header->bitrate_index] >> (fi->nch - 1);
/*
	sf =  s_freq[fi->header->sampling_frequency];
*/
    sf = fi->header->sampling_frequency;

	/* check for errors */
	if (bpc < 0) {
		INFO(("Invalid bitrate index %d\n", fi->header->bitrate_index));
		return (MPAUnsupportedBitrateErr);
	}

	if (sf == aFreq_reserved) {
		INFO(("Unsupported sampling frequency code %d\n",
			fi->header->sampling_frequency));
		return (MPAUnsupportedSampleFreqErr);
	}

#ifndef SUPPORT_ALL_SAMPLING_FREQ
	if (sf != aFreq_44100_Hz) {
		INFO(("Unsupported sampling frequency code %d\n",
			fi->header->sampling_frequency));
		return (MPAUnsupportedSampleFreqErr);
	}
#endif

	if (fi->nch==2) {
		if ((bpc==16) || (bpc==24) || (bpc==28) || (bpc==40)) {
			INFO(("Invalid combination of total bitrate %d and mode %d\n",
				bpc*fi->nch, fi->header->mode));
			status = MPAInvalidBitrateModeErr;
		}
	} else if ((bpc >= 224) && (bpc <= 384)) {
    	INFO(("invalid combination of total bitrate %d and mode %d\n",
			bpc*fi->nch, fi->header->mode));
		status = MPAInvalidBitrateModeErr;
	}


	/* select table based on sampling_frequency and bitrate per channel */
	if (bpc <= 48) {
		if (sf == 32) {
			fi->table = 3;
		} else {
			fi->table = 2;
		}
	} else if ((sf != 48) && (bpc >= 96)) {
		fi->table = 1;
	} else {
		fi->table = 0;
	}

	fi->sblimit = sblimit_table[fi->table];

#ifdef DEBUG_AUDIO
	PRNT(("pickTable(): exiting\n"));
#endif

	return (status);
} /* pickTable() */

Err parseHeader(BufferInfo *bi, FrameInfo *fi, tableToUsePtr tablePtr)
{
	int32	ch, sb;
	int32	temp = (MPEG_AUDIO_SYNCWORD << 20);	/* sizeof(uint32) -
										 		 * sizeof(syncword) (in bits) */
	Err		status;
	uint32	*alloc_pointer;
	uint32	*scfsi_pointer;
	uint32	*index_pointer;
	uint32	scalefactor_info[2*36];
	AudioHeader	*hdr = fi->header;

#ifdef DEBUG_AUDIO
	PRNT(("parseHeader(): entering\n"));
#endif

	/* parse header data from bitstream */
	temp |= getBits(20, bi, &status);
	if ( status != 0 )
		return status;
/*
	hdr->emphasis = temp & 3L;
	temp >>= 2L;
	hdr->original = temp & 1L;
	temp >>= 1L;
	hdr->copyright = temp & 1L;
	temp >>= 1L;
	hdr->mode_extension = temp & 3L;
	temp >>= 2L;
	hdr->mode = temp & 3L;
	temp >>= 2L;
	hdr->private_bit = temp & 1L;
	temp >>= 1L;
	hdr->padding_bit = temp & 1L;
	temp >>= 1L;
	hdr->sampling_frequency = temp & 3L;
	temp >>= 2L;
	hdr->bitrate_index = temp & 0xFL;
	temp >>= 4L;
	hdr->protection_bit = temp & 1L;
	temp >>= 1;
	hdr->layer = 4-(temp & 3L);
	temp >>= 2;
	hdr->id = temp & 1L;
*/

	memcpy( hdr, &temp, AUDIO_HEADER_CONTENT_SIZE );
	if (hdr->protection_bit == PROTECT_ON) {
		fi->crc_check = getBits(16, bi, &status);
		if (status != 0) 
			return status;
	}

	/** perform checks on data and extract needed info **/

	/* check for valid id bit */
	if (hdr->id != ID_ISO_11172_3_AUDIO) {
		INFO(("Only ISO/IEC 11172-3 audio supported\n"));
		return (MPAInvalidIDErr);
	}

	/* determine number of channels */
	fi->nch = (hdr->mode == aMode_single_channel) ? 1 : 2;

	/* parse on layer */
	switch(hdr->layer)
		{
		case audioLayer_I:
			INFO(("layer 1 not supported yet.\n"));
			return(MPAUnsupportedLayerErr);
			break;

		case audioLayer_II:
			/* sets table to use, sblimit. On error exit and parse next frame */
			if ((status = pickTable(fi)) != 0 ) {
				return(status);
			}

#ifdef DEBUG_AUDIO
	PRNT(("parseHeader(): point to correct tables\n"));
	PRNT(("msbit[%d][0][0] @ addr 0x%x\n", fi->table, &msbit[fi->table][0][0]));
#endif

			/* point to correct tables */
			tablePtr->msbit_table    	= &msbit[fi->table][0][0];
			tablePtr->quant_table		= &quant[fi->table][0][0];
			tablePtr->nlevels_table		= &nlevels[fi->table][0][0];
			tablePtr->codebits_table	= &codebits[fi->table][0][0];

#ifdef DEBUG_AUDIO
	PRNT(("parseHeader(): extablish end boundary for joint stereo \n"));
#endif

			/* establish end boundary for joint stereo */
			if (hdr->mode == aMode_joint_stereo) {
				fi->bound = bound_table[hdr->layer-1][hdr->mode_extension];
			} else {
				fi->bound = fi->sblimit;
			}

			/* parse out bit allocations */
			alloc_pointer = &fi->allocation[0];
			for (sb=0; sb<fi->bound; sb++) {
				temp = *(tablePtr->codebits_table + (sb<<4));
				for (ch=0; ch<fi->nch; ch++) {
					*alloc_pointer++ = getBits(temp, bi, &status);
				}
			}
			if (status != 0) 
				return status;

			for (sb=fi->bound; sb<fi->sblimit; sb++) {
	 			temp = getBits( *(tablePtr->codebits_table + (sb<<4)) , bi, &status);
				*alloc_pointer++ = temp;
				*alloc_pointer++ = temp;
			}
			if (status != 0) 
				return status;

			/* parse out scale factor selection information */
			alloc_pointer = &fi->allocation[0];
			scfsi_pointer = &scalefactor_info[0];
			for (sb=0; sb<fi->sblimit; sb++) {
				for (ch=0; ch<fi->nch; ch++) {
					if (*alloc_pointer++) {
						*scfsi_pointer = getBits(2, bi, &status);
					}
					scfsi_pointer++;
				}
			}
			if (status != 0) 
				return status;

			/* parser out scale factors */
			alloc_pointer = &fi->allocation[0];
			scfsi_pointer = &scalefactor_info[0];
			index_pointer = &fi->scalefactor_index[0];
			for (sb=0; sb<fi->sblimit; sb++) {
				for (ch=0; ch<fi->nch; ch++) {
					if (*alloc_pointer++) {
						switch (*scfsi_pointer)
						{
						case 0:
							temp = getBits(18, bi, &status);
							*(index_pointer+2) = temp & 0x3FL;
							temp >>= 6L;
							*(index_pointer+1) = temp & 0x3FL;
							temp >>= 6L;
							*(index_pointer+0) = temp;
							break;
						case 1:
							temp = getBits(12, bi, &status);
							*(index_pointer+2) = temp & 0x3FL;
							temp >>= 6L;
							*(index_pointer+1) = temp;
							*(index_pointer+0) = temp;
							break;
						case 2:
							temp = getBits(6, bi, &status);
							*(index_pointer+2) = temp;
							*(index_pointer+1) = temp;
							*(index_pointer+0) = temp;
							break;
						case 3:
							temp = getBits(12, bi, &status);
							*(index_pointer+2) = temp & 0x3FL;
							*(index_pointer+1) = temp & 0x3FL;
							temp >>= 6L;
							*(index_pointer+0) = temp;
							break;
						}
					}
					scfsi_pointer++;
					index_pointer+=3;
				}
			}
			if (status != 0)
				return status;
      		break;

		case audioLayer_III:
			INFO(("layer 3 not supported yet.\n"));
			return(MPAUnsupportedLayerErr);
			break;

		default:
			INFO(("Undefined or reserved layer.\n"));
			return(MPAUndefinedLayerErr);
			break;
		}

#ifdef DEBUG_AUDIO
	PRNT(("parseHeader(): exiting\n"));
#endif

	return(0);
} /* parseHeader() */


Err parseSubbands(BufferInfo *bi, FrameInfo *fi, tableToUsePtr tablePtr, uint32 *sample_code)
{
	Err		status;
	uint32 temp, temp2, numerator, junk, index, *codeptr, *codeptr2;
	uint32 *alloc_pointer;
	uint32 sb, ch;

#ifdef DEBUG_AUDIO
	PRNT(("parseSubbands(): entered\n"));
#endif

	alloc_pointer = &fi->allocation[0];
	for (sb=0; sb<fi->bound; sb++) {
		for (ch=0; ch<fi->nch; ch++) {
			codeptr = sample_code + (ch<<7) + (sb<<2);
			if (*alloc_pointer) {
				index = (sb<<4) + *alloc_pointer;
				temp = *(tablePtr->codebits_table + index);
				junk = *(tablePtr->nlevels_table + index);
				if ( (junk & 6) != 6) { /* grouping test */
					temp2 = getBits(temp, bi, &status);
					numerator = degrouper[junk];
					temp = (temp2 * numerator) >> 16;
					*codeptr++ = temp2 - (temp*junk);

					temp2 = (temp * numerator) >> 16;
					*codeptr++ = temp - (temp2*junk);

					temp = (temp2 * numerator) >> 16;
					*codeptr	 = temp2 - (temp*junk);
				} else {
					*codeptr++ = getBits(temp, bi, &status);
					*codeptr++ = getBits(temp, bi, &status);
					*codeptr   = getBits(temp, bi, &status);
				}
			}
			alloc_pointer++;
			if (status != 0)
				return status;
		}
	}


	for (sb=fi->bound; sb<fi->sblimit; sb++) {
		if (*alloc_pointer) {
			codeptr	= sample_code + (sb<<2);
			codeptr2 = sample_code + (sb<<2) + (1<<7);
			index = (sb<<4) + *alloc_pointer;
			temp = *(tablePtr->codebits_table + index);
			junk = *(tablePtr->nlevels_table + index);
			if ((junk & 6) != 6) {	/* grouping test */
				temp2 = getBits(temp, bi, &status);
				numerator = degrouper[junk];
				temp = (temp2 * numerator) >> 16;
				index = temp2 - (temp*junk);
				*codeptr++  = index;
				*codeptr2++ = index;

				temp2 = (temp * numerator) >> 16;
				index = temp - (temp2*junk);
				*codeptr++	= index;
				*codeptr2++ = index;

				temp = (temp2 * numerator) >> 16;
				index = temp2 - (temp*junk);
				*codeptr  = index;
				*codeptr2 = index;
      		} else {
				index = getBits(temp, bi, &status);
				*codeptr++  = index;
				*codeptr2++ = index;
				index = getBits(temp, bi, &status);
				*codeptr++  = index;
				*codeptr2++ = index;
				index = getBits(temp, bi, &status);
				*codeptr  = index;
				*codeptr2 = index;
      		}
    	}
    	alloc_pointer += 2;
		if (status != 0)
			return status;
	}

#ifdef DEBUG_AUDIO
	PRNT(("parsesubband(): exiting\n"));
#endif

	return 0;

} /* parsesubbands() */


void Requantize(FrameInfo *fi, tableToUsePtr tablePtr, uint32 *sample_code, AUDIO_FLOAT *input_sample, uint32 *alloc_pointer, uint32 *index_pointer)
{
	uint32	sb, x1, x2, sc, index, count;
	AUDIO_FLOAT temp1, temp2, numer, dt, ct, sf;

#ifdef DEBUG_AUDIO
	PRNT(("Requantize(): entering\n"));
#endif

	count = 0;
	for (sb=0; sb<fi->sblimit; sb++) {
		if (*alloc_pointer) {
			/* find the msb */
			index = count + *alloc_pointer;
			count += 16;
			x1 = *(tablePtr->msbit_table + index);

			/* get the sample */
			sc = *sample_code;
			sample_code += 4;

			/* if msb is set, positive */
			if (sc >> x1) {
				temp1 = 0.0;
			} else {
				temp1 = -1.0;
			} /* if-else */

			/* get other bits and form numer */
			numer = (AUDIO_FLOAT) (sc & ~(-1 << x1));
			temp1 += numer * scaler[x1];

			/* sample = factor * C * (sample + D) */
			x2 = *(tablePtr->quant_table + index);
			dt = d_table[x2];
			temp1 += dt;
			ct = c_table[x2];
			temp2 = ct;
			sf = scalefactor[*index_pointer];
			index_pointer+=3*fi->nch;
			temp2 *= sf;
			*input_sample++ = temp1*temp2;
			alloc_pointer+=fi->nch;
		} else {
			*input_sample++ = 0.0;
			count = count + 16;
			index_pointer+=3*fi->nch;
			alloc_pointer+=fi->nch;
			sample_code += 4;
		} /* if-else */
	} /* for */

#ifdef DEBUG_AUDIO
	PRNT(("Requantize(): Exiting\n"));
#endif

} /* Requantize() */


