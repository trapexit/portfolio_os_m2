;******************************************************************************
;*
;*  Copyright (c) 1993-1996, an unpublished work by The 3DO Company.
;*  All rights reserved. This material contains confidential
;*  information that is the property of The 3DO Company. Any
;*  unauthorized duplication, disclosure or use is prohibited.
;*  
;*  @(#) reverb.mp 96/07/17 1.1
;*  Distributed with Portfolio V32.0
;*
;******************************************************************************

; Multi tap reverberator
; Uses single mixer for taps.

; Instruments
Input Input 1 Signed

Instrument DelOut1 sampler_raw_f1.dsp
Instrument DelOut2 sampler_raw_f1.dsp
Instrument DelOut3 sampler_raw_f1.dsp
Instrument DelOut4 sampler_raw_f1.dsp
Instrument DelOut5 sampler_raw_f1.dsp
Instrument DelOut6 sampler_raw_f1.dsp

Instrument LowPass filter_1o1z.dsp

Instrument DelIn delay_f1.dsp

; We need a maximum delay of 1s = (1 * 44100 + 50) * 2  = 89200 bytes

Delayline Delay 89200 1
Mixer MixIn 2 1
Mixer MixOut 6 1
Output Output 1 Signed

; Hook up the delay line to the in and out
Expose dOutFIFO1 DelOut1 InFIFO
Expose dOutFIFO2 DelOut2 InFIFO
Expose dOutFIFO3 DelOut3 InFIFO
Expose dOutFIFO4 DelOut4 InFIFO
Expose dOutFIFO5 DelOut5 InFIFO
Expose dOutFIFO6 DelOut6 InFIFO
Attach Delay dOutFIFO1 -startat 42000
Attach Delay dOutFIFO2 -startat 35234
Attach Delay dOutFIFO3 -startat 25000
Attach Delay dOutFIFO4 -startat 17038
Attach Delay dOutFIFO5 -startat 12350
Attach Delay dOutFIFO6 -startat  3258

Expose dInFIFO DelIn OutFIFO
Attach Delay dInFIFO

; Hook up the rest of the audio path
Connect	.	    Input   0   MixIn   Input   0
Connect MixOut	Output	0	LowPass	Input	0
Connect LowPass	Output	0	MixIn	Input	1

Connect MixIn	Output	0	DelIn	Input	0

Connect DelOut1	Output	0	MixOut	Input	0
Connect DelOut2	Output	0	MixOut	Input	1
Connect DelOut3	Output	0	MixOut	Input	2
Connect DelOut4	Output	0	MixOut	Input	3
Connect DelOut5	Output	0	MixOut	Input	4
Connect DelOut6	Output	0	MixOut	Input	5

Connect MixOut	Output	0	.	Output	0

; Define knobs for tweaking mix
Knob InGain	1 	Signed 	1.0	; input->delay
Knob Feedback	1 	Signed 	-0.95
Knob Tap1	1	Signed	0.30
Knob Tap2	1 	Signed  0.20
Knob Tap3	1	Signed	0.15
Knob Tap4	1	Signed	0.15
Knob Tap5	1	Signed	0.10
Knob Tap6	1	Signed	0.05

; Connect the knobs up
Connect .	InGain	 0	MixIn	Gain	0
Connect	.	Feedback 0	MixIn	Gain	1

Connect	.	Tap1	0	MixOut	Gain	0
Connect	.	Tap2	0	MixOut	Gain	1
Connect	.	Tap3	0	MixOut	Gain	2
Connect	.	Tap4	0	MixOut	Gain	3
Connect	.	Tap5	0	MixOut	Gain	4
Connect	.	Tap6	0	MixOut	Gain	5

