#ifndef __DIPIR_DIPIRPUB_H
#define __DIPIR_DIPIRPUB_H


/******************************************************************************
**
**  @(#) dipirpub.h 96/08/13 1.32
**
**  Public dipir routines and data structures.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __DIPIR_HWRESOURCE_H
#include <dipir/hwresource.h>
#endif

#ifndef __LOADER_HEADER3DO_H
#include <loader/header3do.h>
#endif


#define	RSA_KEY_SIZE		128	/* Size of the standard key */

/*
 * Layout of boot screen.
 * Banner rectangle is lower half.
 * Icon rectangle is upper right corner.
 */
#define	BANNER_HEIGHT		240	/* Height of banner rectangle */
#define	BANNER_WIDTH		640	/* Width of banner rectangle */

#define	ICON_HEIGHT		96	/* Height of one icon */
#define	ICON_WIDTH		80	/* Width of one icon */
#define	ICON_X_SPACING		8	/* Horizontal space between icons */
#define	ICON_Y_SPACING		8	/* Vertical space between icons */
#define	NUM_X_ICONS		3	/* Max icons on screen, horizontally */
#define	NUM_Y_ICONS		2	/* Max icons on screen, vertically */
#define	ICON_BORDER_COLOR	0x03FF	/* Light blue */

#define	ICON_RECT_WIDTH		((NUM_X_ICONS * ICON_WIDTH) + \
				 ((NUM_X_ICONS-1) * ICON_X_SPACING))
#define	ICON_RECT_HEIGHT	((NUM_Y_ICONS * ICON_HEIGHT) + \
				 ((NUM_Y_ICONS-1) * ICON_Y_SPACING))


/* Context for GenRSADigestXXX routines */
typedef struct DipirDigestContext
{
	uint32 ddc_Info[32];
} DipirDigestContext;

/* Public dipir routines.  Those routines made accessible to the kernel */
typedef struct PublicDipirRoutines
{
  uint32 tableSize;
  int32  (*pdr_SectorECC)(uint8 *buf);
  int32  (*pdr_GenRSADigestInit)(DipirDigestContext *info);
  int32  (*pdr_GenRSADigestUpdate)(DipirDigestContext *info, uchar *inp, uint32 inpLen);
  int32  (*pdr_GenRSADigestFinal)(DipirDigestContext *info, uchar *sig, uint32 sigLen);
  int32  (*pdr_GetHWResource)(HardwareID id, HWResource *buf, uint32 buflen);
  int32	 (*pdr_ReadChannel)(HardwareID id, uint32 offset, uint32 len, void *buf);
  int32	 (*pdr_MapChannel)(HardwareID id, uint32 offset, uint32 len, void **paddr);
  int32	 (*pdr_UnmapChannel)(HardwareID id, uint32 offset, uint32 len);
  int32  (*pdr_GetHWIcon)(HardwareID id, void *buffer, uint32 bufLen);
  int32  (*pdr_SoftReset)(void);
  void * (*pdr_memcpy)(void *dst, const void *src, uint32 numBytes);
  void * (*pdr_memset)(void *dst, uint8 c, uint32 numBytes);
} PublicDipirRoutines;


/*
 * Format of a dipir video image.
 * Used for device icons, application banner screens, etc.
 */
typedef struct VideoImage
{
	uint8		vi_Version;	/* Version # of this header */
	char		vi_Pattern[5];	/* Must contain a SPLASH_PATTERN */
	uint16		vi_ImageID;	/* ID for image replacement */
	uint32		vi_Size;	/* Size of image (bytes) */
	uint16		vi_Height;	/* Height of image (pixels) */
	uint16		vi_Width;	/* Width of image (pixels) */
	uint8		vi_Depth;	/* Depth of each pixel (bits) */
	uint8		vi_Type;	/* Representation */
	uint8		vi_Reserved1;	/* must be zero */
	uint8		vi_Reserved2;	/* must be zero */
	uint32		vi_Reserved3;	/* must be zero */
} VideoImage;

/* Values in vi_Type */
#define	VI_LRFORM	0	/* Old Opera format */
#define	VI_DIRECT	1	/* Uses no CLUT; values go direct to pixels */
#define	VI_STDCLUT	2	/* Uses standard CLUT */
#define	VI_CLUT		3	/* CLUT is prepended to the image */

/* Values in vi_Pattern */
#define	VI_ICON			"ICON-"
#define	VI_APPBANNER		"ABANR"

/* Values in vi_ImageID */
#define	VI_VENTURI_ICON		1
#define	VI_VISA_ICON		2
#define	VI_M2_ICON		3
#define	VI_HOST_ICON		4
#define	VI_GOLDDISC_ICON	5
#define	VI_VISA_0_ICON		6
#define	VI_VISA_1_ICON		7
#define	VI_VISA_2_ICON		8
#define	VI_VISA_3_ICON		9
#define	VI_STORCARD_1K_ICON	10
#define	VI_STORCARD_2K_ICON	11
#define	VI_STORCARD_4K_ICON	12
#define	VI_STORCARD_8K_ICON	13
#define	VI_STORCARD_16K_ICON	14
#define	VI_STORCARD_32K_ICON	15
#define	VI_STORCARD_64K_ICON	16
#define	VI_STORCARD_128K_ICON	17
#define	VI_STORCARD_256K_ICON	18
#define	VI_STORCARD_512K_ICON	19
#define	VI_STORCARD_1M_ICON	20
#define	VI_STORCARD_2M_ICON	21
#define	VI_STORCARD_4M_ICON	22
#define	VI_STORCARD_8M_ICON	23
#define	VI_STORCARD_16M_ICON	24
#define	VI_STORCARD_32M_ICON	25
#define	VI_STORCARD_64M_ICON	26
#define	VI_STORCARD_128M_ICON	27
#define	VI_STORCARD_GEN_ICON	28
#define	VI_VISA_GEN_ICON	29
#define	VI_EXT_MODEM_ICON	32
#define	VI_EXT_MIDI_ICON	33
#define	VI_PC_AUDIN_ICON	34
#define	VI_PC_MODEM_ICON	35

/*
 * Structure passed in to OS (from SYSINFO_TAG_KERNELADDRESS).
 */
typedef struct BootInfo
{
	List		bi_ModuleList;
	List		bi_ComponentList;
	struct Module *	bi_KernelModule;
} BootInfo;

/*
 * Node for boot-time list of OS components (bi_ComponentList).
 */
typedef struct OSComponentNode
{
	Node		n;
	void *		cn_Addr;
	uint32		cn_Size;
} OSComponentNode;

/*****************************************************************************
  Functions
*/

/* In drivers.c */
extern void * DipirSlot(uint32 n);
/* In rsa.c */
extern int32 RSADigestInit(DipirDigestContext *info);
extern int32 RSADigestUpdate(DipirDigestContext *info,
			uchar *input, uint32 inputLen);
extern int32 RSADigestFinal(DipirDigestContext *info,
			uchar *signature, uint32 sigLen);

#endif	/* __DIPIR_DIPIRPUB_H */
