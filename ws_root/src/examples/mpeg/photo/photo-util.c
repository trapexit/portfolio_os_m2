
/******************************************************************************
**
**  @(#) photo-util.c 96/02/21 1.2
**
**  MPEG utility functions.
**
******************************************************************************/

#include <misc/mpeg.h>
#include "photo-util.h"

static int32 NextSequenceHeader(char* buffer, uint32 size, char** dest);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

int32
GetMPEGPictureDimension(char* buffer, uint32 size,
						uint32* width, uint32* height)
{
	VideoSequenceHeaderPtr seqPtr = 0;

	if (NextSequenceHeader(buffer, size, (char**) &seqPtr) < 0)
		return -1;

	*width = seqPtr->horizontal_size;
	*height = seqPtr->vertical_size;
	return 0;
}

/* NextSequenceHeader searches for a sequence header in the buffer.
 * If it finds one returns a pointer to the header in dest. If not
 * returns -1.
 */
static int32
NextSequenceHeader(char* buffer, uint32 size, char** dest)
{
	uint32 index, code;

	for (index = 0; index < size; index++, buffer++) {
		code = *((int32 *) buffer);
		if (code == MPEG_VIDEO_SEQUENCE_HEADER_CODE) {
			*dest = buffer;
			return 0;
		}
	}
	return -1;
}
