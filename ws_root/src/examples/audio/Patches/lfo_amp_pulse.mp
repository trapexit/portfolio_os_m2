
;******************************************************************************
;*
;*  @(#) lfo_amp_pulse.mp 96/01/08 1.2
;*
;*  Tutorial 3: square lfo modulating the amplitude of a square wave
;*
;******************************************************************************

; This demo shows another example of modulation; this time, a pulse wave
; lfo is used to modulate the amplitude of a square wave oscillator.  As
; with lfo_freq_tri, a timesplus.dsp object is used to scale and shift the
; modulating waveform.

; First, we define the instruments needed.
Instrument lfo pulse_lfo.dsp
Instrument osc square.dsp
Instrument mod timesplus.dsp

; Next, we preset values that don't get changed.
Constant lfo PulseWidth 0 -0.6   ; duty cycle

; Next, we define some knobs for values that can be tweaked.
; The values below mean that the modulation will go from max (0.5+0.5)
; to min (0.5-0.5).
Knob Frequency 1 OscFreq 136.0
Knob Amplitude 1 Signed 0.5     ; "base" amplitude
Knob ModDepth 1 Signed 0.5      ; modulation depth
Knob ModRate 1 LFOFreq 8.0      ; modulation rate
Output Output 1 Signed

; Finally, we define the connection matrix.
Connect   .    Frequency 0     osc  Frequency 0
Connect   .    Amplitude 0     mod  InputC    0
Connect   .    ModDepth  0     mod  InputB    0
Connect   .    ModRate   0     lfo  Frequency 0
Connect   lfo  Output    0     mod  InputA    0
Connect   mod  Output    0     osc  Amplitude 0   ; modulation destination
Connect   osc  Output    0     .    Output    0
