/* @(#) syscall_glue.s 96/11/13 1.12 */

#include <hardware/PPCMacroequ.i>


/*****************************************************************************/


/* Calls the system call handler, the correct arguments are already in
 * the registers (r3 = folio/selector code, r4-r10 has the arguments)
 * We just do an sc and a return.
 */
	.macro
	SYSTEMCALL	&number

#ifdef BUILD_DEBUGGER
	mflr		r10
	stw		r10,4(r1)
#endif

	lis		r0,(((&number)>>16) & 0xFFFF)
	ori		r0,r0,((&number) & 0xFFFF)
	sc
	.endm

AUDIOFOLIO .equ 0x040000


/*****************************************************************************/


	DECFN	ReadAudioClock
	SYSTEMCALL	AUDIOFOLIO+42

	DECFN	ArmTrigger
	SYSTEMCALL	AUDIOFOLIO+41

	DECFN	GetAudioTime
	SYSTEMCALL	AUDIOFOLIO+40

/*	DECFN	GetAudioCyclesUsed  @@@ unused
**	SYSTEMCALL	AUDIOFOLIO+39
*/
	DECFN	GetAudioFrameCount
	SYSTEMCALL	AUDIOFOLIO+38

/*	DECFN	SendAttachment      @@@ unused
**	SYSTEMCALL	AUDIOFOLIO+37
*/
	DECFN	ReadProbePart
	SYSTEMCALL	AUDIOFOLIO+36

	DECFN	SetAudioFolioInfo
	SYSTEMCALL	AUDIOFOLIO+35

	DECFN	EnableAudioInput
	SYSTEMCALL	AUDIOFOLIO+34

	DECFN	AbortTimerCue
	SYSTEMCALL	AUDIOFOLIO+33

	DECFN	BendInstrumentPitch
	SYSTEMCALL	AUDIOFOLIO+32

/*	DECFN	IncrementGlobalIndex    @@@ unused
**	SYSTEMCALL	AUDIOFOLIO+31
*/
	DECFN	WhereAttachment
	SYSTEMCALL	AUDIOFOLIO+30

	DECFN	ResumeInstrument
	SYSTEMCALL	AUDIOFOLIO+29

	DECFN	PauseInstrument
	SYSTEMCALL	AUDIOFOLIO+28

	DECFN	SetAudioItemInfo
	SYSTEMCALL	AUDIOFOLIO+27

	DECFN	ScavengeInstrument
	SYSTEMCALL	AUDIOFOLIO+26

	DECFN	AdoptInstrument
	SYSTEMCALL	AUDIOFOLIO+25

	DECFN	AbandonInstrument
	SYSTEMCALL	AUDIOFOLIO+24

/*	DECFN	SetMasterTuning     @@@ unused
**	SYSTEMCALL	AUDIOFOLIO+23
*/
	DECFN	MonitorAttachment
	SYSTEMCALL	AUDIOFOLIO+22

	DECFN	LinkAttachments
	SYSTEMCALL	AUDIOFOLIO+21

	DECFN	StopAttachment
	SYSTEMCALL	AUDIOFOLIO+20

	DECFN	ReleaseAttachment
	SYSTEMCALL	AUDIOFOLIO+19

	DECFN	StartAttachment
	SYSTEMCALL	AUDIOFOLIO+18

/*	DECFN	SetRealKnobPart     @@@ unused
**	SYSTEMCALL	AUDIOFOLIO+17
*/
	DECFN	SetAudioClockDuration
	SYSTEMCALL	AUDIOFOLIO+16

	DECFN	SetAudioClockRate
	SYSTEMCALL	AUDIOFOLIO+15

/*	DECFN	RunAudioSignalTask  @@@ unused
**	SYSTEMCALL	AUDIOFOLIO+14
*/
	DECFN	SignalAtAudioTime
	SYSTEMCALL	AUDIOFOLIO+13

	DECFN	DisconnectInstrumentParts
	SYSTEMCALL	AUDIOFOLIO+12

/*	DECFN	FreeAmplitude       @@@ unused
**	SYSTEMCALL	AUDIOFOLIO+11
*/
	DECFN	ReadKnobPart
	SYSTEMCALL	AUDIOFOLIO+10

/*	DECFN	TraceAudio      @@@ unused
**	SYSTEMCALL	AUDIOFOLIO+9
*/
	DECFN	ConnectInstrumentsHack          /* private (!!! hack to workaround 4-arg SWI limit) */
	SYSTEMCALL	AUDIOFOLIO+8

/*	DECFN	TestHack        @@@ unused
**	SYSTEMCALL	AUDIOFOLIO+7
*/
	DECFN	GetDynamicLinkResourceUsage     /* private */
	SYSTEMCALL	AUDIOFOLIO+6

	DECFN	TuneInstrument
	SYSTEMCALL	AUDIOFOLIO+5

	DECFN	TuneInsTemplate
	SYSTEMCALL	AUDIOFOLIO+4

	DECFN	StopInstrument
	SYSTEMCALL	AUDIOFOLIO+3

	DECFN	ReleaseInstrument
	SYSTEMCALL	AUDIOFOLIO+2

	DECFN	StartInstrument
	SYSTEMCALL	AUDIOFOLIO+1

	DECFN	SetKnobPart
	SYSTEMCALL	AUDIOFOLIO+0
