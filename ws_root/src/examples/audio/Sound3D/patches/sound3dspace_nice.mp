; @(#) sound3dspace_nice.mp 96/02/08 1.1
;
; 3DSpace output stage
;
; Takes input from the various 3D sources, mixes them to left and right
; along with a reverb side chain.
;
; Author: rnm
; Created: 1/26/96

Input Input 2 Signed		; this is one source L/R pair
Output Output 2 Signed

; -----------------------
; Comb filter bank
; -----------------------

; Instruments
Mixer MonoMix 2 1

Instrument DelOut1 sampler_raw_f1.dsp
Instrument DelOut2 sampler_raw_f1.dsp
Instrument DelOut3 sampler_raw_f1.dsp
Instrument DelOut4 sampler_raw_f1.dsp
Instrument DelOut5 sampler_raw_f1.dsp
Instrument DelOut6 sampler_raw_f1.dsp
Instrument DelIn1 delay_f1.dsp
Instrument DelIn2 delay_f1.dsp
Instrument DelIn3 delay_f1.dsp
Instrument DelIn4 delay_f1.dsp
Instrument DelIn5 delay_f1.dsp
Instrument DelIn6 delay_f1.dsp

Instrument lp1 filter_1o1z.dsp
Instrument lp2 filter_1o1z.dsp
Instrument lp3 filter_1o1z.dsp
Instrument lp4 filter_1o1z.dsp
Instrument lp5 filter_1o1z.dsp
Instrument lp6 filter_1o1z.dsp

; Delay line length = time * 44100 * 2 + 50 sample guard
; 50ms = 4510
; 56ms = 5039
; 61ms = 5480
; 68ms = 6098
; 72ms = 6450
; 78ms = 6980

Delayline Delay1 4510 1
Delayline Delay2 5040 1	; must be even number of bytes
Delayline Delay3 5480 1
Delayline Delay4 6098 1
Delayline Delay5 6450 1
Delayline Delay6 6980 1
Mixer CombIn1 2 1
Mixer CombIn2 2 1
Mixer CombIn3 2 1
Mixer CombIn4 2 1
Mixer CombIn5 2 1
Mixer CombIn6 2 1
Mixer CombOut 6 1

; Hook up the delay lines to the ins and outs
Expose dOutFIFO1 DelOut1 InFIFO
Expose dOutFIFO2 DelOut2 InFIFO
Expose dOutFIFO3 DelOut3 InFIFO
Expose dOutFIFO4 DelOut4 InFIFO
Expose dOutFIFO5 DelOut5 InFIFO
Expose dOutFIFO6 DelOut6 InFIFO
Attach Delay1 dOutFIFO1 -startat 50
Attach Delay2 dOutFIFO2 -startat 50
Attach Delay3 dOutFIFO3 -startat 50
Attach Delay4 dOutFIFO4 -startat 50
Attach Delay5 dOutFIFO5 -startat 50
Attach Delay6 dOutFIFO6 -startat 50

Expose dInFIFO1 DelIn1 OutFIFO
Expose dInFIFO2 DelIn2 OutFIFO
Expose dInFIFO3 DelIn3 OutFIFO
Expose dInFIFO4 DelIn4 OutFIFO
Expose dInFIFO5 DelIn5 OutFIFO
Expose dInFIFO6 DelIn6 OutFIFO
Attach Delay1 dInFIFO1
Attach Delay2 dInFIFO2
Attach Delay3 dInFIFO3
Attach Delay4 dInFIFO4
Attach Delay5 dInFIFO5
Attach Delay6 dInFIFO6

; Hook up the rest of the audio path

Connect	.	Input	0	MonoMix	Input	0
Connect	.	Input	1	MonoMix	Input	1

Constant			MonoMix	Gain	0	0.5
Constant			MonoMix	Gain	1	0.5

Connect	MonoMix	Output	0	CombIn1	Input	0
Connect	MonoMix	Output	0	CombIn2	Input	0
Connect	MonoMix	Output	0	CombIn3	Input	0
Connect	MonoMix	Output	0	CombIn4	Input	0
Connect	MonoMix	Output	0	CombIn5	Input	0
Connect	MonoMix	Output	0	CombIn6	Input	0

Connect	lp1	Output	0	CombIn1	Input	1
Connect	lp2	Output	0	CombIn2	Input	1
Connect	lp3	Output	0	CombIn3	Input	1
Connect	lp4	Output	0	CombIn4	Input	1
Connect	lp5	Output	0	CombIn5	Input	1
Connect	lp6	Output	0	CombIn6	Input	1

Connect	CombIn1	Output	0	DelIn1	Input	0
Connect	CombIn2	Output	0	DelIn2	Input	0
Connect	CombIn3	Output	0	DelIn3	Input	0
Connect	CombIn4	Output	0	DelIn4	Input	0
Connect	CombIn5	Output	0	DelIn5	Input	0
Connect	CombIn6	Output	0	DelIn6	Input	0

Connect DelOut1	Output	0	lp1	Input	0
Connect DelOut2	Output	0	lp2	Input	0
Connect DelOut3	Output	0	lp3	Input	0
Connect DelOut4	Output	0	lp4	Input	0
Connect DelOut5	Output	0	lp5	Input	0
Connect DelOut6	Output	0	lp6	Input	0

Connect	DelOut1	Output	0	CombOut	Input	0
Connect	DelOut2	Output	0	CombOut	Input	1
Connect DelOut3	Output	0	CombOut	Input	2
Connect	DelOut4	Output	0	CombOut	Input	3
Connect	DelOut5	Output	0	CombOut	Input	4
Connect DelOut6	Output	0	CombOut	Input	5

; Preset some of the mixer gains.

; Define knobs for tweaking mix
Knob g		1	Signed	-0.83	; loop gain
Knob lpA0	1	Signed	0.5	; tune to sample for best results!
Knob lpA1	1	Signed	0.5	; minimal Q

; Connect the knobs up
Constant 			CombIn1	Gain	0	0.5	; input->delay line always half
Constant 			CombIn2	Gain	0	0.5
Constant 			CombIn3	Gain	0	0.5
Constant 			CombIn4	Gain	0	0.5
Constant 			CombIn5	Gain	0	0.5
Constant 			CombIn6	Gain	0	0.5

Connect	.	g	0	CombIn1	Gain	1
Connect	.	g	0	CombIn2	Gain	1
Connect	.	g	0	CombIn3	Gain	1
Connect	.	g	0	CombIn4	Gain	1
Connect	.	g	0	CombIn5	Gain	1
Connect	.	g	0	CombIn6	Gain	1

Connect	.	lpA0	0	lp1	A0	0
Connect	.	lpA0	0	lp2	A0	0
Connect	.	lpA0	0	lp3	A0	0
Connect	.	lpA0	0	lp4	A0	0
Connect	.	lpA0	0	lp5	A0	0
Connect	.	lpA0	0	lp6	A0	0

Connect	.	lpA1	0	lp1	A1	0
Connect	.	lpA1	0	lp2	A1	0
Connect	.	lpA1	0	lp3	A1	0
Connect	.	lpA1	0	lp4	A1	0
Connect	.	lpA1	0	lp5	A1	0
Connect	.	lpA1	0	lp6	A1	0

Constant			CombOut	Gain	0	0.16
Constant			CombOut	Gain	1	0.16
Constant			CombOut	Gain	2	0.16
Constant			CombOut	Gain	3	0.16
Constant			CombOut	Gain	4	0.16
Constant			CombOut	Gain	5	0.16

; -----------------------
; Allpass reverb filter on the mixed output of the comb filter stage
; -----------------------

; Signal-path Instruments
Instrument dOutAllp sampler_raw_f1.dsp
Instrument dInAllp delay_f1.dsp
Delayline dlAllp 580 1		; about 6ms
Mixer AllpIn 2 1
Mixer AllpOut 2 1

; Hook up the delay lines to the ins and outs
Expose dOutFIFOa dOutAllp InFIFO
Attach dlAllp dOutFIFOa -startat 50
Expose dInFIFOa dInAllp OutFIFO
Attach dlAllp dInFIFOa

; Hook up the signal path
Connect	CombOut		Output	0	AllpIn	Input	0
Connect	CombOut		Output	0	AllpOut	Input	0

Connect	AllpIn		Output	0	dInAllp	Input	0
Connect	dOutAllp	Output	0	AllpOut	Input	1
Connect	dOutAllp	Output	0	AllpIn	Input	1

; Knob to tweak the gain
Knob allp_g 1 Signed 0.35	; scaled by 2 so we don't go over 1.0

; Control-path Instruments
Instrument -g multiply.dsp
Instrument 1-(g*g) timesplus.dsp

; Connect the control path
Connect	.	allp_g	0	-g	InputA	0
Constant			-g	InputB	0	-1.0
Connect	-g	Output	0	AllpOut	Gain	0

Connect	-g	Output	0	1-(g*g)	InputA	0
Connect	.	allp_g	0	1-(g*g)	InputB	0
Constant			1-(g*g) InputC	0	1.0
Connect	1-(g*g)	Output	0	AllpOut	Gain	1

Constant			AllpIn	Gain	0	0.5
Connect	.	allp_g	0	AllpIn	Gain	1

; -----------------------
; Final output mixing
; -----------------------

Mixer DryMix 3 2
Instrument Invert multiply.dsp

Knob Dry	1 	Signed 	0.4	; input->output
Knob Wet	1 	Signed 	-0.6	; delay->output

Connect	.	Input	0	DryMix	Input	0
Connect	.	Input	1	DryMix	Input	1
Connect	AllpOut	Output	0	DryMix	Input	2

Connect .	Dry	0	DryMix	Gain	0
Constant			DryMix	Gain	3	0.0
Constant			DryMix	Gain	1	0.0
Connect	.	Dry	0	DryMix	Gain	4
Connect	.	Wet	0	DryMix	Gain	2	; equal amounts L/R
Connect	.	Wet	0	Invert	InputA	0
Constant			Invert	InputB	0	-1.0
Connect	Invert	Output	0	DryMix	Gain	5

Connect DryMix	Output	0	.	Output	0
Connect	DryMix	Output	1	.	Output	1
