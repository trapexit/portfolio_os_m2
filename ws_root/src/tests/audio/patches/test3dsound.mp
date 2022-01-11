; @(#) test3dsound.mp 96/08/22 1.3
;
; Test sound3d patch
; Something to put into the 3d instrument
;
; Author: rnm

; Declare the resources
Instrument sampler sampler_16_v1.dsp
Sample vox /remote/Samples/PitchedL/OrganHammond/HammondOrgan.C3LM44k.aiff
Expose InFIFO sampler InFIFO
Attach vox InFIFO

Knob SampleRate 1 SampRate 44100.0
Connect . SampleRate 0 sampler SampleRate 0

Knob Amplitude 1 Signed 0.8
Connect . Amplitude 0 sampler Amplitude 0

Output Output 1 Signed
Connect sampler Output 0 . Output 0
