
;******************************************************************************
;*
;*  @(#) rocket.mp 96/01/08 1.2
;*
;*  Tutorial 11: Rocket Sound
;*
;*  Rednoise modulating rednoise frequency and amplitude
;*
;******************************************************************************

Instrument red1 rednoise.dsp
Instrument red2 rednoise.dsp
Instrument red3 rednoise.dsp

Instrument mod1 timesplus.dsp
Instrument mod2 timesplus.dsp
Output Output 1 Signed

Connect red1 Output 0  mod1 InputA 0
Connect mod1 Output 0  red3 Frequency 0  ; modulation destination

Connect red2 Output 0  mod2 InputA 0
Connect mod2 Output 0  red3 Amplitude 0  ; amp modulation destination

Connect red3 Output 0  . Output 0        ; output

Constant red1 Frequency 0   540.0        ; freq modulation rate
Constant mod1 InputB    0     0.030      ; freq modulation depth
Constant mod1 InputC    0     0.037      ; freq modulation bias

Constant red2 Frequency 0   200.0        ; amp modulation rate
Constant mod2 InputB    0     0.366      ; amp modulation depth
Constant mod2 InputC    0     0.560      ; amp modulation bias
