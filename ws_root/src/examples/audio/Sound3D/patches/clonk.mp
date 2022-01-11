; Copyright (C) 1996, an unpublished work by The 3DO Company.
; All rights reserved. This material contains confidential
; information that is the property of The 3DO Company. Any
; unauthorized duplication, disclosure or use is prohibited.
;
; @(#) clonk.mp 96/03/21 1.4
; Distributed with Portfolio V30.0
;
; Stationary clonk for bees
;
; Author: rnm

; Declare the resources
Instrument sampler sampler_16_v1.dsp
Sample vox Clonk.aiff
Expose InFIFO sampler InFIFO
Attach vox InFIFO

Knob SampleRate 1 SampRate 44100.0
Connect . SampleRate 0 sampler SampleRate 0

Knob Amplitude 1 Signed 1.0
Connect . Amplitude 0 sampler Amplitude 0

Output Output 1 Signed
Connect sampler Output 0 . Output 0
