/******************************************************************************
**
**  @(#) audio_probe.c 96/06/20 1.23
**  $Id: audio_probe.c,v 1.14 1995/03/16 20:12:02 peabody Exp phil $
**
**  Audio Probes allow you to read outputs from DSP instruments
**
**  By: Phil Burk
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  940923 PLB  Created.
**  940927 WJB  Cleaned up a bit (reduced code by 24 bytes).
**  940927 WJB  Improved tag trap in CreateProbe().
**  950207 WJB  Added DRSC_TYPE_MASK to drsc_Type comparisons.
**  950509 WJB  Now calling dsphReadDataMem() directory for M2.
**
**  Initials:
**
**  PLB: Phil Burk (phil)
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <dspptouch/dspp_touch.h>   /* dsph...() */

#include "audio_folio_modes.h"
#include "audio_internal.h"

/* Macros for debugging. */
#define DBUG(x)   /* PRT(x) */

/******************************************************************/
/***** USER MODE **************************************************/
/******************************************************************/

 /**
 |||	AUTODOC -public -class Items -group Audio -name Probe
 |||	An item to permit the CPU to read the output of a DSP Instrument(@).
 |||
 |||	  Description
 |||
 |||	    A probe is an item that allows the CPU to read the output of a DSP
 |||	    instrument. It is the inverse of a Knob(@). Multiple probes can be connected
 |||	    to a single output. A probe does not interfere with a connection between
 |||	    instruments.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_PROBE_NODE
 |||
 |||	  Create
 |||
 |||	    CreateProbe(), CreateItem()
 |||
 |||	  Delete
 |||
 |||	    DeleteProbe(), DeleteItem()
 |||
 |||	  Use
 |||
 |||	    ReadProbePart()
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_TYPE (uint8) - Create
 |||	        Determines the signal type of the probe. Must be one of the
 |||	        AF_SIGNAL_TYPE_* defined in <audio/audio.h>. Defaults to the signal type
 |||	        of the output (for standard DSP instruments, this is described in the
 |||	        instrument documentation).
 |||
 |||	  See Also
 |||
 |||	    Instrument(@), Knob(@)
 **/

 /**
 |||	AUTODOC -public -class audio -group Probe -name CreateProbe
 |||	Creates a Probe(@) for an output of an Instrument(@).
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateProbe (Item instrument, const char *portName, const TagArg *tagList)
 |||
 |||	    Item CreateProbeVA (Item instrument, const char *portName, uint32 tag1, ... )
 |||
 |||	  Description
 |||
 |||	    This procedure creates a Probe item that provides a fast connection between
 |||	    a task and one of an instrument's outputs. You can then call ReadProbePart()
 |||	    to poll the value of the probed output. Use DeleteProbe() to delete the
 |||	    Probe when you are finished with it.
 |||
 |||	  Arguments
 |||
 |||	    instrument
 |||	        The item number of the instrument.
 |||
 |||	    portName
 |||	        The name of the Output.
 |||
 |||	  Tag Arguments
 |||
 |||	    AF_TAG_TYPE (uint8) - Create, Query, Modify
 |||	        Determines the signal type of the probe. Must be one of the
 |||	        AF_SIGNAL_TYPE_* defined in <audio/audio.h>. Defaults to the signal type
 |||	        of the output (for standard DSP instruments, this is described in the
 |||	        instrument documentation).
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the probe if successful or an
 |||	    error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Convenience call implemented in libc.a V29.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  Notes
 |||
 |||	    This function is equivalent to:
 |||
 |||	  -preformatted
 |||
 |||	        CreateItemVA (MKNODEID(AUDIONODE,AUDIO_PROBE_NODE),
 |||	                      AF_TAG_INSTRUMENT, instrument,
 |||	                      AF_TAG_NAME,       portName,
 |||	                      TAG_JUMP,          tagList);
 |||
 |||	  See Also
 |||
 |||	    ReadProbePart(), ReadProbe(), DeleteProbe(), Probe(@)
 **/

 /**
 |||	AUTODOC -public -class audio -group Probe -name DeleteProbe
 |||	Deletes a Probe(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteProbe (Item probe)
 |||
 |||	  Description
 |||
 |||	    This procedure deletes a Probe. All Probes to an Instrument are
 |||	    automatically deleted when that Instrument is deleted.
 |||
 |||	  Arguments
 |||
 |||	    probe
 |||	        Item number of probe to delete.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro in <audio/audio.h>
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    CreateProbe(), Probe(@)
 **/

/******************************************************************/
/******** Folio Creation of Probes *********************************/
/******************************************************************/

Item internalCreateAudioProbe (AudioProbe *aprob, /* !!! const */ TagArg *args)
{
	return internalCreateAudioKnobOrProbe( (AudioKnob *) aprob, DRSC_TYPE_OUTPUT, args );
}

/******************************************************************/
int32 internalDeleteAudioProbe (AudioProbe *aprob)
{

TRACEE(TRACE_INT,TRACE_ITEM,("internalDeleteAudioProbe(0x%lx)\n", aprob));

/* Remove from Instrument's List */
/* (use RemoveAndMarkNode() because this list can be cleared with afi_DeleteLinkedItems()) */
	RemoveAndMarkNode( (Node *) aprob );

	return (0);
}

/******************************************************************/
/***** SUPERVISOR MODE ********************************************/
/******************************************************************/

 /**
 |||	AUTODOC -public -class audio -group Probe -name ReadProbe
 |||	Reads the value of a single-part Probe(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err ReadProbe (Item probe, float *valuePtr)
 |||
 |||	  Description
 |||
 |||	    Reads the instantaneous value of a single-part probed instrument output.
 |||
 |||	  Arguments
 |||
 |||	    probe
 |||	        Item number of the probe to be read. Always reads part 0.
 |||
 |||	    valuePtr
 |||	        Pointer to where to put result. Task must have write permission for that
 |||	        address. The result is in the units that apply to the AF_SIGNAL_TYPE_ of
 |||	        the Probe.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Convenience macro implemented in <audio/audio.h> V27.
 |||
 |||	  Notes
 |||
 |||	    This macro is equivalent to:
 |||
 |||	  -preformatted
 |||
 |||	        ReadProbePart (probe, 0, valuePtr)
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    CreateProbe(), ReadProbePart()
 **/
 /**
 |||	AUTODOC -public -class audio -group Probe -name ReadProbePart
 |||	Reads the value of a Probe(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err ReadProbePart (Item probe, int32 partNum, float *valuePtr)
 |||
 |||	  Description
 |||
 |||	    Reads the instantaneous value of a probed instrument output.
 |||
 |||	  Arguments
 |||
 |||	    probe
 |||	        Item number of the probe to be read.
 |||
 |||	    partNum
 |||	        Index of the part you wish to read.
 |||
 |||	    valuePtr
 |||	        Pointer to where to put result. Task must have write permission for that
 |||	        address. The result is in the units that apply to the AF_SIGNAL_TYPE_ of
 |||	        the Probe.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V27.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    CreateProbe(), ReadProbe()
 **/
Err swiReadProbePart( Item probeItem, int32 partNum, float *valuePtr )
{
	AudioProbe *aprob;
DBUG(("ReadProbePart(0x%x, %d, 0x%x)\n", probeItem, PartNum, valuePtr));

	aprob = (AudioProbe *)CheckItem(probeItem, AUDIONODE, AUDIO_PROBE_NODE);
	if (aprob == NULL) return AF_ERR_BADITEM;
	return internalReadKnobOrProbePart( (AudioKnob *) aprob, partNum, valuePtr );
}
