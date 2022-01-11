;******************************************************************************
;*
;*  @(#) multi_lowpass.mp 95/12/15 1.2
;*  
;*  multiple low-pass comb filters, each with their own delay line.
;*
;*  Tapping all the signals off the one delay line just doesn't sound as
;*  good as using separate delays for each filter, because of the inter-
;*  action of the signals.  This version uses a small, separate delay for
;*  each filter element, forming a classic comb filter.  We keep the
;*  svfilters in the feedback loops.  The delays are sized slightly
;*  differently to give the varying delay times, rather than positioning
;*  taps at different points.
;*
;*  According to the references, less than six elements makes an audible
;*  difference in sound quality.
;*
;                        dry gain
;                      +---+        dry signal
;   +------------------| * |-------------------------------+
;   |                  +---+                               |
;   |                                                      v 
;   |  +---+        +-------------------+     +---+      +---+
;  -+->| + |------->|  delay            |--+--| * |------|   |
;   |  +---+        +-------------------+  |  +---+      |   | 
;   |    ^                                 |             |   |
;   |    |     +---+   +----+              |             |   |
;   |    +-----| * |---| sv |--------------+             |   |
;   |          +---+   +----+                            |   |
;   |  +---+        +-------------------+     +---+      |   |
;   +->| + |------->|  delay            |--+--| * |------| + |----->
;   |  +---+        +-------------------+  |  +---+      |   | 
;   |    ^                                 |             |   |
;   |    |     +---+   +----+              |             |   |
;   |    +-----| * |---| sv |--------------+             |   |
;   |          +---+   +----+                            |   |
;   |  +---+        +-------------------+     +---+      |   |
;   +->| + |------->|  delay            |--+--| * |------|   |----->
;   |  +---+        +-------------------+  |  +---+      |   | 
;   |    ^                                 |             .   .
;   .    |     +---+   +----+              |             .   .
;   .    +-----| * |---| sv |--------------+             .   .
;   .          +---+   +----+                                 
;             feedback  filters                               
;             gains
;
;******************************************************************************

; Instruments
Input Input 1 Signed

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

Instrument lp1 svfilter.dsp
Instrument lp2 svfilter.dsp
Instrument lp3 svfilter.dsp
Instrument lp4 svfilter.dsp
Instrument lp5 svfilter.dsp
Instrument lp6 svfilter.dsp

; Delay line length = (time * 44100 + 50) * 2 sample guard
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
Mixer InMix1 2 1
Mixer InMix2 2 1
Mixer InMix3 2 1
Mixer InMix4 2 1
Mixer InMix5 2 1
Mixer InMix6 2 1
Mixer OutMix 7 1
Output Output 1 Signed

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

Connect	lp1	Output	0	InMix1	Input	1
Connect	lp2	Output	0	InMix2	Input	1
Connect	lp3	Output	0	InMix3	Input	1
Connect	lp4	Output	0	InMix4	Input	1
Connect	lp5	Output	0	InMix5	Input	1
Connect	lp6	Output	0	InMix6	Input	1

Connect	InMix1	Output	0	DelIn1	Input	0
Connect	InMix2	Output	0	DelIn2	Input	0
Connect	InMix3	Output	0	DelIn3	Input	0
Connect	InMix4	Output	0	DelIn4	Input	0
Connect	InMix5	Output	0	DelIn5	Input	0
Connect	InMix6	Output	0	DelIn6	Input	0

Connect DelOut1	Output	0	lp1	Input	0
Connect DelOut2	Output	0	lp2	Input	0
Connect DelOut3	Output	0	lp3	Input	0
Connect DelOut4	Output	0	lp4	Input	0
Connect DelOut5	Output	0	lp5	Input	0
Connect DelOut6	Output	0	lp6	Input	0

Connect	DelOut1	Output	0	OutMix	Input	1
Connect	DelOut2	Output	0	OutMix	Input	2
Connect DelOut3	Output	0	OutMix	Input	3
Connect	DelOut4	Output	0	OutMix	Input	4
Connect	DelOut5	Output	0	OutMix	Input	5
Connect DelOut6	Output	0	OutMix	Input	6

Connect OutMix	Output	0	.	Output	0

; Preset some of the mixer gains.

; Define knobs for tweaking mix
Knob Dry	1 	Signed 	0.4	; input->output
Knob Wet	1 	Signed 	-0.6	; delay lines->output
Knob g		1	Signed	-0.83	; loop gain
Knob lpFreq	1	Signed	0.134	; tune to sample for best results!
Knob lpQ	1	Signed	1.0	; minimal Q

; Scale the wet
Instrument *wet multiply.dsp
Constant *wet InputB 0 0.16		; 1/# of delays

; Connect the knobs up
Constant InMix1	Gain	0	0.5	; input->delay line always half
Constant InMix2	Gain	0	0.5
Constant InMix3	Gain	0	0.5
Constant InMix4	Gain	0	0.5
Constant InMix5	Gain	0	0.5
Constant InMix6	Gain	0	0.5

Connect	.	g	0	InMix1	Gain	1
Connect	.	g	0	InMix2	Gain	1
Connect	.	g	0	InMix3	Gain	1
Connect	.	g	0	InMix4	Gain	1
Connect	.	g	0	InMix5	Gain	1
Connect	.	g	0	InMix6	Gain	1

Connect	.	lpFreq	0	lp1	Frequency	0
Connect	.	lpFreq	0	lp2	Frequency	0
Connect	.	lpFreq	0	lp3	Frequency	0
Connect	.	lpFreq	0	lp4	Frequency	0
Connect	.	lpFreq	0	lp5	Frequency	0
Connect	.	lpFreq	0	lp6	Frequency	0

Connect	.	lpQ	0	lp1	Resonance	0
Connect	.	lpQ	0	lp2	Resonance	0
Connect	.	lpQ	0	lp3	Resonance	0
Connect	.	lpQ	0	lp4	Resonance	0
Connect	.	lpQ	0	lp5	Resonance	0
Connect	.	lpQ	0	lp6	Resonance	0

Connect .	Dry	0	OutMix	Gain	0
Connect	.	Wet	0	*wet	InputA	0
Connect	*wet	Output	0	OutMix	Gain	1
Connect	*wet	Output	0	OutMix	Gain	2
Connect	*wet	Output	0	OutMix	Gain	3
Connect	*wet	Output	0	OutMix	Gain	4
Connect	*wet	Output	0	OutMix	Gain	5
Connect	*wet	Output	0	OutMix	Gain	6

; That's it.  Here's some sample input.  Pick a sample off the command
; line.

; Instrument source sampler_16_f1.dsp
; Expose InFIFO source InFIFO

; Connect source	Output	0	InMix1	Input	0
; Connect source	Output	0	InMix2	Input	0
; Connect source	Output	0	InMix3	Input	0
; Connect source	Output	0	InMix4	Input	0
; Connect source	Output	0	InMix5	Input	0
; Connect source	Output	0	InMix6	Input	0

; Connect source	Output	0	OutMix	Input	0

; Or alternatively, uncomment out below, comment out above, and hook
; something to the Input of the patch (say, line.in)

Connect	.	Input	0	InMix1	Input	0
Connect	.	Input	0	InMix2	Input	0
Connect	.	Input	0	InMix3	Input	0
Connect	.	Input	0	InMix4	Input	0
Connect	.	Input	0	InMix5	Input	0
Connect	.	Input	0	InMix6	Input	0

Connect	.	Input	0	OutMix	Input	0


