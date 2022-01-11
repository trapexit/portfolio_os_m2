#ifndef __DEVICE_CDROM_H
#define __DEVICE_CDROM_H


/******************************************************************************
**
**  @(#) cdrom.h 96/02/20 1.17
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_DEVICECMD_H
#include <kernel/devicecmd.h>
#endif


/********
 In all of the following enum classes, a value of 0 means "not
 specified by caller, use the driver default".
********/

#define CDROM_Default_Option 0

enum CDROM_DensityCodes {
  CDROM_DEFAULT_DENSITY = 1,
  CDROM_DATA,
  CDROM_MODE2_XA,
  CDROM_DIGITAL_AUDIO,
};

enum CDROM_Error_Recovery {
  CDROM_DEFAULT_RECOVERY = 1,
  CDROM_CIRC_RETRIES_ONLY,
  CDROM_BEST_ATTEMPT_RECOVERY
};

enum CDROMAddressFormat {
  CDROM_Address_Blocks = 1,
  CDROM_Address_Abs_MSF
};

enum CDROM_Speed {
  CDROM_SINGLE_SPEED = 1,
  CDROM_DOUBLE_SPEED,
  CDROM_4X_SPEED,
  CDROM_6X_SPEED,
  CDROM_8X_SPEED
};

enum CDROM_Pitch {
  CDROM_PITCH_SLOW = 1,
  CDROM_PITCH_NORMAL,
  CDROM_PITCH_FAST
};

enum CDROM_Block_Sizes {
  CDROM_AUDIO		      = 2352,
  CDROM_AUDIO_SUBCODE	      = 2448,
  CDROM_MODE1		      = 2048,
  CDROM_MODE2FORM1	      = 2048,
  CDROM_MODE2FORM1_SUBHEADER  = 2056,
  CDROM_MODE2FORM2	      = 2324,
  CDROM_MODE2FORM2_SUBHEADER  = 2332
};

enum CDROM_Disc_IDs {
  CDROM_DISC_AUDIO_OR_MODE1	= 0x00,
  CDROM_DISC_CDI		= 0x10,
  CDROM_DISC_MODE2		= 0x20
};

typedef struct CDROM_MSF {
  uint8	rfu;
  uint8	minutes;
  uint8	seconds;
  uint8	frames;
} CDROM_MSF;

/*
  N.B.  The "retryShift" field is meaningful if and only if the errorRecovery
  field contains a meaningful value.  Specifying 0 for the retryShift field
  does _not_ mean "use the default value"!  The default value for retries
  is used iff the errorRecovery field contains the default.
*/

typedef union CDROMCommandOptions {
  uint32 asLongword;
  struct {
    unsigned int     reserved      : 5;
    unsigned int     densityCode   : 3;
    unsigned int     errorRecovery : 2;
    unsigned int     addressFormat : 2;
    unsigned int     retryShift    : 3; /* (2^N)-1 retries */
    unsigned int     speed         : 3;
    unsigned int     pitch         : 2;
    unsigned int     blockLength   : 12;
  } asFields;
} CDROMCommandOptions;

typedef struct SubQInfo {
    uint8   validByte;
    uint8   addressAndControl;
    uint8   trackNumber;
    uint8   Index;                  /* Or Point */
    uint8   minutes;
    uint8   seconds;
    uint8   frames;
    uint8   reserved;
    uint8   aminutes;               /* Or PMIN */
    uint8   aseconds;               /* Or PSEC */
    uint8   aframes;                /* Or PFRAME */
} SubQInfo;

typedef struct CDDiscInfo {
  uint8	discID;
  uint8	firstTrackNumber;
  uint8	lastTrackNumber;
  uint8	minutes;
  uint8	seconds;
  uint8	frames;
} CDDiscInfo;

typedef struct CDTOCInfo {
  uint8	reserved0;
  uint8	addressAndControl;
  uint8	trackNumber;
  uint8	reserved3;
  uint8	minutes;
  uint8	seconds;
  uint8	frames;
  uint8	reserved7;
} CDTOCInfo;

typedef struct CDSessionInfo {
  uint8	valid;
  uint8	minutes;
  uint8	seconds;
  uint8	frames;
  uint8	rfu[2];
} CDSessionInfo;

typedef struct CDROM_Disc_Data {
  CDDiscInfo	info;
  CDTOCInfo	TOC[100];
  CDSessionInfo	session;
  CDSessionInfo	firstsession;
} CDROM_Disc_Data;

typedef struct CDWobbleInfo {
  uint32	LowKHzEnergy;
  uint32	HighKHzEnergy;
  uint8		RatioWhole;
  uint8		RatioFraction;
} CDWobbleInfo;

#define CD_CTL_PREEMPHASIS    0x01
#define CD_CTL_COPY_PERMITTED 0x02
#define CD_CTL_DATA_TRACK     0x04
#define CD_CTL_FOUR_CHANNEL   0x08
#define CD_CTL_QMASK          0xF0
#define CD_CTL_Q_NONE         0x00
#define CD_CTL_Q_POSITION     0x10
#define CD_CTL_Q_MEDIACATALOG 0x20
#define CD_CTL_Q_ISRC         0x30

/* status flag bits */
#define CDROM_STATUS_DOOR         0x00000100 /* drawer is closed */
#define CDROM_STATUS_DISC_IN      0x00000080 /* disc is present */
#define CDROM_STATUS_SPIN_UP      0x00000040 /* disc is spinning */
#define CDROM_STATUS_8X_SPEED	  0x00000020 /* drive is in 8x speed mode */
#define CDROM_STATUS_6X_SPEED	  0x00000010 /* drive is in 6x speed mode */
#define CDROM_STATUS_4X_SPEED	  0x00000008 /* drive is in 4x speed mode */
#define CDROM_STATUS_DOUBLE_SPEED 0x00000004 /* drive is in double speed mode */
#define CDROM_STATUS_ERROR        0x00000002
#define CDROM_STATUS_READY        0x00000001 /* drive ready (attempted to read TOC) */

/*****************************************************************************/


#endif /* __DEVICE_CDROM_H */
