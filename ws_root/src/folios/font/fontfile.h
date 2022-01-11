/* @(#) fontfile.h 95/09/15 1.3 */

#ifndef __FONTFILE_H
#define __FONTFILE_H

#ifndef macintosh
#include <kernel/types.h>

#ifndef AddToPtr
#define AddToPtr(ptr, val) ((void*)((((char *)(ptr)) + (long)(val))))
#endif

#ifndef ArrayElements
#define ArrayElements(a)	(sizeof(a) / sizeof((a)[0]))
#endif

#else
typedef long				int32;
typedef unsigned long		uint32;
typedef short				int16;
typedef unsigned short	uint16;
typedef char				int8;
typedef unsigned char		uint8;
#endif

#define DIAGNOSE(x)				{									\
    printf("Error ("__FILE__"): ");		\
    printf x;							\
}

/*----------------------------------------------------------------------------
 * Font internal datatypes
 *	Client code should not count on these things remaining as they are now.
 *--------------------------------------------------------------------------*/

#define	FONT_VERSION		4

#ifndef CHAR4LITERAL
#define CHAR4LITERAL(a,b,c,d)	((unsigned long) (a<<24)|(b<<16)|(c<<8)|d)
#endif

#define	FORM_LABEL			CHAR4LITERAL('F','O','R','M')
#define CHUNK_FONT			CHAR4LITERAL('F','O','N','T')

typedef struct
{
    int32		fontFamilyID;				/* Font family id */
    int32		fontSize;					/* Font point size */
    int32		fontBitsPerPixel;			/* Pixel depth of each character, as stored in file */
    int32		fontMaxWidth;				/* Max width of character (pixels) */
    int32		fontMaxHeight;				/* Height of character (ascent+descent) */
    int32		fontFlags;					/* Font flags */
    int32		fontFace;					/* Font typeface */
    int32		fontAscent;					/* Distance from baseline to ascentline */
    int32		fontDescent;				/* Distance from baseline to descentline */
    int32		fontSpacing;				/* Spacing between characters */
    int32		fontLeading;				/* Distance from descent line to next ascent line */
    int32		fontFirstChar;				/* First char defined in character set */
    int32		fontLastChar;				/* Last char defined in character set */
    int32		fontTotalChars;				/* Num characters in font set */
}
FontFileDesc;

typedef struct
{
    int32			label;					/* Standard 'FORM' label */
    int32			size;					/* Size of form with form header */
    int32			id;						/* Form type */
}
FormHeader;

typedef struct
{
    FormHeader		form;					/* Standard 'FORM' header */

    int32			chunkID;				/* Standard 3DO chunk file id */
    int32			chunkSize;				/* Size of chunk with chunk header */
    uint8			fontVersion;			/* Version of this particular font */
    uint8			fontRevision;			/* Revision of this particular font */
    int16			chunkVersion;			/* Chunk version for this font */

    FontFileDesc	fontDesc;				/* Font file descriptor */

    uint32			charWidth;				/* Width of character image (pixels) */
    uint32			charHeight;				/* Height of character image (pixels) */
    uint32			charRowBytes;			/* Bytes per row of character image */
    uint32			charSize;				/* Bytes of one entire character image */

    uint32			charWTableOffset;		/* Offset from file beginning to offset/width table */
    uint32			charWTableSize;			/* Size of offset/width table in bytes */
    uint32			charDataOffset;			/* Offset from file beginning to char data */
    uint32			charDataSize;			/* Size of all character data in bytes (uncompressed) */

    uint32			reserved[4];			/* Typical reserved-for-future-expansion */
}
FontHeader;

typedef struct FontCharInfo {
    unsigned int	fci_charOffset : 22;		/* Offset from start of char data to this char */
    unsigned int	fci_unused     :  2;		/* two available bits */
    unsigned int	fci_charWidth  :  8;		/* Width (in pixels) of data for this char */
} FontCharInfo;


int32 GetFontCharInfo (const FontDescriptor *fDesc, int32 character, void **blitInfo);
int32 BlitFontChar    (const FontDescriptor *fd, uint32 theChar, void *blitInfo, void *dstBuf, int32 dstX, int32 dstY, int32 dstBPR, int32 dstBPP, int32 dstWidth, int32 dstHeight);

#endif	/* __FONTFILE_H */
