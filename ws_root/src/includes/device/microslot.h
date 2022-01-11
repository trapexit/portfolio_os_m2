#ifndef __DEVICE_MICROSLOT_H
#define __DEVICE_MICROSLOT_H


/******************************************************************************
**
**  @(#) microslot.h 96/02/17 1.14
**
**  Microslot driver definitions.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_DEVICECMD_H
#include <kernel/devicecmd.h>
#endif


/*****************************************************************************/


#define MakeUSErr(svr,class,err) MakeErr(ER_DEVC, ER_USLOT,svr,ER_E_SSTM,class,err)

#define USLOT_ERR_OFFLINE MakeUSErr(ER_SEVERE, ER_C_STND, ER_DeviceOffline)
#define USLOT_ERR_BADCARD MakeUSErr(ER_SEVERE, ER_C_STND, ER_MediaError)


/*****************************************************************************/


typedef struct MicroSlotStatus
{
    DeviceStatus us_ds;               /* standard stuff */
    uint8        us_CardType;         /* type of card, see below              */
    uint8        us_CardVersion;      /* version code                         */
    bool         us_CardDownloadable; /* whether the card has ROM to download */
    uint8        us_ClockSpeed;       /* current clock speed, see below       */
} MicroSlotStatus;

/* currently known card types */
typedef enum USlotCardTypes
{
    USLOT_CARDTYPE_STORAGE,
    USLOT_CARDTYPE_DIAG
} USlotCardTypes;

/* possible clock speeds */
typedef enum USlotClockSpeeds
{
    USLOT_CLKSPEED_1MHZ,
    USLOT_CLKSPEED_2MHZ,
    USLOT_CLKSPEED_4MHZ,
    USLOT_CLKSPEED_8MHZ
} USlotClockSpeeds;


/*****************************************************************************/


typedef enum USlotSequenceCommands
{
    USSCMD_END,           /* mark the end of a sequence */
    USSCMD_READ,          /* read into buffer           */
    USSCMD_WRITE,         /* write from buffer          */
    USSCMD_VERIFY,        /* read & compare with buffer */
    USSCMD_STARTDOWNLOAD  /* start a ROM download       */
} USlotSequenceCommands;

/* used with the USLOTCMD_SEQ command */
typedef struct MicroSlotSeq
{
    USlotSequenceCommands uss_Cmd;
    void                 *uss_Buffer;
    uint32                uss_Len;
} MicroSlotSeq;


/*****************************************************************************/


#endif /* __DEVICE_MICROSLOT_H */
