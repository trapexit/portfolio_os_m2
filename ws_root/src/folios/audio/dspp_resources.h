#ifndef __DSPP_RESOURCES_H
#define __DSPP_RESOURCES_H


/******************************************************************************
**
**  @(#) dspp_resources.h 96/06/19 1.15
**  $Id: dspp_resources.h,v 1.25 1995/03/18 02:19:34 peabody Exp phil $
**
**  DSPP resource manager
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950206 WJB  Created.
**  950207 WJB  Revised function names.
**  950207 WJB  Changed some functions to return Err.
**              Made some args const.
**  950208 WJB  Moved available resource definitions from dspp_touch.h.
**  950209 WJB  Added DSPR_ prefix to available resource definitions.
**  950209 WJB  Cleaned up formatting a bit.
**  950209 WJB  Renamed dsppAlloc/FreeResource() to dsppOpen/CloseResource().
**  950216 WJB  Added dsppOpen/CloseInstrumentResources().
**              Hid dsppOpen/CloseResource().
**  950217 WJB  Added dsppExport/UnexportInstrumentResources().
**  950217 WJB  Made dsppCloseInstrumentResources() return void.
**  950222 PLB  Added DSPR_FRAMES_PER_BATCH
**  950301 WJB  Moved Opera DSPR_ defines into dspp_resources_anvil.c.
**  950303 WJB  Added dsppIsValidResourceType().
**  950303 WJB  Privatized resource limit definitions.
**  950306 WJB  Added RBASE8 resource information.
**  950308 WJB  Added dsppAdjustAvailTicks() and DSPR_NUM_TICK_FRAMES.
**  950308 WJB  Removed DSPR_NUM_TICK_FRAMES.
**  950310 WJB  Publicized dsppImport/UnimportResource().
**  950317 WJB  Added dsppGrabSystemReservedResources() proto.
**  950417 WJB  Added dsppValidateTemplateResources().
**              Removed dsppIsValidResourceType().
**  950508 WJB  Added DSPR_NUM_TRIGGERS.
**  950614 WJB  Moved dsppAlloc/FreeResource() protos from dspp_resources_internal.h.
**              Added dsppAllocResourceHere().
**  950708 WJB  Moved dsppAlloc/FreeTicks() protos from dspp_resources_internal.h
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <kernel/types.h>

#include "audio_folio_modes.h"
#include "dspp.h"               /* DSPP structures */


/* -------------------- Assorted public resource definitions */

    /* Frames / Batch */
#define DSPR_FRAMES_PER_BATCH     (8)

    /* Triggers */
#define DSPR_NUM_TRIGGERS         (16)
#define DSPR_MAX_TRIGGERS         (16)

/* -------------------- RBASE8 resource definitions */
/* Note: Alignment rules are asic-dependent and come from dspp_instructions.h */

#define DSPR_RBASE8_NUM_REGS    8   /* # of registers (words) per allocation */
#define DSPR_RBASE8_FIRST_REG   4   /* first register for which allocation is a success */


/* -------------------- Functions */

    /* init */
void dsppInitResources (void);
Err dsppAdjustAvailTicks (float32 framerate);

    /* validation */
Err dsppValidateTemplateResources (const DSPPTemplate *);

    /* open/close */
Err dsppOpenInstrumentResources (DSPPInstrument *);
void dsppCloseInstrumentResources (DSPPInstrument *);

    /* export */
Err dsppExportInstrumentResources (DSPPInstrument *);
Err dsppUnexportInstrumentResources (DSPPInstrument *);
int32 dsppSumResourceReferences (const DSPPInstrument *);

    /* import */
int32 dsppImportResource (const char *name);
void dsppUnimportResource (const char *name);

    /* query */
int32 dsppGetResourceAttribute (const DSPPResource *, int32 Attribute);

    /* mode-specific low-level resource alloc/free */
int32 dsppAllocResource (int32 rsrctype, int32 rsrcamount);
int32 dsppAllocResourceHere (int32 rsrctype, int32 rsrcalloc, int32 rsrcamount);
void dsppFreeResource (int32 rsrctype, int32 rsrcalloc, int32 rsrcamount);

    /* mode-specific low-level tick alloc/free */
void dsppFreeTicks( int32 RateShift, int32 rsrcamount );
int32 dsppAllocTicks( int32 RateShift, int32 rsrcamount );

    /* consumption / availability queries */
int32 dsppGetTemplateResourceUsage (const DSPPTemplate *, int32 rsrcType);

    /* port queries */
void dsppGetInstrumentPortInfo (InstrumentPortInfo *resultInfo, uint32 resultInfoSize, const DSPPTemplate *, int32 rsrcIndex);
void dsppGetInstrumentEnvPortInfo (InstrumentPortInfo *resultInfo, uint32 resultInfoSize, const DSPPTemplate *, const DSPPEnvHookRsrcInfo *);
int32 dsppGetRsrcPortType (uint8 rsrcType);
#define dsppIsPortRsrc(drsc) (dsppGetRsrcPortType((drsc)->drsc_Type) >= 0)
bool dsppIsEnvPortRsrc (DSPPEnvHookRsrcInfo *resultderi, const DSPPTemplate *, int32 rsrcIndex);


/*****************************************************************************/

#endif /* __DSPP_RESOURCES_H */
