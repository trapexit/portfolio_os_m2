
#include <stdio.h>
#include <kernel/types.h>
#include <kernel/operror.h>

/*****************************************************************************/

/* Types for __vfscanf */
typedef int (* GetCFunc)(void *userData);
typedef int (* UnGetCFunc)(int, void *userData);

/*****************************************************************************/

/* Prototypes for internals ... */
extern FILE *CreateFH(uint8 btype, uint8 *buff, uint32 bsize);
extern void DeleteFH(FILE *f);
extern int  closenodelete(FILE *f);
extern bool SetFHBuffering(FILE *f, uint8 type, uint8 *buffer, uint32 bsize);
extern int __vfscanf(const char *fmt, va_list va, 
					 GetCFunc get, UnGetCFunc unget, void *userData);

/*****************************************************************************/

/* Internal specifics */

struct FILE
{
	uint8			*siof_Buffer;
	uint32			 siof_BufferLength;
	uint32			 siof_BufferActual;
	uint8			 siof_Flags;
	RawFile			*siof_RawFile;
	FileOpenModes	 	 siof_Mode;
	uint8			 siof_UngotChar;
	uint8			 siof_AutoDeleteFilename[80];
};

/* Bit definitions for siof_flags */

#define	SIOF_TYPE_MASK		 1
#define SIOF_TYPE_LINEMODE	 1
#define SIOF_TYPE_BLOCKMODE	 0

/* If SIOF_BUFFER_SYSALLOC is set, the system allocated the buffer,
 * and the system will attempt to free it upon cleanup.
 */
#define SIOF_BUFFER_SYSALLOC	 2

/* If this descriptor depends on RawFile, the next bit will be set. */
#define SIOF_RAWFILE		 4

/* If this descriptor depends on Console IO, the next bit will be set. */
#define SIOF_CONSOLE		 8 

/* If siof_UngotChar contains a valid character that someone used ungetc() on */
#define SIOF_UNGOTCHAR		16

/* If the given file should be deleted on closing */
#define SIOF_AUTODELETE		32

/* If the descriptor is owned by the system, the user can't delete it.  (stdin, stdout) */
#define SIOF_OWNEDSYSTEM	64

/* Note: No buffering is indicated by a NULL siof_Buffer pointer */

/*****************************************************************************/

