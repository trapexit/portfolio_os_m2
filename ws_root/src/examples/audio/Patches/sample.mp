
;******************************************************************************
;*
;*  @(#) sample.mp 96/08/22 1.3
;*
;*  Tutorial 5: Simple sample player patch
;*
;******************************************************************************

; This example just plays an aiff file through a sample instrument.  You
; can play with the sample playback rate and the amplitude with the knobs
; provided.

    ; sampler
Instrument player sampler_16_v1.dsp
Sample sample /remote/Samples/PitchedL/Trombone/Trombone.C3LM44k.aiff
Expose SampleFIFO player InFIFO
Attach sample SampleFIFO

    ; couple of control knobs
Knob SampleRate 1 SampRate 44100.0
Knob Amplitude 1 Signed 1.0

    ; output
Output Output 1 Signed

    ; connections
Connect player Output 0 . Output 0
Connect . SampleRate 0 player SampleRate 0
Connect . Amplitude 0 player Amplitude 0
