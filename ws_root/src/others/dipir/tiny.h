/*
 *	@(#) tiny.h 95/09/13 1.3
 *	Copyright 1994,1995, The 3DO Company
 *
 * Defintions related to "tiny" devices.
 */

typedef struct TinyRomTag
{
	uint8	trt_Type;
	uint8	trt_DataSizeHi;
	uint8	trt_DataSizeLo;
	uint8	trt_Data[1];
} TinyRomTag;

#define	SIZE_TRT_HEADER 3
#define TRT_DATASIZE(trt) \
	MakeInt16((trt)->trt_DataSizeHi, (trt)->trt_DataSizeLo)

/*
 * trt_Types in a TinyRomTag table.
 */
#define	TTAG_DEVICENAME	1
	/* Data is the name of the device */
#define	TTAG_DEVICEINFO	2
	/* Data goes into hwdev_DeviceSpecific */
#define	TTAG_ICON	3
	/* Data is an icon (VideoImage) */

typedef void DefaultNameFunction(DDDFile *fd, char *namebuf, uint32 namelen);
