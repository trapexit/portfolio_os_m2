#ifndef __AUDIO_AUDIO_OBSOLETE_H
#define __AUDIO_AUDIO_OBSOLETE_H

/****************************************************************************
**
**  @(#) audio_obsolete.h 95/09/21 1.3
**
**  Obsolete Audio Folio Includes
**  Only use this file while transitioning from the old Opera API
**  to the new M2 API.
**
****************************************************************************/

#define AllocAmplitude(a) (0)
#define FreeAmplitude(a) /* noop */

#define AllocInstrument(InsTemplate,priority) CreateInstrumentVA((InsTemplate), AF_TAG_PRIORITY, (priority), TAG_END)
#define FreeInstrument(Instrument) DeleteInstrument(Instrument)
#define AttachEnvelope(instrument,envelope,envHookName) CreateAttachmentVA((instrument), (envelope), AF_TAG_NAME, (envHookName), TAG_END)
#define AttachSample(instrument,sample,FIFOName) CreateAttachmentVA((instrument), (sample), AF_TAG_NAME, (FIFOName), TAG_END)
#define DetachSample(att) DeleteAttachment(att)
#define DetachEnvelope(att) DeleteAttachment(att)

#define GrabKnob( Ins, Name ) CreateKnob( Ins, Name, NULL )
#define ReleaseKnob( Knob ) DeleteKnob( Knob )
#define TweakKnob( KnobItem, Value ) SetKnob( KnobItem, ConvertF16_FP(Value) )
#define TweakRawKnob( KnobItem, Value ) SetKnob( KnobItem, ConvertF16_FP((Value<<1)) )

#define DisconnectInstruments( SrcIns, SrcName, DstIns, DstName) \
	DisconnectInstrumentParts( DstIns, DstName, 0)

#endif /* __AUDIO_AUDIO_OBSOLETE_H */
