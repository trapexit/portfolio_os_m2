! @(#) linkexec.x 96/08/05 1.24
!
! --------------------- Exports
!
MAGIC 10
! EXPORTS 0=@@@ unused
! EXPORTS 1=@@@ unused - was CreateProbe
! EXPORTS 2=@@@ unused - was CreatePatchTemplate
! EXPORTS 3=@@@ unused - was CreateInstrument
EXPORTS 4=GetAudioFolioInfo
! EXPORTS 5=@@@ unused
! EXPORTS 6=@@@ unused - was CreateSample
EXPORTS 7=Convert12TET_FP
EXPORTS 8=CreateMixerTemplate
! EXPORTS 9=@@@ unused - was CreateDelayLine
! EXPORTS 10=@@@ unused
! EXPORTS 11=@@@ unused - was CreateAttachment
! EXPORTS 12=@@@ unused - was DeleteEnvelope
! EXPORTS 13=@@@ unused
EXPORTS 14=GetNumInstrumentPorts
EXPORTS 15=GetInstrumentPortInfoByIndex
EXPORTS 16=GetInstrumentPortInfoByName
EXPORTS 17=UnloadInstrument
! EXPORTS 18=@@@ unused - was UnloadInsTemplate
EXPORTS 19=GetAudioResourceInfo
EXPORTS 20=GetInstrumentResourceInfo
EXPORTS 21=ConvertAudioSignalToGeneric
EXPORTS 22=ConvertGenericToAudioSignal
EXPORTS 23=GetCueSignal
EXPORTS 24=SleepUntilAudioTime
EXPORTS 25=GetAudioClockDuration
EXPORTS 26=GetAudioClockRate
EXPORTS 27=LoadInstrument
EXPORTS 28=GetAudioItemInfo
! EXPORTS 29=@@@ unused - was GetNumKnobs
EXPORTS 30=DebugSample
! EXPORTS 31=@@@ unused - was CreateKnob
EXPORTS 32=LoadInsTemplate
! EXPORTS 33=@@@ unused - was NextPatchCmd
EXPORTS 34=ArmTrigger
EXPORTS 35=GetAudioTime
! EXPORTS 36=@@@ unused - was GetAudioCyclesUsed
EXPORTS 37=GetAudioFrameCount
EXPORTS 38=ReadProbePart
EXPORTS 39=SetAudioFolioInfo
EXPORTS 40=EnableAudioInput
EXPORTS 41=AbortTimerCue
EXPORTS 42=BendInstrumentPitch
EXPORTS 43=WhereAttachment
EXPORTS 44=ResumeInstrument
EXPORTS 45=PauseInstrument
EXPORTS 46=SetAudioItemInfo
EXPORTS 47=ScavengeInstrument
EXPORTS 48=AdoptInstrument
EXPORTS 49=AbandonInstrument
EXPORTS 50=MonitorAttachment
EXPORTS 51=LinkAttachments
EXPORTS 52=StopAttachment
EXPORTS 53=ReleaseAttachment
EXPORTS 54=StartAttachment
EXPORTS 55=SetAudioClockDuration
EXPORTS 56=SetAudioClockRate
EXPORTS 57=ConnectInstrumentParts
EXPORTS 58=SignalAtAudioTime
EXPORTS 59=DisconnectInstrumentParts
EXPORTS 60=ReadKnobPart
! EXPORTS 61=@@@ unused - was TraceAudio
! EXPORTS 62=@@@ unused - was ConnectInstrumentsHack
EXPORTS 63=GetAttachments
EXPORTS 64=GetAudioSignalInfo
EXPORTS 65=TuneInstrument
EXPORTS 66=TuneInsTemplate
EXPORTS 67=StopInstrument
EXPORTS 68=ReleaseInstrument
EXPORTS 69=StartInstrument
EXPORTS 70=SetKnobPart
EXPORTS 71=ReadAudioClock
!
! private entry points for AudioPatch Folio
EXPORTS 72=dsppCreateUserTemplate
EXPORTS 73=dsppDeleteUserTemplate
EXPORTS 74=dsppLookupTemplate
EXPORTS 75=dsppGetTemplateRsrcName
EXPORTS 76=dsppFindResourceIndex
EXPORTS 77=dsppFindEnvHookResources
EXPORTS 78=dsppFixupCodeImage
EXPORTS 79=dsppRelocate
EXPORTS 80=dsppClipRawValue
!
! --------------------- Imports
!
! IMPORT_ON_DEMAND iff
! REIMPORT_ALLOWED iff
