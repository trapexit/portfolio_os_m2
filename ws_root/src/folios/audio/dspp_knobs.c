/* @(#) dspp_knobs.c 96/02/27 1.21 */
/* $Id: dspp_knobs.c,v 1.14 1995/02/24 20:57:39 peabody Exp phil $ */
/****************************************************************
**
** DSPP Instrument Knobs
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 931215 PLB Make Knob name matching case insensitive.
** 940406 PLB Add break after calculation for synthetic frequency.
**				Removed PRT
** 940817 PLB Shift Frequency by ShiftRate to compensate for execution rate.
** 950208 WJB Added usage of DRSC_TYPE_MASK.
** 950628 PLB Converted to M2 floating point knob style.
** 950724 WJB Added dsppClipKnobValue().
** 960104 PLB Change > to >= to include == Many , CR5497
** 960226 PLB Fixed knob clipping. Was using resource sub type, CR 5833
****************************************************************/

#include <dspptouch/dspp_touch.h>
#include <dspptouch/touch_hardware.h>

#include "audio_internal.h"

#define DBUG(x)   /* PRT(x) */

/******************************************************************
** dsppCreateKnobProbe - match name, error check, and fill in aknob
** Sets:
**    aknob->aknob_Type
**    aknob->aknob_DeviceInstrument
**    aknob->aknob_RsrcIndex
*/
int32 dsppCreateKnobProbe(AudioKnob *aknob, DSPPInstrument *dins, char *knobname, int32 legalType)
{
	DSPPResource *drsc;
	int32 Result = 0;
	int32 RsrcIndex;

DBUG(("dsppCreateKnobProbe:  name = %s\n", knobname));
	aknob->aknob_DeviceInstrument = NULL; /* until successful */

/* Source resource validation. */
	RsrcIndex = DSPPFindResourceIndex (dins->dins_Template, knobname );
	if (RsrcIndex < 0)
	{
DBUG(("dsppCreateKnobProbe: bad name.\n"));
		Result = RsrcIndex;
		goto error;
	}
	drsc = &dins->dins_Resources[RsrcIndex];
	if( drsc->drsc_Type != legalType )
	{
DBUG(("dsppCreateKnobProbe: bad type = %d\n", drsc->drsc_Type));
		Result = AF_ERR_NAME_NOT_FOUND;
		goto error;
	}

/* Set default type in aknob */
	aknob->aknob_Type = drsc->drsc_SubType;

	aknob->aknob_DeviceInstrument = dins;
	aknob->aknob_RsrcIndex = RsrcIndex;

error:
DBUG(("dsppCreateKnob:  returns = 0x%x\n", Result));
	return Result;
}

/*****************************************************************/
int32 dsppDeleteKnob( AudioKnob *aknob )
{
	if (aknob->aknob_DeviceInstrument == NULL) return AF_ERR_NOINSTRUMENT;
	aknob->aknob_DeviceInstrument = NULL;
	return 0;
}

/*****************************************************************/
DSPPResource *dsppKnobToResource(AudioKnob *aknob)
{
	DSPPResource *drsc;
	DSPPInstrument *dins;
	int32 rsi;

	dins = (DSPPInstrument *)aknob->aknob_DeviceInstrument;
	rsi = aknob->aknob_RsrcIndex;
	drsc = &(dins->dins_Resources[rsi]);
DBUG(("dsppKnobToResource: aknob = 0x%x, rsi = 0x%x, drsc = 0x%x\n",
	aknob, rsi, drsc ));
	return drsc;
}

/*****************************************************************/
int32 dsppSetKnob( DSPPResource *drsc, int32 PartNum, float32 val, int32 type)
{
	int32 intval, DataAddress;

/* Validate PartNum */
/* 960104 Change > to >= to include == Many , CR5497 */
	if( (PartNum < 0) || (PartNum >= drsc->drsc_Many) )
	{
DBUG(("dsppSetKnob: bad partnum = %d\n", PartNum));
		return AF_ERR_OUTOFRANGE;
	}

/* Convert FP to DSPP internal 1.15 representation. */
	intval = ConvertFP_SF15(val);
DBUG(("dsppSetKnob: intval = 0x%x, type = %d\n", intval, type ));

/* Clip to legal range. Use Knob Type to determine range. */
/* 960226 Fixed CR 5833.  Was using resource sub type. */
	intval = dsppClipRawValue (type, intval);
DBUG(("dsppSetKnob: intval clipped = 0x%x\n", intval ));
DBUG(("dsppSetKnob: allocated = 0x%x\n", drsc->drsc_Allocated ));

	DataAddress = drsc->drsc_Allocated + PartNum;  /* !!! Sleezy */
DBUG2(("dsppSetKnob: Set Data[0x%x] to $%x\n", DataAddress, intval));
	dsphWriteDataMem(DataAddress, intval);

	return 0;
}
