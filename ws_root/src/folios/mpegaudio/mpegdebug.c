/******************************************************************************
**
**  @(#) mpegdebug.c 96/07/17 1.3
**
******************************************************************************/

#ifndef _MPEGAUDIOTYPES_H
#include "mpegaudiotypes.h"
#endif

#include "mpegdebug.h"
#include "decode.h"

#ifdef BUILD_STRINGS

void DEBUGprintparse( FrameInfo *fi, AUDIO_FLOAT *matrixInputSamples )
{
  AudioHeader *hdr = fi->header;
  int32  i, ch, j;

  printf("id = %d\n", hdr->id);
  printf("layer = %d\n", hdr->layer);
  printf("protection_bit = %d\n", hdr->protection_bit);
  printf("bitrate_index = %d\n", hdr->bitrate_index);
  printf("sampling_frequency = %d\n", hdr->sampling_frequency);
  printf("padding_bit = %d\n", hdr->padding_bit);
  printf("private_bit = %d\n", hdr->private_bit);
  printf("mode = %d\n", hdr->mode);
  printf("mode_extension = %d\n", hdr->mode_extension);
  printf("copyright = %d\n", hdr->copyright);
  printf("original = %d\n", hdr->original_or_copy);
  printf("emphasis = %d\n", hdr->emphasis);
  if (hdr->protection_bit == PROTECT_ON) {
    printf("crc_check = %d\n", fi->crc_check);
  }
  printf("table = %d\n", fi->table);
  printf("sblimit = %d\n", fi->sblimit);
  printf("bound = %d\n", fi->bound);
  printf("nch = %d\n", fi->nch);
#if 1
  for (ch=0; ch<fi->nch; ch++) {
	printf("allocation for channel %d", ch);
    for (j=0; j<fi->sblimit; j++) {
      if (!(j%16)) printf("\n%4d:", j);
      printf("%d  ", fi->allocation[2*j+ch]);
    }
    printf("\n");
  }
#endif
#if 1
  for (ch=0; ch<fi->nch; ch++) {
	printf("scalefactor index for channel %d\n", ch);
    for (j=0; j<fi->sblimit; j++) {
	  printf("band %d: %d %d %d\n", j, fi->scalefactor_index[6*j+3*ch],
							fi->scalefactor_index[6*j+3*ch+1], fi->scalefactor_index[6*j+3*ch+2]);
    }
  }
#endif
#if 1
  for (ch=0; ch<fi->nch; ch++) {
	printf("matrixInputSamples for channel %d", ch);
    for (i=0; i<36; i++) {
	  printf("\nmatrix %d", i);
      for (j=0; j<fi->sblimit; j++) {
        if (!(j%6)) printf("\n%4d:", j);
        /* printf("% 5.3e ", matrixInputSamples[ch][32*i+j]); */
        printf("% 5.3e ", *(matrixInputSamples+ch+(32*i+j)));
      }
    }
    printf("\n");
  }
#endif
  return;
}

void DEBUGprintmatrix( FrameInfo *fi, AUDIO_FLOAT *matrixOutputSamples )
{
  int32  i, ch, j;

  for (ch=0; ch<fi->nch; ch++) {
	printf("matrixOutputSamples for channel %d", ch);
    for (i=0; i<36; i++) {
      for (j=0; j<32; j++) {
        if (!(j%6)) printf("\n%4d:", 32*i+j);
        /* printf("% 5.3e ", matrixOutputSamples[ch][32*i+j]); */
        printf("% 5.3e ", *(matrixOutputSamples+ch+(32*i+j)));
      }
    }
    printf("\n");
  }

  return;
}


void DEBUGprintmatrix2( FrameInfo *fi, AUDIO_FLOAT *matrixOutputSamples )
{
  int32  i, ch, j;

  for (ch=0; ch<fi->nch; ch++) {
	printf("matrixOutputSamples for channel %d", ch);
    for (i=0; i<36; i++) {
	  printf("\nmatrix %d", i);
      for (j=0; j<64; j++) {
        if (!(j%6)) {
	  printf("\n%4d:", j);
	}
        /* if (j<16) printf("% 5.3e ", matrixOutputSamples[ch][2*i+72*j]); */
        if (j<16) printf("% 5.3e ", *(matrixOutputSamples+ch+(2*i+72*j)));
        if (j==16) printf("% 5.3e ", 0.0);

/*
        if ((j>16) && (j<33))
			printf("% 5.3e ",
				-matrixOutputSamples[ch][2*i+72*(32-j)]);
		if ((j>32) && (j<48))
			printf("% 5.3e ",
				-matrixOutputSamples[ch][2*i+1+72*(j-32)]);
*/
        if ((j>16) && (j<33))
			printf("% 5.3e ",
				- *(matrixOutputSamples+ch+(2*i+72*(32-j))));
		if ((j>32) && (j<48))
			printf("% 5.3e ",
				- *(matrixOutputSamples+ch+(2*i+1+72*(j-32))));
/*
        if (j==48) printf("% 5.3e ", -matrixOutputSamples[ch][2*i+1]);
        if (j>48) printf("% 5.3e ",
					-matrixOutputSamples[ch][2*i+1+72*(64-j)]);
*/
        if (j==48) printf("% 5.3e ",
					- *(matrixOutputSamples+ch+(2*i+1)));
        if (j>48) printf("% 5.3e ",
					-  *(matrixOutputSamples+ch+(2*i+1+72*(64-j))));
      }
    }
    printf("\n");
  }

  return;
}


void DEBUGprintwindow()
{
#if 0				/* theOutputBuffer is a bogus array */
  int32  i;

  printf("Window output samples");
  for (i=0; i<1152; i++) {
    if (!(i%4)) printf("\n%4d:", i);
    printf("0x%08x  ", theOutputBuffer[i]);
  }
  printf("\n");
#endif
  return;
}

#endif
