#ifndef __DSPP_IMEM_H
#define __DSPP_IMEM_H


/******************************************************************************
**
**  @(#) dspp_imem.h 95/06/12 1.3
**
**  DSPP I-Mem handshaking (for Opera, just stubs for M2)
**
**  By: Phil Burk and Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950508 WJB  Gave a home to this code thrown out of dspptouch library.
**  950509 WJB  Adapted to changed M2 dsphReadDataMem() API.
**  950612 WJB  Retired M2-compatible versions of dsphRead/WriteIMem().
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include "audio_folio_modes.h"


#ifdef AF_ASIC_OPERA    /* { */

#include <dspptouch/dspp_touch.h>   /* dsphRead/WriteDataMem() */


/* -------------------- Opera I-Mem handshaking */

#define DSPP_WRITE_KNOB_NAME "IMemWriteAddr"
#define DSPP_READ_KNOB_NAME  "IMemReadAddr"

int32  DSPP_InitIMemAccess( void );
int32  dsphReadIMem( int32 ReadAddr, int32 *ValuePtr );
int32  dsphWriteIMem( int32 WriteAddr, int32 WriteValue );


#endif  /* } */


/*****************************************************************************/

#endif  /* __DSPP_IMEM_H */
