; @(#) bee3dspace.mp 96/02/23 1.4
;
; 3DSpace output stage
;
; Takes input from the various 3D sources, mixes them to left and right
; along with a reverb side chain.
;
; Author: rnm
; Created: 1/26/96

Input Input 6 Signed		; fed from the 3d sources
Mixer OutMix 8 3		; stereo out, plus mono to reverb, plus reverb return
Output Output 2 Signed

; Hook up the audio path
Connect	.		Input	0	OutMix		Input	0
Connect	.		Input	1	OutMix		Input	1
Connect	.		Input	2	OutMix		Input	2
Connect	.		Input	3	OutMix		Input	3
Connect	.		Input	4	OutMix		Input	4
Connect	.		Input	5	OutMix		Input	5
Connect	AllpOut1	Output	0	OutMix		Input	6
Connect AllpOut2	Output	0	OutMix		Input	7

Connect	OutMix		Output	0	.		Output	0
Connect	OutMix		Output	1	.		Output	1

; Control Knobs
Knob Gain 4 Signed 0.33		; signal gain for each input
Knob PreSend 3 Signed 0.33	; send to reverb from each input

; Hook up the control path as stereo pairs
Connect .	Gain	0	OutMix	Gain	0
Constant			OutMix	Gain	8	0.0

Constant			OutMix	Gain	1	0.0
Connect	.	Gain	0	OutMix	Gain	9

Connect	.	Gain	1	OutMix	Gain	2
Constant			OutMix	Gain	10	0.0

Constant			OutMix	Gain	3	0.0
Connect	.	Gain	1	OutMix	Gain	11

Connect	.	Gain	2	OutMix	Gain	4
Constant			OutMix	Gain	12	0.0

Constant			OutMix	Gain	5	0.0
Connect	.	Gain	2	OutMix	Gain	13

Connect	.	Gain	3	OutMix	Gain	6
Constant			OutMix	Gain	14	0.0

Constant			OutMix	Gain	7	0.0
Connect	.	Gain	3	OutMix	Gain	15

; Hook up reverb sends
Connect	.	PreSend	0	OutMix	Gain	16
Connect	.	PreSend	0	OutMix	Gain	17
Connect	.	PreSend	1	OutMix	Gain	18
Connect	.	PreSend	1	OutMix	Gain	19
Connect	.	PreSend	2	OutMix	Gain	20
Connect	.	PreSend	2	OutMix	Gain	21
Constant			OutMix	Gain	22	0.0
Constant			OutMix	Gain	23	0.0	; don't send reverb return back into reverb

; -------------------
; Reverb section

; -----------------------
; Comb filter bank
; -----------------------

; Instruments
Instrument DelOut1 sampler_raw_f1.dsp
Instrument DelOut2 sampler_raw_f1.dsp
Instrument DelOut3 sampler_raw_f1.dsp
Instrument DelIn1 delay_f1.dsp
Instrument DelIn2 delay_f1.dsp
Instrument DelIn3 delay_f1.dsp

Instrument lp1 filter_1o1z.dsp
Instrument lp2 filter_1o1z.dsp
Instrument lp3 filter_1o1z.dsp

; Delay line length = time * 44100 * 2 + 50 sample guard
; 50ms = 4510
; 56ms = 5039
; 61ms = 5480

Delayline Delay1 4510 1
Delayline Delay2 5040 1	; must be even number of bytes
Delayline Delay3 5480 1
Mixer CombIn1 2 1
Mixer CombIn2 2 1
Mixer CombIn3 2 1
Mixer CombOut 3 1

; Hook up the delay lines to the ins and outs
Expose dOutFIFO1 DelOut1 InFIFO
Expose dOutFIFO2 DelOut2 InFIFO
Expose dOutFIFO3 DelOut3 InFIFO
Attach Delay1 dOutFIFO1 -startat 50
Attach Delay2 dOutFIFO2 -startat 50
Attach Delay3 dOutFIFO3 -startat 50

Expose dInFIFO1 DelIn1 OutFIFO
Expose dInFIFO2 DelIn2 OutFIFO
Expose dInFIFO3 DelIn3 OutFIFO
Attach Delay1 dInFIFO1
Attach Delay2 dInFIFO2
Attach Delay3 dInFIFO3

; Hook up the rest of the audio path

Connect	OutMix	Output	2	CombIn1	Input	0
Connect	OutMix	Output	2	CombIn2	Input	0
Connect	OutMix	Output	2	CombIn3	Input	0

Connect	lp1	Output	0	CombIn1	Input	1
Connect	lp2	Output	0	CombIn2	Input	1
Connect	lp3	Output	0	CombIn3	Input	1

Connect	CombIn1	Output	0	DelIn1	Input	0
Connect	CombIn2	Output	0	DelIn2	Input	0
Connect	CombIn3	Output	0	DelIn3	Input	0

Connect DelOut1	Output	0	lp1	Input	0
Connect DelOut2	Output	0	lp2	Input	0
Connect DelOut3	Output	0	lp3	Input	0

Connect	DelOut1	Output	0	CombOut	Input	0
Connect	DelOut2	Output	0	CombOut	Input	1
Connect DelOut3	Output	0	CombOut	Input	2

; Preset some of the mixer gains.

; Define knobs for tweaking mix
Knob g		1	Signed	-0.83	; loop gain
Knob lpA0	1	Signed	0.5	; tune to sample for best results!
Knob lpA1	1	Signed	0.5	; minimal Q

; Connect the knobs up
Constant 			CombIn1	Gain	0	0.5	; input->delay line always half
Constant 			CombIn2	Gain	0	0.5
Constant 			CombIn3	Gain	0	0.5

Connect	.	g	0	CombIn1	Gain	1
Connect	.	g	0	CombIn2	Gain	1
Connect	.	g	0	CombIn3	Gain	1

Connect	.	lpA0	0	lp1	A0	0
Connect	.	lpA0	0	lp2	A0	0
Connect	.	lpA0	0	lp3	A0	0

Connect	.	lpA1	0	lp1	A1	0
Connect	.	lpA1	0	lp2	A1	0
Connect	.	lpA1	0	lp3	A1	0

Constant			CombOut	Gain	0	0.16
Constant			CombOut	Gain	1	0.16
Constant			CombOut	Gain	2	0.16

; -----------------------
; Allpass reverb filter on the mixed output of the comb filter stage
; -----------------------

; Signal-path Instruments
Instrument dOutAllp1 sampler_raw_f1.dsp
Instrument dOutAllp2 sampler_raw_f1.dsp
Instrument dInAllp1 delay_f1.dsp
Instrument dInAllp2 delay_f1.dsp
Delayline dlAllp1 580 1		; about 6ms
Delayline dlAllp2 532 1
Mixer AllpIn1 2 1
Mixer AllpIn2 2 1
Mixer AllpOut1 2 1
Mixer AllpOut2 2 1

; Hook up the delay lines to the ins and outs
Expose dOutFIFOa1 dOutAllp1 InFIFO
Expose dOutFIFOa2 dOutAllp2 InFIFO
Attach dlAllp1 dOutFIFOa1 -startat 50
Attach dlAllp2 dOutFIFOa2 -startat 50
Expose dInFIFOa1 dInAllp1 OutFIFO
Expose dInFIFOa2 dInAllp2 OutFIFO
Attach dlAllp1 dInFIFOa1
Attach dlAllp2 dInFIFOa2

; Hook up the signal path
Connect	CombOut		Output	0	AllpIn1		Input	0
Connect	CombOut		Output	0	AllpOut1	Input	0

Connect	CombOut		Output	0	AllpIn2		Input	0
Connect	CombOut		Output	0	AllpOut2	Input	0

Connect	AllpIn1		Output	0	dInAllp1	Input	0
Connect	dOutAllp1	Output	0	AllpOut1	Input	1
Connect	dOutAllp1	Output	0	AllpIn1		Input	1

Connect	AllpIn2		Output	0	dInAllp2	Input	0
Connect	dOutAllp2	Output	0	AllpOut2	Input	1
Connect	dOutAllp2	Output	0	AllpIn2		Input	1

; Knob to tweak the gain
Knob allp_g 1 Signed 0.35	; scaled by 2 so we don't go over 1.0

; Control-path Instruments
Instrument -g1 multiply.dsp
Instrument -g2 multiply.dsp
Instrument 1-(g*g)1 timesplus.dsp
Instrument 1-(g*g)2 timesplus.dsp

; Connect the control path
Connect	.	allp_g	0	-g1	InputA	0
Constant			-g1	InputB	0	-1.0
Connect	-g1	Output	0	AllpOut1	Gain	0

Connect	.	allp_g	0	-g2	InputA	0
Constant			-g2	InputB	0	-1.0
Connect	-g2	Output	0	AllpOut2	Gain	0

Connect	-g1	Output	0	1-(g*g)1	InputA	0
Connect	.	allp_g	0	1-(g*g)1	InputB	0
Constant			1-(g*g)1 InputC	0	1.0
Connect	1-(g*g)1	Output	0	AllpOut1	Gain	1

Constant			AllpIn1	Gain	0	0.5
Connect	.	allp_g	0	AllpIn1	Gain	1

Connect	-g2	Output	0	1-(g*g)2	InputA	0
Connect	.	allp_g	0	1-(g*g)2	InputB	0
Constant			1-(g*g)2 InputC	0	1.0
Connect	1-(g*g)2	Output	0	AllpOut2	Gain	1

Constant			AllpIn2	Gain	0	0.5
Connect	.	allp_g	0	AllpIn2	Gain	1
