
/* @(#) iohandle.c 96/05/08 1.1 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <kernel/mem.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/

void FreeFHBuffer(FILE *f)
{
	if (f->siof_Buffer)
	{
		if (f->siof_Flags & SIOF_BUFFER_SYSALLOC)
			FreeMem(f->siof_Buffer, f->siof_BufferLength);
		f->siof_Buffer = (uint8 *) NULL;
	}
}

/*****************************************************************************/

bool SetFHBuffering(FILE *f, uint8 type, uint8 *buffer, uint32 bsize)
{
	
	bool	ret = TRUE;				/* Default return to correct */
	bool	sysalloc = FALSE;		/* Default that user alloc'd buffer */

	switch (type)
	{
		/* Block buffering */
		/* Line buffering */
		case _IOFBF:
		case _IOLBF:
		{
			/* If they didn't pass in a buffer, allocate one */
			if (!buffer)
			{
				buffer = AllocMem(bsize, MEMTYPE_ANY);
				if (!buffer)
				{
					/* Fail if we couldn't allocate one */
					ret = FALSE;
					break;
				}
				else
					sysalloc = TRUE;
			}
			
			/* As long as we now have a buffer, free any existing */
			FreeFHBuffer(f);

			/* Set this FILE to use the new buffer */
			f->siof_Buffer = buffer;
			f->siof_BufferLength = bsize;

			/* Set the correct Flags bit declaring LINE or BLOCK mode */
			/* And that determine who allocated the buffer */

			/* Clear out previous bits */
			f->siof_Flags &= (~(SIOF_TYPE_MASK | SIOF_BUFFER_SYSALLOC));

			/* Set bits depending on block or line mode */
			f->siof_Flags |= ((type == _IOFBF) ? 
							(SIOF_TYPE_BLOCKMODE) : (SIOF_TYPE_LINEMODE));

			/* Set bits determining who allocated the buffer */
			f->siof_Flags |= (sysalloc ? (SIOF_BUFFER_SYSALLOC) : (0));
			
			break;
		}

		/* No buffering */
		case _IONBF:
		{
			/* Free any existing */
			FreeFHBuffer(f);
			break;
		}
		default:
			ret = FALSE;
			break;
	}
	
	return(ret);
}


/*****************************************************************************/

FILE *CreateFH(uint8 btype, uint8 *buff, uint32 bsize)
{
	FILE 	*fh;

	fh = (FILE *) AllocMem(sizeof(FILE), MEMTYPE_ANY);
	if (fh)
	{
		/* Preinitialize structure elements to 0 */
		memset(fh, 0, sizeof(FILE));

		/* What kind of buffering? */
		if (SetFHBuffering(fh, btype, buff, bsize))
		{
			return(fh);
		}

		FreeMem(fh, sizeof(FILE));	
	}
	return ( (FILE *) NULL);
}

/*****************************************************************************/

void DeleteFH(FILE *f)
{
	FreeFHBuffer(f);
	FreeMem(f, sizeof(FILE));
}

/*****************************************************************************/


