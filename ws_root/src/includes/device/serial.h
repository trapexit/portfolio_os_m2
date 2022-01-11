#ifndef __DEVICE_SERIAL_H
#define __DEVICE_SERIAL_H


/******************************************************************************
**
**  @(#) serial.h 96/01/22 1.6
**
**  Standard serial interface definitions.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif


/*****************************************************************************/


typedef enum SerHandshake
{
    SER_HANDSHAKE_NONE,
    SER_HANDSHAKE_HW,
    SER_HANDSHAKE_SW
} SerHandshake;

typedef enum SerWordLength
{
    SER_WORDLENGTH_5,
    SER_WORDLENGTH_6,
    SER_WORDLENGTH_7,
    SER_WORDLENGTH_8
} SerWordLength;

typedef enum SerParity
{
    SER_PARITY_NONE,
    SER_PARITY_EVEN,
    SER_PARITY_ODD,
    SER_PARITY_MARK,
    SER_PARITY_SPACE
} SerParity;

typedef enum SerStopBits
{
    SER_STOPBITS_1,
    SER_STOPBITS_2
} SerStopBits;

/* configuration packet for SER_CMD_SETCONFIG and SER_CMD_GETCONFIG */
typedef struct SerConfig
{
    uint32        sc_BaudRate;
    SerHandshake  sc_Handshake;
    SerWordLength sc_WordLength;
    SerParity     sc_Parity;
    SerStopBits   sc_StopBits;
    uint32        sc_OverflowBufferSize;
} SerConfig;


/*****************************************************************************/


/* status structure for use with SER_CMD_STATUS */
typedef struct SerStatus
{
    uint32 ss_Flags;              /* see definitions below                */
    uint32 ss_NumOverflowErrors;  /* # overflow errors since reset        */
    uint32 ss_NumFramingErrors;   /* # framing errors since reset         */
    uint32 ss_NumParityErrors;    /* # parity errors since reset          */
    uint32 ss_NumBreaks;          /* # break signals received since reset */
    uint32 ss_NumDroppedBytes;    /* # bytes that weren't read in time    */
} SerStatus;

/* values for ss_Flags */
#define SER_STATE_CTS      0x00000001
#define SER_STATE_DSR      0x00000002
#define SER_STATE_DCD      0x00000004
#define SER_STATE_RING     0x00000008
#define SER_STATE_LOOPBACK 0x00010000
#define SER_STATE_RTS      0x00020000
#define SER_STATE_DTR      0x00040000


/*****************************************************************************/


/* Event mask values for SER_CMD_WAITEVENT */
#define SER_EVENT_CTS_SET        0x00000001
#define SER_EVENT_DSR_SET        0x00000002
#define SER_EVENT_DCD_SET        0x00000004
#define SER_EVENT_RING_SET       0x00000008

#define SER_EVENT_CTS_CLEAR      0x00010000
#define SER_EVENT_DSR_CLEAR      0x00020000
#define SER_EVENT_DCD_CLEAR      0x00040000
#define SER_EVENT_RING_CLEAR     0x00080000

#define SER_EVENT_OVERFLOW_ERROR 0x00001000
#define SER_EVENT_PARITY_ERROR   0x00002000
#define SER_EVENT_FRAMING_ERROR  0x00004000
#define SER_EVENT_BREAK          0x00008000


/*****************************************************************************/


/* Error codes */

#define MakeSerErr(svr,class,err) MakeErr(ER_DEVC,ER_SER,svr,ER_E_SSTM,class,err)

#define SER_ERR_NOMEM         MakeSerErr(ER_SEVERE,ER_C_STND,ER_NoMem)
#define SER_ERR_ABORTED       MakeSerErr(ER_SEVERE,ER_C_STND,ER_Aborted)
#define SER_ERR_BADPRIV       MakeSerErr(ER_SEVERE,ER_C_STND,ER_NotPrivileged)
#define SER_ERR_BADBAUDRATE   MakeSerErr(ER_SEVERE,ER_C_NSTND,1)
#define SER_ERR_BADHANDSHAKE  MakeSerErr(ER_SEVERE,ER_C_NSTND,2)
#define SER_ERR_BADWORDLENGTH MakeSerErr(ER_SEVERE,ER_C_NSTND,3)
#define SER_ERR_BADPARITY     MakeSerErr(ER_SEVERE,ER_C_NSTND,4)
#define SER_ERR_BADSTOPBITS   MakeSerErr(ER_SEVERE,ER_C_NSTND,5)
#define SER_ERR_BADEVENT      MakeSerErr(ER_SEVERE,ER_C_NSTND,6)


/*****************************************************************************/


#endif /* __DEVICE_SERIAL_H */
