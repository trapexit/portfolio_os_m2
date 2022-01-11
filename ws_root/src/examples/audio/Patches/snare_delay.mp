;******************************************************************************
;*
;*  @(#) snare_delay.mp 96/08/22 1.3
;*
;*  Tutorial 13: Shows usage of delay line commands and delay_f1.dsp.
;*
;******************************************************************************

; --------------------- snare

Output Output 1 Signed

Instrument  sampler sampler_16_f1.dsp

Expose InFIFO sampler InFIFO

Connect   sampler Output 0 Mixer Input 0

Sample    snare /remote/Samples/Unpitched/DrumSnare/SnareOpen.M44k.aiff
Attach    snare InFIFO


; --------------------- 1-tap delay

Instrument  Send      delay_f1.dsp
Instrument  Tap       sampler_16_f1.dsp
Mixer       Mixer     2 2

Knob   InputGain    1 Signed +0.5
Knob   FeedbackGain 1 Signed -0.5
Knob   BypassGain   1 Signed +0.5
Knob   DelayGain    1 Signed -0.5

Expose SendFIFO Send OutFIFO
Expose TapFIFO  Tap  InFIFO

Connect Tap Output 0 Mixer Input 1
Connect Mixer Output 0 . Output 0
Connect Mixer Output 1 Send Input 0
Connect . BypassGain   0 Mixer Gain 0
Connect . DelayGain    0 Mixer Gain 1
Connect . InputGain    0 Mixer Gain 2
Connect . FeedbackGain 0 Mixer Gain 3

DelayLine Delay 44100 1
Attach    Delay SendFIFO

; Attachment offset is set so that the player attachment is
; just a little ahead of the writer (nearly as far away from the
; writer as possible).  This yields the maximum delay time
; for a given delay line.

; NEW FOR 1.2
; The instrument "sampler_drift_v1.dsp" allows you to effectively
; adjust the delay time by moving the tap.  Look at the example
; "flange.mp" to see how to do this.

Attach    Delay TapFIFO -startat 50

