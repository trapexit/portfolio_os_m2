
;******************************************************************************
;*
;*  @(#) tri_amp_samp.mp 96/08/22 1.3
;*
;*  Tutorial 4: Apply modulator to a sample. Modulate amplitude using triangle.
;*
;******************************************************************************

; This time, we're modulating a sample instrument rather than an
; oscillator.  As with tutorial 2, we use a triangle LFO, but this
; time connected to the amplitude control of the sample.

; Instruments
Instrument osc sampler_16_v2.dsp
Sample aSample /remote/Samples/PitchedLR/RecorderRL/Recorder.C4LRM44k.aiff
Expose oscFIFO osc InFIFO
Attach aSample oscFIFO

Instrument lfo triangle_lfo.dsp
Instrument mod timesplus.dsp
Output Output 2 Signed

Knob SampleRate 1 SampRate 44100.0  ; playback rate for sample
Knob Amplitude 1 Signed 0.8  ; "base" amplitude
Knob ModRate 1 LFOFreq 6.0   ; modulation rate
Knob ModDepth 1 Signed 0.15   ; modulation amount

Connect lfo    Output           0     mod    InputA 0
Connect mod    Output           0     osc    Amplitude 0   ; modulation destination
Connect osc    Output           0     .      Output 0      ; left channel
Connect osc    Output           1     .      Output 1      ; right channel

Connect .      ModRate          0     lfo    Frequency 0
Connect .      ModDepth         0     mod    InputB 0
Connect .      Amplitude        0     mod    InputC 0
Connect .      SampleRate       0     osc    SampleRate 0
