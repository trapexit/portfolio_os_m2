#include <kernel/mem.h>
#include <kernel/debug.h>
#include "MPEGVideoParse.h"

/* input a buffer, "optimize" the buffer */

/* on the API level, we have the followings routines */
/* int32 InitVCDOptimize(VCDOPT_INFO **vcdopt, MPEGStreamInfo *streamInfo); */
/*         Initialize the state used to optimize stream */
/*         If optimize state structure doesn't exist, create it */
/* int32 DestroyVCDOptimize(VCDOPT_INFO *vcdopt); */
/*         Destroy the optimize state structure */
/* void DoVCDOptimize(VCDOPT_INFO *vcdopt, MPEGStreamInfo *streamInfo, char *bsBuffer, long length); */
/*         Pass in a buffer, and length, and optimize in place */

/* attributes */
#define PCODE_I 1
#define PCODE_P 2
#define PCODE_B 3
#define MB_QUANT    0x10
#define MB_FORWARD  0x08
#define MB_BACKWARD 0x04
#define MB_PATTERN  0x02
#define MB_INTRA    0x01
#define EF_DEFAULT 0
#define EF_FLOOR 1

/* states */
#define SLICE_START            0
#define SLICE_QUANT            1
#define SLICE_EXTRA_FLAG       2
#define SLICE_EXTRA_DATA       3
#define MACROBLOCK_START       4
#define MACROBLOCK_TYPE        5
#define MACROBLOCK_QUANT       6
#define MACROBLOCK_CODE        7
#define MACROBLOCK_RES         8
#define MACROBLOCK_PATTERN     9
#define BLOCK_START            10
#define DC_SIZE                11
#define DC_DIFF                12
#define RLP_OK_STAGE1          13
#define RLP_OK_STAGE2          14
#define RLP_OK_ESCAPE          15
#define RLP_CHECK_STAGE1       16
#define RLP_CHECK_STAGE2       17
#define RLP_CHECK_ESCAPE       18
#define RLP_CHECK_ESCAPE2      19
#define MACROBLOCK_END         20


/* start codes (8-bits) */
#define FIRST_SLICE_START_CODE 0x01
#define LAST_SLICE_START_CODE  0xAF

static void VCDChangeBits(VCDOPT_INFO *vcdopt, long n, long value);
static long VCDNextStartCode(VCDOPT_INFO *vcdopt);
static long VCDNextBits(VCDOPT_INFO *vcdopt, long n);
static long VCDGetBits(VCDOPT_INFO *vcdopt, long n);
static long VCDSkipBits(VCDOPT_INFO *vcdopt, long n);
static void VCDCheckQuant(VCDOPT_INFO *vcdopt, MPEGStreamInfo *streamInfo);

/* initialize the optimizing structure */
int32 InitVCDOptimize(VCDOPT_INFO **vcdopt, MPEGStreamInfo *streamInfo)
{
	int i;

	if (!(*vcdopt))	{			/* if structure doesn't exist */
		*vcdopt = (VCDOPT_INFO *) SuperAllocMem(sizeof(VCDOPT_INFO), MEMTYPE_NORMAL); /* alloc it */
		if (!(*vcdopt)) {
			PERR(("ERROR: Unable to malloc VCD Optmization state structure.\n"));
			return -1;			/* error value here */
		}
	}

	/* initialize structure */
	(*vcdopt)->bsLen = 0;
	(*vcdopt)->bsBits = 0;
	(*vcdopt)->bsData = 0;
	(*vcdopt)->state = SLICE_START;
	(*vcdopt)->forward_r_size = streamInfo->forwardFCode - 1;
	(*vcdopt)->backward_r_size = streamInfo->backwardFCode - 1;
	/* set min_intra_quant */
	(*vcdopt)->min_intra_quant = 255; /* set to max value */
	for (i=0; i<64; i++) {
		if (streamInfo->intraQuantMatrix[i]<(*vcdopt)->min_intra_quant) {
			(*vcdopt)->min_intra_quant = streamInfo->intraQuantMatrix[i];
		}
	}
	(*vcdopt)->min_non_intra_quant = 255; /* set to max value */
	for (i=0; i<64; i++) {
		if (streamInfo->nonIntraQuantMatrix[i]<(*vcdopt)->min_non_intra_quant) {
			(*vcdopt)->min_non_intra_quant = streamInfo->nonIntraQuantMatrix[i];
		}
	}
	if ((*vcdopt)->min_intra_quant<8) /* intra not o.k. */
		(*vcdopt)->ok_intra_matrix = 0;
	else 
		(*vcdopt)->ok_intra_matrix = 1;

	if ((*vcdopt)->min_non_intra_quant<8) /* nonintra not o.k. */
		(*vcdopt)->ok_non_intra_matrix = 0;
	else
		(*vcdopt)->ok_non_intra_matrix = 1;	/* by default */


	return 0;
}

/* destroy the optimizing structure */
int32 DestroyVCDOptimize(VCDOPT_INFO *vcdopt)
{
	if (vcdopt)
		SuperFreeMem(vcdopt, sizeof(VCDOPT_INFO));

	return 0;
}

/* operate on the buffer */
void DoVCDOptimize(VCDOPT_INFO *vcdopt, MPEGStreamInfo *streamInfo, char *bsBuffer, long length)
{
	int value, value2;

	if ((length==0) ||	/* if zero length buffer, or */
		(vcdopt->min_intra_quant==0) ||
		(vcdopt->min_non_intra_quant==0) || /* got lost, or  */
		((vcdopt->min_intra_quant>=8) && 
		 ((vcdopt->min_non_intra_quant>=8) ||
		  (streamInfo->pictHeader.pictureCodingType==PCODE_I)))) { /* no modification needed  */
		return;
	}

	/* init new buffer pointers and length value */
	vcdopt->bsPtr = bsBuffer;
	vcdopt->bsLen = length;
	vcdopt->bsDone = 0;

	/* parse through buffer, until done or a nonslice start code is found */
	while (1) {
		switch(vcdopt->state) {
		case SLICE_START:
			/* parse slice */
			vcdopt->start_code = VCDNextStartCode(vcdopt);
			if ((vcdopt->start_code<FIRST_SLICE_START_CODE) ||
				(vcdopt->start_code>LAST_SLICE_START_CODE)) { /* none slice start code found */
				vcdopt->bsDone = 1;
				break;
			}
		case SLICE_QUANT:
			/* get quantizer scale */
			value = VCDGetBits(vcdopt, 5);
			if (value<0) {
				vcdopt->state = SLICE_QUANT;
				break;
			} else {
				vcdopt->quantizer_scale = value;
				if (value==0) {	/*  got lost, look for next slice start code */
					vcdopt->state = SLICE_START;
					break;
				}
				/* now do check on min and quant */
				VCDCheckQuant(vcdopt, streamInfo);
			}
			/* parse out slice extra information */
		case SLICE_EXTRA_FLAG:
			value = VCDGetBits(vcdopt, 1);
			if (value < 0) {
				vcdopt->state = SLICE_EXTRA_FLAG;
				break;
			} else if (value==0) {
				vcdopt->state = MACROBLOCK_START;
				break;
			}
		case SLICE_EXTRA_DATA:
			value = VCDSkipBits(vcdopt, 8);
			if (value < 0) {
				vcdopt->state = SLICE_EXTRA_DATA;
				break;
			} else {
				vcdopt->state = SLICE_EXTRA_FLAG;
				break;
			}
		case MACROBLOCK_START:
			/* make sure we have next 11 bits before parsing */
			value = VCDNextBits(vcdopt, 11);
			if (value<0) {
				vcdopt->state = MACROBLOCK_START;
				break;
			}

			/* parse stuffing and escapes */
			if ((value==0xf) || (value==0x8)) {
				VCDSkipBits(vcdopt, 11);	/* since the above VCDNextBits passed, so should this one */
				vcdopt->state = MACROBLOCK_START;
				break;
			}				

			/* parse out macroblock address increment */
			if (value&0x400)      VCDSkipBits(vcdopt, 1);
			else if (value&0x200) VCDSkipBits(vcdopt, 3);
			else if (value&0x100) VCDSkipBits(vcdopt, 4);
			else if (value&0x80)  VCDSkipBits(vcdopt, 5);
			else if (value&0x40) {
				if (value&0x20)   VCDSkipBits(vcdopt, 7);
				else              VCDSkipBits(vcdopt, 8);
			} else if (value&0x20) {
				if (value&0x10)   VCDSkipBits(vcdopt, 8);
				else if (value&0xc) VCDSkipBits(vcdopt, 10);
				else              VCDSkipBits(vcdopt, 11);
			} else if (value&0x10) VCDSkipBits(vcdopt, 11);
			else {				/* got lost, find next start code */
				vcdopt->state = SLICE_START;
				break;
			}
		case MACROBLOCK_TYPE:
			/* get and decode type */
			value = VCDNextBits(vcdopt, 6);
			if (value<0) {
				vcdopt->state = MACROBLOCK_TYPE;
				break;
			}
			if (streamInfo->pictHeader.pictureCodingType==PCODE_I) {
				if (value&0x20) {
					vcdopt->macroblock_type = MB_INTRA;
					VCDSkipBits(vcdopt, 1);
				} else if (value&0x10) {
					vcdopt->macroblock_type = MB_QUANT | MB_INTRA;
					VCDSkipBits(vcdopt, 2);
				} else {		/* got lost, find next start code */
					vcdopt->state = SLICE_START;
					break;
				}
			} else if (streamInfo->pictHeader.pictureCodingType==PCODE_P) {
				if (value&0x38) {
					if (value&0x20) {
						vcdopt->macroblock_type = MB_FORWARD | MB_PATTERN;
						VCDSkipBits(vcdopt, 1);
					} else if (value&0x10) {
						vcdopt->macroblock_type = MB_PATTERN;
						VCDSkipBits(vcdopt, 2);
					} else {
						vcdopt->macroblock_type = MB_FORWARD;
						VCDSkipBits(vcdopt, 3);
					}
				} else if (value&0x4) {
					VCDSkipBits(vcdopt, 5);
					if (value&0x2) {
						vcdopt->macroblock_type = MB_INTRA;
					} else {
						vcdopt->macroblock_type = MB_QUANT | MB_FORWARD | MB_PATTERN;
					}
				} else if (value&0x2) {
					vcdopt->macroblock_type = MB_QUANT | MB_PATTERN;
					VCDSkipBits(vcdopt, 5);
				} else if (value&0x1) {
					vcdopt->macroblock_type = MB_QUANT | MB_INTRA;
					VCDSkipBits(vcdopt, 6);
				} else {		/* got lost, find next start code */
					vcdopt->state = SLICE_START;
					break;
				}
			} else { /* B picture */
				if (value&0x38) {
					if (value&0x20) {
						VCDSkipBits(vcdopt, 2);
						if (value&0x10) {
							vcdopt->macroblock_type = MB_FORWARD | MB_BACKWARD | MB_PATTERN;
						} else {
							vcdopt->macroblock_type = MB_FORWARD | MB_BACKWARD;
						}
					} else if (value&0x10) {
						VCDSkipBits(vcdopt, 3);
						if (value&0x8) {
							vcdopt->macroblock_type = MB_BACKWARD | MB_PATTERN;
						} else {
							vcdopt->macroblock_type = MB_BACKWARD;
						}
					} else {
						VCDSkipBits(vcdopt, 4);
						if (value&0x4) {
							vcdopt->macroblock_type = MB_FORWARD | MB_PATTERN;
						} else {
							vcdopt->macroblock_type = MB_FORWARD;
						}
					}
				} else if (value&0x4) {
					VCDSkipBits(vcdopt, 5);
					if (value&0x2) {
						vcdopt->macroblock_type = MB_INTRA;
					} else {
						vcdopt->macroblock_type = MB_QUANT | MB_FORWARD | MB_BACKWARD | MB_PATTERN;
					}
				} else if (value&0x2) {
					VCDSkipBits(vcdopt, 6);
					if (value&0x1) {
						vcdopt->macroblock_type = MB_QUANT | MB_FORWARD | MB_PATTERN;
					} else {
						vcdopt->macroblock_type = MB_QUANT | MB_BACKWARD | MB_PATTERN;
					}
				} else if (value&0x1) {
					VCDSkipBits(vcdopt, 6);
					vcdopt->macroblock_type = MB_QUANT | MB_INTRA;
				} else {		/* got lost, find next start code */
					vcdopt->state = SLICE_START;
					break;
				}
			}
		case MACROBLOCK_QUANT:
			/* get macroblock quantizer scale */
			if (vcdopt->macroblock_type&MB_QUANT) {
				value = VCDGetBits(vcdopt, 5);
				if (value<0) {
					vcdopt->state = MACROBLOCK_QUANT;
					break;
				} else {
					vcdopt->quantizer_scale = value;
					if (value==0) {	/* lost, find next slice start code */
						vcdopt->state = SLICE_START;
						break;
					}
					/* now do check on min and quant */
					VCDCheckQuant(vcdopt, streamInfo);
				}
			}
			vcdopt->fvect_count = (vcdopt->macroblock_type&MB_FORWARD) ? 2 : 0;
			vcdopt->bvect_count = (vcdopt->macroblock_type&MB_BACKWARD) ? 2 : 0;
		case MACROBLOCK_CODE:
			if ((vcdopt->fvect_count==0) &&
				(vcdopt->bvect_count==0)) {	/* no more vectors */
				vcdopt->state = MACROBLOCK_PATTERN;
				break;
			}
			/* get code */
			value = VCDNextBits(vcdopt, 11); /* may need up to 11 bits */
			if (value<0) {
				vcdopt->state = MACROBLOCK_CODE;
				break;
			}
				
			/* parse out the motion code */
			if (value&0x400) {/* zero case */
				VCDSkipBits(vcdopt, 1);
				if (vcdopt->fvect_count)
					vcdopt->fvect_count--;
				else 
					vcdopt->bvect_count--;
				vcdopt->state = MACROBLOCK_CODE;
				break;
			} else if (value&0x7C0) {
				if (value&0x200) {
					VCDSkipBits(vcdopt, 3);
				} else if (value&0x100) {
					VCDSkipBits(vcdopt, 4);
				} else if (value&0x80) {
					VCDSkipBits(vcdopt, 5);
				} else if (value&0x20) {
					VCDSkipBits(vcdopt, 7);
				} else {
					VCDSkipBits(vcdopt, 8);
				}
			} else {
				if (!(value&0x20)) {
					if ((value&0x18)==0x18) {
						VCDSkipBits(vcdopt, 11);
					} else {	/* got lost, find next start code */
						vcdopt->state = SLICE_START;
						break;
					}
				} else if (!(value&0x1C)) {
					VCDSkipBits(vcdopt, 11);
				} else if (!(value&0x10)) {
					VCDSkipBits(vcdopt, 10);
				} else {
					VCDSkipBits(vcdopt, 8);
				}
			}
		case MACROBLOCK_RES:
			/* skip residual */
			if (VCDSkipBits(vcdopt, 
							(vcdopt->fvect_count) ? 
							vcdopt->forward_r_size : 
							vcdopt->backward_r_size)<0) {
				vcdopt->state = MACROBLOCK_RES;
			} else {
				if (vcdopt->fvect_count)
					vcdopt->fvect_count--;
				else 
					vcdopt->bvect_count--;
				vcdopt->state = MACROBLOCK_CODE;
			}
			break;
		case MACROBLOCK_PATTERN:
			if (vcdopt->macroblock_type&MB_PATTERN) {
				/* get pattern code */
				value = VCDNextBits(vcdopt, 9);/* may need up to 9 bits */
				if (value<0) {
					vcdopt->state = MACROBLOCK_PATTERN;
					break;
				}

				/* derive luma and chroma counts */
				/* assume shorter codes occur more often */
				if (value&0x100) {
					vcdopt->chroma_count = 0;
					value2 = value&0xc0;
					if (value2==0xc0) {
						vcdopt->luma_count = 4;
						VCDSkipBits(vcdopt, 3);
					} else if (value2) {
						vcdopt->luma_count = 1;
						VCDSkipBits(vcdopt, 4);
					} else {
						vcdopt->luma_count = 2;
						VCDSkipBits(vcdopt, 5);
					}
				} else if (value&0x80) {
					if (value&0x40) {
						vcdopt->luma_count = 3;
						vcdopt->chroma_count = 0;
					} else {
						vcdopt->chroma_count = 1;
						if (value&0x10) {
							vcdopt->luma_count = 0;
						} else {
							vcdopt->luma_count = 4;
						}
					}
					VCDSkipBits(vcdopt, 5);
				} else if (value&0x40) {
					if (!(value&0x20)) {
						vcdopt->luma_count = 1;
						vcdopt->chroma_count = 1;
						VCDSkipBits(vcdopt, 7);
					} else {
						if (value&0x10) {
							vcdopt->luma_count = 2;
							vcdopt->chroma_count = 0;
						} else {
							vcdopt->chroma_count = 2;
							if (value&0x8) {
								vcdopt->luma_count = 0;
							} else {
								vcdopt->luma_count = 4;
							}
						}
						VCDSkipBits(vcdopt, 6);
					}
				} else if (value&0x20) {
					value2 = value&0x18;
					if (value2==0x18) {
						vcdopt->luma_count = 1;
						vcdopt->chroma_count = 2;
					} else {
						vcdopt->luma_count = 2;
						if (value2) {
							vcdopt->chroma_count = 1;
						} else {
							vcdopt->chroma_count = 2;
						}
					}
					VCDSkipBits(vcdopt, 8);
				} else if (value&0x18) {
					vcdopt->chroma_count = 1;
					if ((value&0x18)==0x18) {
						vcdopt->luma_count = 2;
					} else {
						vcdopt->luma_count = 3;
					}
					VCDSkipBits(vcdopt, 8);
				} else if (value&0x6) {
					vcdopt->chroma_count = 2;
					if (value&0x4) {
						vcdopt->luma_count = 3;
					} else {
						vcdopt->luma_count = 2;
					}
					VCDSkipBits(vcdopt, 9);
				} else {		/* got lost, find next start code */
					vcdopt->state = SLICE_START;
					break;
				}
			} else {
				if (vcdopt->macroblock_type&MB_INTRA) {
					vcdopt->luma_count = 4;
					vcdopt->chroma_count = 2;
				} else {
					vcdopt->luma_count = 0;
					vcdopt->chroma_count = 0;
				}
			}
		case BLOCK_START:
			if ((vcdopt->luma_count==0) &&
				(vcdopt->chroma_count==0)) {
				vcdopt->state = MACROBLOCK_END;
				break;
			}
		case DC_SIZE:
			if (!(vcdopt->macroblock_type&MB_INTRA)) {
				vcdopt->coef_first = 1;
				if (vcdopt->ok_non_intra_quant_scale) {
					vcdopt->state =  RLP_OK_STAGE1;
				} else {
					vcdopt->coefindex = 0; /* init coef index */
					vcdopt->state = RLP_CHECK_STAGE1;
				}
				vcdopt->thresh = vcdopt->non_intra_quant_thresh;
				vcdopt->maxindex = vcdopt->min_non_intra_index;
				break;
			} else {
				vcdopt->coef_first = 0;
				vcdopt->thresh = vcdopt->intra_quant_thresh;
				vcdopt->maxindex = vcdopt->min_intra_index;
			}
			/* get enough bits for the max size vlc */
			value = VCDNextBits(vcdopt, (vcdopt->luma_count) ? 7 : 8);
			if (value<0) {
				vcdopt->state = DC_SIZE;
				break;
			}
				
			/* decode the vlc */
			if (vcdopt->luma_count) {/* decode luma dc size */
				if ((value&0x70)==0x70) {
					if ((value&0xc)==0xc) {
						if (value&0x2) {
							if (value&0x1) { /* got lost, find next start code */
								vcdopt->state = SLICE_START;
								break;
							} else {
								vcdopt->dc_size = 8;
								VCDSkipBits(vcdopt, 7);
							}
						} else {
							vcdopt->dc_size = 7;
							VCDSkipBits(vcdopt, 6);
						}
					} else if (value&0x8) {
						vcdopt->dc_size = 6;
						VCDSkipBits(vcdopt, 5);
					} else {
						vcdopt->dc_size = 5;
						VCDSkipBits(vcdopt, 4);
					}
				} else {
					switch(value&0x60) {
					case 0x60:
						vcdopt->dc_size = 4;
						VCDSkipBits(vcdopt, 3);
						break;
					case 0x40:
						if (value&0x10) {
							vcdopt->dc_size = 3;
						} else {
							vcdopt->dc_size = 0;
						}						
						VCDSkipBits(vcdopt, 3);
						break;
					case 0x20:
						vcdopt->dc_size = 2;
						VCDSkipBits(vcdopt, 2);
						break;
					case 0x00:
						vcdopt->dc_size = 1;
						VCDSkipBits(vcdopt, 2);
						break;
					}
				}
			} else {			/* decoding chroma */
				if ((value&0xf0)==0xf0) {
					if ((value&0xc)==0xc) {
						if (value&0x2) {
							if (value&0x1) { /* got lost, find next start code */
								vcdopt->state = SLICE_START;
								break;
							} else {
								vcdopt->dc_size = 8;
								VCDSkipBits(vcdopt, 8);
							}
						} else {
							vcdopt->dc_size = 7;
							VCDSkipBits(vcdopt, 7);
						}
					} else if (value&0x8) {
						vcdopt->dc_size = 6;
						VCDSkipBits(vcdopt, 6);
					} else {
						vcdopt->dc_size = 5;
						VCDSkipBits(vcdopt, 5);
					}
				} else {
					switch(value&0xc0) {
					case 0xc0:
						if (value&0x20) {
							vcdopt->dc_size = 4;
							VCDSkipBits(vcdopt, 4);
						} else {
							vcdopt->dc_size = 3;
							VCDSkipBits(vcdopt, 3);
						}
						break;
					case 0x80:
						vcdopt->dc_size = 2;
						VCDSkipBits(vcdopt, 2);
						break;
					case 0x40:
						vcdopt->dc_size = 1;
						VCDSkipBits(vcdopt, 2);
						break;
					case 0x00:
						vcdopt->dc_size = 0;
						VCDSkipBits(vcdopt, 2);
						break;
					}
				}
			}
		case DC_DIFF:
			if (VCDSkipBits(vcdopt, vcdopt->dc_size) <0) {
				vcdopt->state = DC_DIFF;
				break;
			}
			if (!vcdopt->ok_intra_quant_scale) {
				vcdopt->coefindex = 1; /* init coefindex */
				vcdopt->state = RLP_CHECK_STAGE1;
				break;
			}
			vcdopt->state = RLP_OK_STAGE1;
		case RLP_OK_STAGE1:
			/* there will be a minimum of 2+1+2+5=10 bits between the last vlc */
			/* of an ok block and the first vlc of a block that needs to be checked */
			/* lets prefetch 6 bits and parse */
			value = VCDNextBits(vcdopt, 6);
			if (value<0) {
				break;
			}

			if (value&0x20) {
				if (vcdopt->coef_first) { /* 1x */
					VCDSkipBits(vcdopt, 2);
				} else if (value&0x10) {	/* 11x */
					VCDSkipBits(vcdopt, 3);
				} else {		/* 10 - end of block */
					VCDSkipBits(vcdopt, 2);
					if (vcdopt->luma_count) {
						vcdopt->luma_count--;
					} else {
						vcdopt->chroma_count--;
					}
					vcdopt->state = BLOCK_START;
				}
			} else if (value&0x10) {
				if (value&0x8) {
					VCDSkipBits(vcdopt, 4);
				} else {
					VCDSkipBits(vcdopt, 5);
				}
			} else if (value&0x8) {
				if (value&0x6) {
					VCDSkipBits(vcdopt, 6);
				} else {
					if (VCDSkipBits(vcdopt, 9)<0) {
						break;
					}
				}
			} else if (value&0x4) {
				VCDSkipBits(vcdopt, 7);
			} else if (value&0x2) {
				VCDSkipBits(vcdopt, 8);
			} else if (value&0x1) {
				/* escape */
				if (VCDSkipBits(vcdopt, 12)<0) {
					break;
				} else {
					vcdopt->state = RLP_OK_ESCAPE;
				}
			} else {			/* need stage2 */
				VCDSkipBits(vcdopt, 6);
				vcdopt->state = RLP_OK_STAGE2;
			}
			vcdopt->coef_first = 0;
			break;
		case RLP_OK_STAGE2:
			/* there will be a minimum of 5+1+2+5=13 bits between the last vlc */
			/* of an ok block and the first vlc of a block that needs to be checked */
			/* lets prefetch the max bits needed and parse */
			value = VCDNextBits(vcdopt, 11);
			if (value<0) break;
			
			if (value&0x400) {
				VCDSkipBits(vcdopt, 5);
			} else if (value&0x200) {
				VCDSkipBits(vcdopt, 7);
			} else if (value&0x100) {
				VCDSkipBits(vcdopt, 8);
			} else if (value&0x80) {
				VCDSkipBits(vcdopt, 9);
			} else if (value&0x40) {
				VCDSkipBits(vcdopt, 10);
			} else if (value&0x20) {
				VCDSkipBits(vcdopt, 11);
			} else {			/* got lost, find next start code */
				vcdopt->state = SLICE_START;
				break;
			}
			vcdopt->state = RLP_OK_STAGE1;
			break;
		case RLP_OK_ESCAPE:
			/* there will be a minimum of 8+1+2+5=16 bits between the last vlc */
			/* of an ok block and the first vlc of a block that needs to be checked */
			/* lets prefetch 8 bits and parse */
			value = VCDNextBits(vcdopt, 8);
			if (value<0) {
				break;
			}

			if (value&0x7f) {	/* short code */
				VCDSkipBits(vcdopt, 8);
			} else {			/* long code */
				if (VCDSkipBits(vcdopt, 16)<0) {
					break;
				}
			}
			vcdopt->state = RLP_OK_STAGE1;
			break;
		case RLP_CHECK_STAGE1:
			/* we need to be really careful here so that we don't get rid of
			   something that we may need to change later, that means only getting 
			   the number of bits that we need for one rlp code */
			value = VCDNextBits(vcdopt, 2); /* minimum code */
			if (value<0) break;

			if (value&0x2) {
				if (vcdopt->coef_first) { /* 0,1 */
					VCDSkipBits(vcdopt, 2);
					if ((value&0x1) && /* negative */
						(vcdopt->thresh[vcdopt->coefindex]>0)) { /* not above threshold */
						VCDChangeBits(vcdopt, 1, 0);
					}
				} else if (value&0x1) { /* 0,1 */
					value2 = VCDGetBits(vcdopt, 3);
					if (value2<0) break;
					if ((value2&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>0)) { /* not above threshold */
						VCDChangeBits(vcdopt, 1, 0);
					}
				} else {		/* end of block */
					VCDSkipBits(vcdopt, 2);
					if (vcdopt->luma_count) {
						vcdopt->luma_count--;
					} else {
						vcdopt->chroma_count--;
					}
					vcdopt->state = BLOCK_START;
					break;
				}
			} else if (value&0x1) {
				value = VCDNextBits(vcdopt, 4);
				if (value<0) break;
				if (value&0x2) { /* 1,1 */
					VCDSkipBits(vcdopt, 4);
					vcdopt->coefindex++;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=1)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
				} else {
					value2 = VCDGetBits(vcdopt, 5);
					if (value2<0) break;
					if (value2&0x2) { /* 2,1 */
						vcdopt->coefindex +=2;
						if ((value2&0x1) &&
							(vcdopt->thresh[vcdopt->coefindex]>=1)) {
							VCDChangeBits(vcdopt, 1, 0);
						}
					} else { /* 0,2 */
						if ((value2&0x1) &&
							(vcdopt->thresh[vcdopt->coefindex]>=2)) {
							VCDChangeBits(vcdopt, 1, 0);
						}
					}
				}
			} else {
				value = VCDNextBits(vcdopt, 6);
				if (value<0) break;
				if (value&0x8) {
					if (value&0x4) {
						VCDSkipBits(vcdopt, 6);
						if (value&0x2) { /* 3,1 */
							vcdopt->coefindex +=3;
						} else { /* 4,1 */
							vcdopt->coefindex +=4;
						}
						if ((value&0x1) &&
							(vcdopt->thresh[vcdopt->coefindex]>=1)) {
							VCDChangeBits(vcdopt, 1, 0);
						}
					} else if (value&0x2) {	/* 0,3 */
						VCDSkipBits(vcdopt, 6);
						if ((value&0x1) &&
							(vcdopt->thresh[vcdopt->coefindex]>=3)) {
							VCDChangeBits(vcdopt, 1, 0);
						}
					} else {	/* 00100xxx */
						value = VCDGetBits(vcdopt, 9);
						if (value<0) break;
						switch((value>>1)&0x7) {
						case 0:	/* 13,1 */
							vcdopt->coefindex++;
						case 2:	/* 12,1 */
							vcdopt->coefindex++;
						case 3:	/* 11,1 */
							vcdopt->coefindex++;
						case 7:	/* 10,1 */
							vcdopt->coefindex+=10;
							if ((value&0x1) &&
								(vcdopt->thresh[vcdopt->coefindex]>=1)) {
								VCDChangeBits(vcdopt, 1, 0);
							}
							break;
						case 1:	/* 0,6 */
							if ((value&0x1) &&
								(vcdopt->thresh[vcdopt->coefindex]>=6)) {
								VCDChangeBits(vcdopt, 1, 0);
							}
							break;
						case 4:	/* 3,2 */
							vcdopt->coefindex+=3;
							if ((value&0x1) &&
								(vcdopt->thresh[vcdopt->coefindex]>=2)) {
								VCDChangeBits(vcdopt, 1, 0);
							}
							break;
						case 5:	/* 1,3 */
							vcdopt->coefindex++;
							if ((value&0x1) &&
								(vcdopt->thresh[vcdopt->coefindex]>=3)) {
								VCDChangeBits(vcdopt, 1, 0);
							}
							break;
						case 6:	/* 0,5 */
							if ((value&0x1) &&
								(vcdopt->thresh[vcdopt->coefindex]>=5)) {
								VCDChangeBits(vcdopt, 1, 0);
							}
							break;
						}
					}
				} else if (value&0x4) {
					value = VCDGetBits(vcdopt, 7);
					if (value<0) break;
					switch((value>>1)&0x3) {
					case 0:		/* 7,1 */
						vcdopt->coefindex++;
					case 1:		/* 6,1 */
						vcdopt->coefindex++;
					case 3:		/* 5,1 */
						vcdopt->coefindex+=5;
						if ((value&0x1) &&
							(vcdopt->thresh[vcdopt->coefindex]>=1)) {
							VCDChangeBits(vcdopt, 1, 0);
						}
						break;
					case 2:		/* 1,2 */
						vcdopt->coefindex++;
						if ((value&0x1) &&
							(vcdopt->thresh[vcdopt->coefindex]>=2)) {
							VCDChangeBits(vcdopt, 1, 0);
						}
						break;
					}
				} else if (value&0x2) {
					value = VCDGetBits(vcdopt, 8);
					if (value<0) break;
					switch((value>>1)&0x3) {
					case 1:		/* 9,1 */
						vcdopt->coefindex++;
					case 3:		/* 8,1 */
						vcdopt->coefindex+=8;
						if ((value&0x1) &&
							(vcdopt->thresh[vcdopt->coefindex]>=1)) {
							VCDChangeBits(vcdopt, 1, 0);
						}
						break;
					case 2:		/* 0,4 */
						if ((value&0x1) &&
							(vcdopt->thresh[vcdopt->coefindex]>=4)) {
							VCDChangeBits(vcdopt, 1, 0);
						}
						break;
					case 0:		/* 2,2 */
						vcdopt->coefindex+=2;
						if ((value&0x1) &&
							(vcdopt->thresh[vcdopt->coefindex]>=2)) {
							VCDChangeBits(vcdopt, 1, 0);
						}
						break;
					}
				} else if (value&0x1) {	/* escape code, get run */
					value=VCDGetBits(vcdopt, 12);
					if (value<0) break;
					vcdopt->coefindex+=(value&0x3f);
					if (vcdopt->thresh[vcdopt->coefindex]>0) {
						vcdopt->state = RLP_CHECK_ESCAPE;
					} else {	/* no checking needed */
						vcdopt->state = RLP_OK_ESCAPE;
					}
					vcdopt->coef_first = 0;
					break;
				} else {		/* need stage2 */
					VCDSkipBits(vcdopt, 6);
					vcdopt->coef_first = 0;
					vcdopt->state = RLP_CHECK_STAGE2;
					break;
				}
			}
			vcdopt->coefindex++;
			if (vcdopt->coefindex>vcdopt->maxindex) {
				vcdopt->state = RLP_OK_STAGE1;
			}
			vcdopt->coef_first = 0;
			break;
		case RLP_CHECK_STAGE2:
			value = VCDNextBits(vcdopt, 5);
			if (value<0) break;
			if (value&0x10) {
				value = VCDGetBits(vcdopt, 5);
				switch((value>>1)&0x7) {
				case 0:			/* 16,1 */
					vcdopt->coefindex++;
				case 5:			/* 15,1 */
					vcdopt->coefindex++;
				case 6:			/* 14,1 */
					vcdopt->coefindex+=14;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=1)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 1:			/* 5,2 */
					vcdopt->coefindex++;
				case 7:			/* 4,2 */
					vcdopt->coefindex+=4;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=2)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 3:			/* 2,3 */
					vcdopt->coefindex+=2;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=3)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 4:			/* 1,4 */
					vcdopt->coefindex++;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=4)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 2:			/* 0,7 */
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=7)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				}
			} else if (value&0x8) {
				value = VCDGetBits(vcdopt, 7);
				if (value<0) break;
				switch((value>>1)&0xf) {
				case 6:			/* 21,1 */
					vcdopt->coefindex++;
				case 7:			/* 20,1 */
					vcdopt->coefindex++;
				case 9:			/* 19,1 */
					vcdopt->coefindex++;
				case 10:		/* 18,1 */
					vcdopt->coefindex++;
				case 15:		/* 17,1 */
					vcdopt->coefindex+=17;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=1)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 1:			/* 8,2 */
					vcdopt->coefindex++;
				case 5:			/* 7,2 */
					vcdopt->coefindex++;
				case 14:		/* 6,2 */
					vcdopt->coefindex+=6;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=2)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 2:			/* 4,3 */
					vcdopt->coefindex++;
				case 12:		/* 3,3 */
					vcdopt->coefindex+=3;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=3)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 4:			/* 2,4 */
					vcdopt->coefindex+=2;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=4)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 11:		/* 1,5 */
					vcdopt->coefindex++;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=5)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				}
			} else if (value&0x4) {
				value = VCDGetBits(vcdopt, 8);
				if (value<0) break;
				switch((value>>1)&0xf) {
				case 11:		/* 26,1 */
					vcdopt->coefindex++;
				case 12:		/* 25,1 */
					vcdopt->coefindex++;
				case 13:		/* 24,1 */
					vcdopt->coefindex++;
				case 14:		/* 23,1 */
					vcdopt->coefindex++;
				case 15:		/* 22,1 */
					vcdopt->coefindex+=22;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=1)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 0:			/* 10,2 */
					vcdopt->coefindex++;
				case 1:			/* 9,2 */
					vcdopt->coefindex+=9;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=2)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 2:			/* 5,3 */
					vcdopt->coefindex+=5;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=3)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 3:			/* 3,4 */
					vcdopt->coefindex+=3;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=4)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 4:			/* 2,5 */
					vcdopt->coefindex+=2;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=5)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 5:			/* 1,7 */
					vcdopt->coefindex++;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=7)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 6:			/* 1,6 */
					vcdopt->coefindex++;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=6)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				}
			} else if (value&0x2) {
				value = VCDSkipBits(vcdopt, 9);
				if (value<0) break;
			} else if (value&0x1) {
				value = VCDGetBits(vcdopt, 10);
				if (value<0) break;
				if ((value&0x10) &&
					(value&0xe)) { /* run of one, no check needed */
					vcdopt->coefindex++;
				}
			} else {
				value = VCDNextBits(vcdopt, 11);
				if (value<0) break;
				if (!(value&0x20)) { /* got lost, find next start code */
					vcdopt->state = SLICE_START;
					break;
				}
				value = VCDGetBits(vcdopt, 11);
				if (value<0) break;
				switch((value>>1)&0xf) {
				case 11:		/* 31,1 */
					vcdopt->coefindex++;
				case 12:			/* 30,1 */
					vcdopt->coefindex++;
				case 13:			/* 29,1 */
					vcdopt->coefindex++;
				case 14:		/* 28,1 */
					vcdopt->coefindex++;
				case 15:		/* 27,1 */
					vcdopt->coefindex+=27;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=1)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 5:			/* 16,2 */
					vcdopt->coefindex++;
				case 6:			/* 15,2 */
					vcdopt->coefindex++;
				case 7:			/* 14,2 */
					vcdopt->coefindex++;
				case 8:			/* 13,2 */
					vcdopt->coefindex++;
				case 9:			/* 12,2 */
					vcdopt->coefindex++;
				case 10:		/* 11,2 */
					vcdopt->coefindex+=11;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=2)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				case 4:		/* 6,3 */
					vcdopt->coefindex+=6;
					if ((value&0x1) &&
						(vcdopt->thresh[vcdopt->coefindex]>=3)) {
						VCDChangeBits(vcdopt, 1, 0);
					}
					break;
				default:		/* runs of 1, no checking needed */
					vcdopt->coefindex++;
					break;
				}
			}				
			vcdopt->coefindex++;
			if (vcdopt->coefindex>vcdopt->maxindex) {
				vcdopt->state = RLP_OK_STAGE1;
			} else {
				vcdopt->state = RLP_CHECK_STAGE1;
			}
			break;
		case RLP_CHECK_ESCAPE:
			/* now we get to the really tricky part, need to get as much data without */
			/* switching buffers, if we have to switch make a decision as to whether to */
			/* negate or not */

			/* first get the sign bit, see if we need to do anything */
			value = VCDNextBits(vcdopt, 1);
			if (value<0) break;

			/* if positive, no change needed */
			if (!(value)) {
				value = VCDNextBits(vcdopt, 8);
				if (value<0) break;
				if (value&0x7f) {	/* short code */
					VCDSkipBits(vcdopt, 8);
				} else {			/* long code */
					if (VCDSkipBits(vcdopt, 16)<0) break;
				}
			} else {			/* negative, may need a change */
				value = VCDNextBits(vcdopt, 8);
				if (value>=0) {	/* enough bits left in buffer */
					if (value&0x7f) { /* short code */
						VCDSkipBits(vcdopt, 8);
						if ((value&0xf8)==0xf8) { /* might need to change */
							if (vcdopt->thresh[vcdopt->coefindex]>=(8-(value&0x7))) { /* need to change */
								VCDChangeBits(vcdopt, 8, (~value+1)&0xff);
							}
						}
					} else {	/* long code, definitely don't need to change */
						if (VCDSkipBits(vcdopt, 16)<0) break;
					}
				} else {  /* not enough bits left in buffer, BIG PAIN!!! */
					/* return code is -(validbits+1) */
					value = -(value+1);/* numbits valid */
					value2 = VCDGetBits(vcdopt, value);
					vcdopt->escape_numbits = value; /* number of bits from now empty buffer */
					vcdopt->escape_fix = EF_DEFAULT; /* no decision made yet */
					vcdopt->escape_allzeros = 1; /* flag for possible extended level case */
					vcdopt->escape_allones = 1;	/* flag for possible modification case */
					switch(value) {
					case 1:		/* only sign bit */
						break;
					case 2:
						if (value2&0x1) {
							vcdopt->escape_allzeros = 0;
						} else {
							vcdopt->escape_allones = 0;
						}
						break;
					case 3:
						if ((value2&0x3)!=0x3) {
							vcdopt->escape_allones = 0;
						}
						if (value2&0x3) {
							vcdopt->escape_allzeros = 0;
						}
						break;
					case 4:
						if ((value2&0x7)!=0x7) {
							vcdopt->escape_allones = 0;
						}
						if (value2&0x7) {
							vcdopt->escape_allzeros = 0;
						}
						break;
					case 5:
						if ((value2&0xf)!=0xf) {
							vcdopt->escape_allones = 0;
						}
						if (value2&0xf) {
							vcdopt->escape_allzeros = 0;
						}							
						break;
					case 6:
						if (value2&0x1f) {
							vcdopt->escape_allzeros = 0;
						}							
						if ((value2&0x1e)!=0x1e) { /* some zeros in msb, level <-8, won't need to modify  */
							vcdopt->escape_allones = 0;
						} else if (value2&0x1) { /* -4<=level<=-1, may need to change */
							if (vcdopt->thresh[vcdopt->coefindex]==7) {	/* need to change */
								VCDChangeBits(vcdopt, 6, 0x1); /* change to +4 */
								vcdopt->escape_fix = EF_FLOOR;
							} else if (vcdopt->thresh[vcdopt->coefindex]==3) {	/* change to -4 */
								vcdopt->escape_fix = EF_FLOOR;
							} /* else may need to floor in next state */
						} else { /* -8<=level<=-5, may need to change, but first six bits won't change */
							if (vcdopt->thresh[vcdopt->coefindex]==7) {	/* change to negative 8 */
								vcdopt->escape_fix = EF_FLOOR;
							} /* else no change */
						}
						break;
					case 7:
						if (value2&0x3f) {
							vcdopt->escape_allzeros = 0;
						}
						if ((value2&0x3c)!=0x3c) { /* some zeros in msb, level < -8, won't need modify */
							vcdopt->escape_allones = 0;
						} else {
							switch(value2&0x3) {
							case 0: /*  -8, -7, may need to change, but first six bits won't change */
								if (vcdopt->thresh[vcdopt->coefindex]==7) {	/* floor to -8 */
									vcdopt->escape_fix = EF_FLOOR;
								} /* else no need to change */
							case 1: /* -6, -5, may need to change */
								if (vcdopt->thresh[vcdopt->coefindex]==7) {	/* need to change */
									VCDChangeBits(vcdopt, 7,0x3);	/* change to +6 */
									vcdopt->escape_fix = EF_FLOOR;
								} /* else no need to change */
								break;
							case 2: /* -4, -3, may need to change */
								if (vcdopt->thresh[vcdopt->coefindex]==7) {	/* need to change */
									VCDChangeBits(vcdopt, 7,0x2);	/* change to +4 */
									vcdopt->escape_fix = EF_FLOOR;
								} else if (vcdopt->thresh[vcdopt->coefindex]==3) { /* floor to -4 */
									vcdopt->escape_fix = EF_FLOOR;
								} /* else no need to change */
								break;
							case 3: /* -2, -1, may need to change */
								if (vcdopt->thresh[vcdopt->coefindex]>=2) {	/* need to change */
									VCDChangeBits(vcdopt, 7,0x1);	/* change to +2 */
									vcdopt->escape_fix = EF_FLOOR;
								} else { /* floor to -2 */
									vcdopt->escape_fix = EF_FLOOR;
								}
								break;
							}
						}
					}
					/* haven't finished a coefficient yet */
					vcdopt->state = RLP_CHECK_ESCAPE2;
					break;
				}
			}		
			vcdopt->coefindex++;
			if (vcdopt->coefindex>vcdopt->maxindex) {
				vcdopt->state = RLP_OK_STAGE1;
			} else {
				vcdopt->state = RLP_CHECK_STAGE1;
			}
			break;
		case RLP_CHECK_ESCAPE2:
			value = VCDNextBits(vcdopt, 8-vcdopt->escape_numbits);
			if (value<0) break;
			if (vcdopt->escape_allzeros && 
				(value==0)) { /* extended case */
					if (VCDSkipBits(vcdopt, 16-vcdopt->escape_numbits) < 0) break;
			} else {
				value = VCDGetBits(vcdopt, 8-vcdopt->escape_numbits);
				switch(vcdopt->escape_numbits) {
				case 1:			/* only get sign bit so far */
					if ((value&0x78)==0x78) { /* may need to do something */
						if (vcdopt->thresh[vcdopt->coefindex]==7) {	/* floor anything to -8 */
							VCDChangeBits(vcdopt, 3, 0);
						} else if ((value&0x4)==0x4) { /* (-4:-1) */
							if (vcdopt->thresh[vcdopt->coefindex]>=2) {	/* floor (-1,-4) to -4 */
								VCDChangeBits(vcdopt, 2, 0);
							} else if ((value&0x2)==0x2) { /* (-2:-1) */
								if (vcdopt->thresh[vcdopt->coefindex]==1) {	/* floor (-1) to -2 */
									VCDChangeBits(vcdopt, 1,0);
								}
							}
						}
					} /* else nothing to do */
					break;
				case 2:
					if ((vcdopt->escape_allones) &&
						(value&0x38)==0x38) { /* may need to do something */
						if (vcdopt->thresh[vcdopt->coefindex]==7) {	/* floor anything to -8 */
							VCDChangeBits(vcdopt, 3, 0);
						} else if ((value&0x4)==0x4) { /* (-4:-1) */
							if (vcdopt->thresh[vcdopt->coefindex]>=2) {	/* floor (-1,-4) to -4 */
								VCDChangeBits(vcdopt, 2, 0);
							} else if ((value&0x2)==0x2) { /* (-2:-1) */
								if (vcdopt->thresh[vcdopt->coefindex]==1) {	/* floor (-1) to -2 */
									VCDChangeBits(vcdopt, 1,0);
								}
							}
						}
					} /* else nothing to do */
					break;
				case 3:
					if ((vcdopt->escape_allones) &&
						(value&0x18)==0x18) { /* may need to do something */
						if (vcdopt->thresh[vcdopt->coefindex]==7) {	/* floor anything to -8 */
							VCDChangeBits(vcdopt, 3, 0);
						} else if ((value&0x4)==0x4) { /* (-4:-1) */
							if (vcdopt->thresh[vcdopt->coefindex]>=2) {	/* floor (-1,-4) to -4 */
								VCDChangeBits(vcdopt, 2, 0);
							} else if ((value&0x2)==0x2) { /* (-2:-1) */
								if (vcdopt->thresh[vcdopt->coefindex]==1) {	/* floor (-1) to -2 */
									VCDChangeBits(vcdopt, 1,0);
								}
							}
						}
					} /* else nothing to do */
					break;
				case 4:
					if ((vcdopt->escape_allones) &&
						(value&0x8)==0x8) { /* may need to do something */
						if (vcdopt->thresh[vcdopt->coefindex]==7) {	/* floor anything to -8 */
							VCDChangeBits(vcdopt, 3, 0);
						} else if ((value&0x4)==0x4) { /* (-4:-1) */
							if (vcdopt->thresh[vcdopt->coefindex]>=2) {	/* floor (-1,-4) to -4 */
								VCDChangeBits(vcdopt, 2, 0);
							} else if ((value&0x2)==0x2) { /* (-2:-1) */
								if (vcdopt->thresh[vcdopt->coefindex]==1) {	/* floor (-1) to -2 */
									VCDChangeBits(vcdopt, 1,0);
								}
							}
						}
					} /* else nothing to do */
					break;
				case 5:
					if (vcdopt->escape_allones) { /* may need to do something */
						if (vcdopt->thresh[vcdopt->coefindex]==7) {	/* floor anything to -8 */
							VCDChangeBits(vcdopt, 3, 0);
						} else if ((value&0x4)==0x4) { /* (-4:-1) */
							if (vcdopt->thresh[vcdopt->coefindex]>=2) {	/* floor (-1,-4) to -4 */
								VCDChangeBits(vcdopt, 2, 0);
							} else if ((value&0x2)==0x2) { /* (-2:-1) */
								if (vcdopt->thresh[vcdopt->coefindex]==1) {	/* floor (-1) to -2 */
									VCDChangeBits(vcdopt, 1,0);
								}
							}
						}
					} /* else nothing to do */
					break;
				case 6:
					if (vcdopt->escape_allones) { /* may need to do something */
						if (vcdopt->escape_fix==EF_FLOOR) {
							VCDChangeBits(vcdopt, 2, 0);
						}
					} /* else nothing to do */
					break;
				case 7:
					if (vcdopt->escape_allones) { /* may need to do something */
						if (vcdopt->escape_fix==EF_FLOOR) {
							VCDChangeBits(vcdopt, 1, 0);
						}
					} /* else nothing to do */
					break;
				}
			}
			vcdopt->coefindex++;
			if (vcdopt->coefindex>vcdopt->maxindex) {
				vcdopt->state = RLP_OK_STAGE1;
			} else {
				vcdopt->state = RLP_CHECK_STAGE1;
			}
			break;
		case MACROBLOCK_END:
			value = VCDNextBits(vcdopt, 24);
			if (value<0) {
				vcdopt->state = MACROBLOCK_END;
			} else if (value&0xfffffe) { /* another macroblock */
				vcdopt->state = MACROBLOCK_START;
			} else {
				vcdopt->state = SLICE_START;
			}
			break;
		}

		if (vcdopt->bsDone)		/* run out of buffer, or found next none slice start code */
			break;
	}

	return;
}

static long VCDGetBits(VCDOPT_INFO *vcdopt, long n)
{
	long result;

	if (n > vcdopt->bsBits) {	/* not enough bits in bsData */
		vcdopt->bsData >>= (32 - vcdopt->bsBits);	/* adjust to lsb */
		do {
			if (vcdopt->bsLen <= 0L) {
				/* read buffer is empty, return -(numbits+1) error */
				vcdopt->bsData <<= (32 - vcdopt->bsBits); /* adjust to msb */
				vcdopt->bsDone = 1;
				return -(vcdopt->bsBits+1);
			}
			/* get 8 more bits */
			vcdopt->bsData <<= 8L;
			vcdopt->bsData |= *vcdopt->bsPtr;
			vcdopt->bsPtr++;
			vcdopt->bsBits += 8L;
			vcdopt->bsLen--;
		} while (vcdopt->bsBits < n);
		vcdopt->bsData <<= (32 - vcdopt->bsBits); /* adjust to msb */
	}
	/* got enough bits, return n bits */
	result = vcdopt->bsData >> (32 - n);
	vcdopt->bsBits -= n;
	vcdopt->bsData <<= n;
	return (result);
}

static long VCDSkipBits(VCDOPT_INFO *vcdopt, long n)
{
	if (n > vcdopt->bsBits) {	/* not enough bits in bsData */
		vcdopt->bsData >>= (32 - vcdopt->bsBits);	/* adjust to lsb */
		do {
			if (vcdopt->bsLen <= 0L) {
				/* read buffer is empty, return -(numbits+1) error */
				vcdopt->bsData <<= (32 - vcdopt->bsBits); /* adjust to msb */
				vcdopt->bsDone = 1;
				return -(vcdopt->bsBits+1);
			}
			/* get 8 more bits */
			vcdopt->bsData <<= 8L;
			vcdopt->bsData |= *vcdopt->bsPtr;
			vcdopt->bsPtr++;
			vcdopt->bsBits += 8L;
			vcdopt->bsLen--;
		} while (vcdopt->bsBits < n);
		vcdopt->bsData <<= (32 - vcdopt->bsBits); /* adjust to msb */
	}
	/* got enough bits, return 0 */
	vcdopt->bsBits -= n;
	vcdopt->bsData <<= n;
	return 0;
}

static long VCDNextBits(VCDOPT_INFO *vcdopt, long n)
{
	long result;

	if (n > vcdopt->bsBits) {	/* not enough bits in bsData */
		vcdopt->bsData >>= (32 - vcdopt->bsBits);	/* adjust to lsb */
		do {
			if (vcdopt->bsLen <= 0L) {
				/* read buffer is empty, return -(numbits+1) error */
				vcdopt->bsData <<= (32 - vcdopt->bsBits); /* adjust to msb */
				vcdopt->bsDone = 1;
				return -(vcdopt->bsBits+1);
			}
			/* get 8 more bits */
			vcdopt->bsData <<= 8L;
			vcdopt->bsData |= *vcdopt->bsPtr;
			vcdopt->bsPtr++;
			vcdopt->bsBits += 8L;
			vcdopt->bsLen--;
		} while (vcdopt->bsBits < n);
		vcdopt->bsData <<= (32 - vcdopt->bsBits); /* adjust to msb */
	}
	/* got enough bits, return n bits */
	result = vcdopt->bsData >> (32 - n);
	return (result);
}

static long VCDNextStartCode(VCDOPT_INFO *vcdopt)
{
	long oddbits, result;

	/* align to next byte boundary */
	oddbits = vcdopt->bsBits & 0x7;    /* same as %8 */
	vcdopt->bsData <<= oddbits;	/* shift off odd bits */
	vcdopt->bsBits -= oddbits;	/* decrement  */
	vcdopt->bsData >>= (32 - vcdopt->bsBits); /* shift to lsb */
	/* loop until we find something */
	while (1) {
		/* load up the bsData with 4 bytes */
		while(vcdopt->bsBits < 32) {
			if (vcdopt->bsLen <= 0) {
				/* read buffer is empty, return -(numbits+1) error */
				vcdopt->bsData <<= (32 - vcdopt->bsBits); /* adjust to msb */
				vcdopt->bsDone = 1;
				return -(vcdopt->bsBits+1);
			}				
			vcdopt->bsData <<= 8L;
			vcdopt->bsData |= *vcdopt->bsPtr;
			vcdopt->bsPtr++;
			vcdopt->bsBits += 8L;
			vcdopt->bsLen--;
		}
		
		if ((vcdopt->bsData&0xffffff00)==0x100) { /* found start code */
			result = vcdopt->bsData&0xff;
			vcdopt->bsBits = 0;
			vcdopt->bsData = 0;
			break;
		} else if (vcdopt->bsData&0xfe) {	/* shift off 32 bits */
			vcdopt->bsBits = 0;
		} else if (vcdopt->bsData&0xff00) {	/* shift off 24 bits */
			vcdopt->bsBits = 8;
		} else if (vcdopt->bsData&0xff0000) { /* shift off 16 bits */
			vcdopt->bsBits = 16;
		} else {				/* shift off 8 bits */
			vcdopt->bsBits = 24;
		}
	}

	return result;
}

static void VCDChangeBits(VCDOPT_INFO *vcdopt, long n, long value)
{
	char *chngPtr;
	long mask;
	long data;

	/* first need to load the section of the buffer that we need to change */
	chngPtr = vcdopt->bsPtr - 1 - (vcdopt->bsBits/8);/* this gets us to where the top of the data buffer came from */

	/* adjust to find the first byte we're interested in */
	if ((n+(vcdopt->bsBits&0x7)) > 8) { /* straddles two bytes */
		chngPtr--;
		data = (*chngPtr)<<8;
		data |= *(chngPtr+1);
		mask = ((1<<n)-1)<<(vcdopt->bsBits&0x7);
		/* clear out old bits */
		data &= ~mask;
		data |= ((value<<(vcdopt->bsBits&0x7))&mask);
		/* replace data */
		*chngPtr = (data>>8)&0xff;
		*(chngPtr+1) = data&0xff;
	} else {					/* contained in one byte */
		data = (*chngPtr);
		mask = ((1<<n)-1)<<(vcdopt->bsBits&0x7);
		/* clear out old bits */
		data &= ~mask;
		data |= ((value<<(vcdopt->bsBits&0x7))&mask);
		/* replace data */
		*chngPtr = data&0xff;
	}	
	
	return;
}

static void VCDCheckQuant(VCDOPT_INFO *vcdopt, MPEGStreamInfo *streamInfo)
{
	int i;

	if (vcdopt->ok_intra_matrix ||
		(vcdopt->quantizer_scale*vcdopt->min_intra_quant>=8)) {
		vcdopt->ok_intra_quant_scale = 1;
	} else {
		vcdopt->ok_intra_quant_scale = 0;
		/* form intra level threshold table */
		vcdopt->min_intra_index = 0;
		for (i=0; i<64; i++) {
			vcdopt->intra_quant_thresh[i] =
				7/(streamInfo->intraQuantMatrix[i]*vcdopt->quantizer_scale);
			/* find minimum index to worry about */
			if (vcdopt->intra_quant_thresh[i]>0) {
				vcdopt->min_intra_index = i;
			}
		}
	}
	if (vcdopt->ok_non_intra_matrix ||
		(vcdopt->quantizer_scale*vcdopt->min_non_intra_quant>=8)) {
		vcdopt->ok_non_intra_quant_scale = 1;
	} else {
		vcdopt->ok_non_intra_quant_scale = 0;
		/* form non-intra level threshold table */
		vcdopt->min_non_intra_index = 0;
		for (i=0; i<64; i++) {
			vcdopt->non_intra_quant_thresh[i] =
				7/(streamInfo->nonIntraQuantMatrix[i]*vcdopt->quantizer_scale);
			/* find minimum index to worry about */
			if (vcdopt->non_intra_quant_thresh[i]>0) {
				vcdopt->min_non_intra_index = i;
			}
		}
	}
	return;
}
