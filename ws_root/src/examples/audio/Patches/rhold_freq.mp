
;******************************************************************************
;*
;*  @(#) rhold_freq.mp 96/01/08 1.2
;*
;*  Tutorial 9: Sample/hold frequency modulation.
;*
;******************************************************************************

    ; oscillator w/ modulation input
Instrument osc sawtooth.dsp
Knob Frequency 1 OscFreq 440.0          ; modulation bias
Knob Amplitude 1 Signed 0.8

    ; LFO
Instrument lfo randomhold.dsp
Knob ModRate 1 OscFreq 10.0             ; modulation rate
Knob ModDepth 1 OscFreq 200.0           ; modulation amount
Instrument mod timesplus.dsp

    ; output
Output Output 1 Signed

    ; connections
Connect . ModRate 0 lfo Frequency 0
Connect lfo Output 0 mod InputA 0
Connect . ModDepth 0 mod InputB 0
Connect . Frequency 0 mod InputC 0
Connect mod Output 0 osc Frequency 0   ; modulation destination
Connect osc Output 0 . Output 0
