/* @(#) cdtype.c 96/03/18 1.6 */

/*
 * Determine type of a CD.
 */

#include <kernel/types.h>
#include <string.h>
#include <stdio.h>
#include <kernel/mem.h>
#include <kernel/devicecmd.h>
#include <kernel/driver.h>
#include <kernel/io.h>
#include <file/discdata.h>
#include <device/cdrom.h>


/*****************************************************************************/


static bool videoCD;
static bool photoCD;
static bool checkedISO;


/*****************************************************************************/


/* root directory block on an ISO-9660 disc */
typedef struct VolumeDescriptor
{
    uint8 type;                         /* type of this descriptor */
    uint8 formatStandard[5];            /* the format of this volume */
    uint8 version;                      /* version of this descriptor */
    uint8 filler1;                      /* reserved */
    uint8 systemName[32];               /* name of the host system */
    uint8 volumeName[32];               /* volume name */
    uint8 filler2[8];                   /* reserved */
    uint8 volumeSize_le[4];             /* volume size (little-endian) */
    uint8 volumeSize[4];                /* volume size (big-endian) */
    uint8 filler3[32];                  /* reserved */
    uint8 volumeSetCount_le[2];         /* number of volumes in volume set (little-endian) */
    uint8 volumeSetCount[2];            /* number of volumes in volume set (big-endian) */
    uint8 volumeSetIndex_le[2];         /* index of this volume in volume set (little-endian) */
    uint8 volumeSetIndex[2];            /* index of this volume in volume set (big-endian) */
    uint8 logicalBlockSize_le[2];       /* size of a block on this volume (little-endian) */
    uint8 logicalBlockSize[2];          /* size of a block on this volume (big-endian) */
    uint8 pathTableSize_le[4];          /* size of path table in bytes (little-endian) */
    uint8 pathTableSize[4];             /* size of path table in bytes (big-endian) */
    uint8 pathTabDiskAddr_le[4];        /* disk address of L-type path table (little-endian) */
    uint8 pathTabDiskAddr2_le[4];       /* disk address of redundant L-type path table (little-endian) */
    uint8 pathTabDiskAddr[4];           /* disk address of M-type path table (big-endian) */
    uint8 pathTabDiskAddr2[4];          /* disk address of redundant M-type path table (big-endian) */
    uint8 rootDirectoryEntry[34];       /* root directory information */
    uint8 volumeSetName[128];           /* volume set identification */
    uint8 publisherName[128];           /* publisher identification */
    uint8 dataPreparerName[128];        /* data preparer identification */
    uint8 applicationName[128];         /* application identification */
    uint8 copyrightFilePathName[37];    /* path name of file containing copyright */
    uint8 abstractFilePathName[37];     /* path name of file containing abstract */
    uint8 bibliographicFilePathName[37];/* path name of file containing bibliography */
    uint8 creationTime[17];             /* volume creation time */
    uint8 modificationTime[17];         /* volume modification time */
    uint8 expirationTime[17];           /* volume information expiration date */
    uint8 effectiveTime[17];            /* volume information effective after date */
    uint8 formatVersion;                /* version of volume format */
    uint8 filler4;                      /* reserved */
} VolumeDescriptor;

/* an entry in a ISO-9660 path table */
typedef struct PathTableEntry
{
    uint8 nameLength;           /* length of directory name             */
    uint8 extendedLength;       /* length of extended-attribute record  */
    uint8 discAddr[4];          /* first block of extent of directory   */
    uint8 parentDirectoryID[2]; /* path table index of parent directory */
} PathTableEntry;


/*****************************************************************************/


/* some helpful macros for ISO-9660 parsing */
#define FRAME_SIZE         DISC_BLOCK_SIZE
#define FRAMES_PER_SECOND  75
#define GetFrameNum(m,s,f) (((uint32)m * 60 + (uint32)s) * FRAMES_PER_SECOND + (uint32)f)


/*****************************************************************************/


static Err 
GetCDStatus(Item ioreq, DeviceStatus *ds)
{
	IOInfo ioInfo;

	memset(&ioInfo,0,sizeof(ioInfo));
	ioInfo.ioi_Command         = CMD_STATUS;
	ioInfo.ioi_Recv.iob_Buffer = ds;
	ioInfo.ioi_Recv.iob_Len    = sizeof(DeviceStatus);

	return (DoIO(ioreq, &ioInfo));
}


/*****************************************************************************/


static Err 
GetCDData(Item ioreq, struct CDROM_Disc_Data *cddd)
{
	IOInfo ioInfo;

	memset(&ioInfo, 0, sizeof(IOInfo));

	ioInfo.ioi_Command         = CDROMCMD_DISCDATA;
	ioInfo.ioi_Flags           = 0;
	ioInfo.ioi_Send.iob_Buffer = 0;
	ioInfo.ioi_Send.iob_Len    = 0;
	ioInfo.ioi_Recv.iob_Buffer = cddd;
	ioInfo.ioi_Recv.iob_Len    = sizeof(struct CDROM_Disc_Data);

	return DoIO(ioreq, &ioInfo);
}


/*****************************************************************************/


static Err 
GetFrames(Item ioreq, uint32 startFrame, uint32 numFrames, void *destination)
{
	IOInfo ioInfo;

	memset(&ioInfo, 0, sizeof(IOInfo));

	ioInfo.ioi_Command         = CMD_BLOCKREAD;
	ioInfo.ioi_Flags           = 0;
	ioInfo.ioi_Offset          = startFrame;
	ioInfo.ioi_CmdOptions      = 0;
	ioInfo.ioi_Send.iob_Buffer = 0;
	ioInfo.ioi_Send.iob_Len    = 0;
	ioInfo.ioi_Recv.iob_Buffer = destination;
	ioInfo.ioi_Recv.iob_Len    = numFrames * FRAME_SIZE;

	return DoIO(ioreq, &ioInfo);
}


/*****************************************************************************/


/* compare a length limited string with a NULL-terminated one */
static bool 
CompareStr(char *chars, int32 count, char *string)
{
	while (count--)
	{
		if (*string++ != *chars++)
		return FALSE;
	}

	/* All the characters matched, now make sure the strings have
	* the same length
	*/
	if (*string)
		return FALSE;
	return TRUE;
}


/*****************************************************************************/


/* convert 4 bytes into a uint32 */
static uint32 
Int32Cast(uint8 raw[4])
{
	return ((uint32) raw[0] << 24) |
		((uint32) raw[1] << 16) |
		((uint32) raw[2] << 8)  |
		(uint32) raw[3];
}


/*****************************************************************************/


/* convert 2 bytes into a uint32 */
static uint32 
Int16Cast(uint8 raw[2])
{
	return ((uint32) raw[0] << 8) | (uint32) raw[1];
}


/*****************************************************************************/


/* search for a name within a path table */
static bool 
FindPathEntry(PathTableEntry *pte, char *name)
{
	/* Visit all entries until an empty entry is found, unless the
	 * desired entry is found first
	 */

	while (pte->nameLength)
	{
		/* we only want entries in the root directory */
		if (Int16Cast(pte->parentDirectoryID) == 1 &&
		    CompareStr((char *)pte + sizeof(PathTableEntry),
				(int32)pte->nameLength, name))
		{
			return TRUE;
		}

		pte = (PathTableEntry *) 
		    ((char *)pte + sizeof(PathTableEntry) + pte->nameLength);

		/* PathTableEntry structures are always 16-bit aligned */
		if ((uint32)pte & 1)
			pte = (PathTableEntry*) ((char*) pte + 1);
	}
	return FALSE;
}


/*****************************************************************************/


/* check if we've got a VideoCD or a PhotoCD */
static void 
CheckISOCD(Item ioreq)
{
	VolumeDescriptor *vd;
	uint32 track1Frame;
	uint32 sessionFrame;
	uint32 pteBytes;
	uint32 pteFrames;
	PathTableEntry *pte;
	struct CDROM_Disc_Data cddd;

	/* only do this code once */
	if (checkedISO)
		return;
	checkedISO = TRUE;

	vd = (VolumeDescriptor *)AllocMem(FRAME_SIZE, MEMTYPE_ANY);
        if (vd == NULL)
		return;

	if (GetCDData(ioreq, &cddd) < 0)
	{
		FreeMem(vd, FRAME_SIZE);
		return;
	}

	track1Frame = GetFrameNum(cddd.TOC[1].minutes,
				  cddd.TOC[1].seconds,
				  cddd.TOC[1].frames);

	/* Determine the address of the last session, if
	 * this is a multi-session disc
	 */
	sessionFrame = 0;
	if (cddd.session.valid)
	{
		/* retain session address relative to beginning of track one */
		sessionFrame = GetFrameNum(cddd.session.minutes,
					cddd.session.seconds,
					cddd.session.frames) - track1Frame;
	}

	/* read the root block */
	if (GetFrames(ioreq, 16 + sessionFrame + track1Frame, 1, vd) < 0)
	{
		FreeMem(vd, FRAME_SIZE);
		return;
	}

	/* is this an ISO-9660 format? */
	if (CompareStr(vd->formatStandard, 5, "CD001"))
	{
		pteBytes  = Int32Cast(vd->pathTableSize);
		pteFrames = ((pteBytes + FRAME_SIZE - 1) / FRAME_SIZE);

		pte = (PathTableEntry *)
			AllocMem(pteFrames * FRAME_SIZE, MEMTYPE_ANY);
		if (pte == NULL)
		{
			FreeMem(vd, FRAME_SIZE);
			return;
		}
		/* read the path table for the root */
		if (GetFrames(ioreq, 
			Int32Cast(vd->pathTabDiskAddr) + track1Frame,
			pteFrames, pte) >= 0)
		{
			/* try and find interesting directories... */
			videoCD = FindPathEntry(pte,"MPEGAV");
			photoCD = FindPathEntry(pte,"PHOTO_CD");
		}
		FreeMem(pte, pteFrames * FRAME_SIZE);
	}
	FreeMem(vd, FRAME_SIZE);
}


/*****************************************************************************/


static bool 
IsAudioCD(Item ioreq, DeviceStatus *ds)
{
	TOUCH(ioreq);
	if (ds->ds_DeviceBlockSize == 2352)
		return TRUE;
	return FALSE;
}


/*****************************************************************************/

#define OPERA_LABEL_MINUTES 0
#define OPERA_LABEL_SECONDS 2
#define OPERA_LABEL_OFFSET  0
#define OPERA_LABEL_FRAME   (((OPERA_LABEL_MINUTES * 60 + OPERA_LABEL_SECONDS) * FRAMES_PER_SECOND) + OPERA_LABEL_OFFSET)


static bool 
IsOperaCD(Item ioreq, DeviceStatus *ds)
{
	DiscLabel *vl;
	uint32 i;

	if (ds->ds_DeviceBlockSize != 2048)
		return FALSE;
	/* FIXME */
	vl = (DiscLabel *) AllocMem(FRAME_SIZE, MEMTYPE_ANY);
	if (vl == NULL)
		return FALSE;

	if (GetFrames(ioreq, OPERA_LABEL_FRAME, 1, vl) < 0)
	{
		FreeMem(vl, FRAME_SIZE);
		return FALSE;
	}

	if (vl->dl_RecordType != RECORD_STD_VOLUME)
		return FALSE;
	for (i = 0;  i < VOLUME_SYNC_BYTE_LEN;  i++)
		if (vl->dl_VolumeSyncBytes[i] != VOLUME_SYNC_BYTE)
			return FALSE;
	if (vl->dl_VolumeFlags != 0)
		return FALSE;

	return TRUE;
}


/*****************************************************************************/


static bool 
IsNoCD(Item ioreq, DeviceStatus *ds)
{
	TOUCH(ioreq);
	if ((ds->ds_DeviceFlagWord & CDROM_STATUS_DISC_IN) == 0)
		return TRUE;
	return FALSE;
}


/*****************************************************************************/


static bool 
IsVideoCD(Item ioreq, DeviceStatus *ds)
{
	if (ds->ds_DeviceBlockSize != 2048)
		return FALSE;
	CheckISOCD(ioreq);
	return videoCD;
}


/*****************************************************************************/


static bool 
IsPhotoCD(Item ioreq, DeviceStatus *ds)
{
	if (ds->ds_DeviceBlockSize != 2048)
		return FALSE;
	CheckISOCD(ioreq);
	return photoCD;
}


/*****************************************************************************/


typedef struct NavikenLabel
{
    char	nl_Reserved1[9];
    char	nl_VolumeStrings[5];
    uint8	nl_VolumeVersion;
    char	nl_Reserved2;
    char	nl_SystemID[32];
} NavikenLabel;

#define NAVIKEN_LABEL_MINUTES 0
#define NAVIKEN_LABEL_SECONDS 2
#define NAVIKEN_LABEL_OFFSET  16
#define NAVIKEN_LABEL_FRAME   (((NAVIKEN_LABEL_MINUTES * 60 + NAVIKEN_LABEL_SECONDS) * FRAMES_PER_SECOND) + NAVIKEN_LABEL_OFFSET)


static bool 
IsNavikenCD(Item ioreq, DeviceStatus *ds)
{
	NavikenLabel *nl;

	if (ds->ds_DeviceBlockSize != 2048)
		return FALSE;

	nl = (NavikenLabel *) AllocMem(FRAME_SIZE, MEMTYPE_ANY);
        if (nl == NULL)
		return FALSE;

	if (GetFrames(ioreq, NAVIKEN_LABEL_FRAME, 1, nl) < 0)
	{
		FreeMem(nl,FRAME_SIZE);
		return FALSE;
	}
	if (strncmp(nl->nl_VolumeStrings, "NSRAJ", 5) == 0 &&
	    nl->nl_VolumeVersion == 0x02 &&
	    strcmp(nl->nl_SystemID, "NAVIGATION SYSTEM") == 0)
	{
		FreeMem(nl, FRAME_SIZE);
		return TRUE;
        }
	FreeMem(nl, FRAME_SIZE);
	return FALSE;
}


/*****************************************************************************/


typedef bool CheckCDTypeFunc(Item ioreq, DeviceStatus *ds);

typedef struct CDType
{
	CheckCDTypeFunc *cdt_CheckFunc;
	char            *cdt_Type;
} CDType;

static const CDType cdTypes[] =
{
    { IsNoCD,		"empty" },
    { IsAudioCD,	"audio" },
    { IsVideoCD,	"video" },
    { IsPhotoCD,	"photo" },
    { IsOperaCD,	"opera" },
    { IsNavikenCD,	"naviken" },
};

#define NUM_TYPES     (sizeof(cdTypes) / sizeof(CDType))


/*****************************************************************************/


/**
|||	AUTODOC -private -class LibROMApp -name RomAppMediaType
|||	Determine type of RomApp media.
|||
|||	  Synopsis
|||
|||	    const char *RomAppMediaType(void);
|||
|||	  Description
|||
|||	    This function examines the RomApp media (disc) in the system
|||	    and determines the type of the media.
|||
|||	  Arguments
|||
|||	    None.
|||
|||	  Return Value
|||
|||	    A pointer to a character string describing the type of media.
|||	    The string will be one of: "audio", "video", "photo", 
|||	    "naviken", "opera", "unknown", or "empty".
|||
**/

const char *
RomAppMediaType(void)
{
	Item media;
	Item ioreq;
	uint32 i;
	DeviceStatus ds;
	char *disctype;

	media = OpenRomAppMedia();
	if (media < 0)
	{
#ifdef BUILD_STRINGS
		printf("Cannot open RomApp media, err %x\n", media);
#endif
		return "empty";
	}
	ioreq = CreateIOReq(0,0,media,0);
	if (ioreq < 0)
	{
		printf("Cannot create IOReq for RomApp media\n");
		CloseDeviceStack(media);
		return "unknown";
	}

	if (GetCDStatus(ioreq, &ds) < 0)
	{
		printf("Cannot get status of RomApp media\n");
		DeleteIOReq(ioreq);
		CloseDeviceStack(media);
		return "unknown";
	}

	disctype = "unknown";
	for (i = 0; i < NUM_TYPES; i++)
	{
		if ((*cdTypes[i].cdt_CheckFunc)(ioreq, &ds))
		{
			disctype = cdTypes[i].cdt_Type;
			break;
		}
	}
	DeleteIOReq(ioreq);
	CloseDeviceStack(media);
	return disctype;
}

