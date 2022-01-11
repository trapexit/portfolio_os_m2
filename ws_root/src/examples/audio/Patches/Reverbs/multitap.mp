;******************************************************************************
;*
;*  @(#) multitap.mp 95/12/15 1.3
;*
;******************************************************************************

; Multi tap reverberator

; To get denser reverberation, we can use more than one tap off the delay
; line mixed back into the wet signal.  Ideally the taps should be spaced
; so as not to reinforce or cancel each other. In this example, they're
; spaced at 50, 56, 61, 68, 72 and 78 ms.  The diagram looks like this:
;
;                        dry gain
;                      +---+        dry signal
;   +------------------| * |-------------------------------+
;   |                  +---+                               |
;   |                                                      v 
;   |  +---+        +-------------------+                +---+
;  -+->| + |------->|  delay            |                | + |----->
;      |   |        +-------------+-+--++                |   | 
;      |   |                      | |  |                 |   |
;      |   |           +---+      | |  |                 |   |
;      |   |<----------| * |------+-|--|---------------->|   |
;      |   |           +---+        |  |                 |   |
;      |   |                        |  |                 |   |
;      |   |           +---+        |  |                 |   |
;      |   |<----------| * |--------+--|---------------->|   |
;      |   |           +---+           |                 |   |
;      |   |                           |                 |   |
;      |   |           +---+           |                 |   |
;      |   |<----------| * |-----------+---------------->|   |
;      |   |           +---+                             |   |
;       etc              feedback gains                   etc
;
; Different delay lengths create different comb filter responses,
; ameliorating the harmonic effect of the single tap somewhat.

; Instruments
Input Input 1 Signed

Instrument DelOut1 sampler_raw_f1.dsp
Instrument DelOut2 sampler_raw_f1.dsp
Instrument DelOut3 sampler_raw_f1.dsp
Instrument DelOut4 sampler_raw_f1.dsp
Instrument DelOut5 sampler_raw_f1.dsp
Instrument DelOut6 sampler_raw_f1.dsp

Instrument DelIn delay_f1.dsp

; We need a maximum delay of 0.1s = (0.1 * 44100 + 50) * 2  = 8920 samples

Delayline Delay 8920 1
Mixer Mix 7 2
Output Output 1 Signed

; Hook up the delay line to the in and out
Expose dOutFIFO1 DelOut1 InFIFO
Expose dOutFIFO2 DelOut2 InFIFO
Expose dOutFIFO3 DelOut3 InFIFO
Expose dOutFIFO4 DelOut4 InFIFO
Expose dOutFIFO5 DelOut5 InFIFO
Expose dOutFIFO6 DelOut6 InFIFO
Attach Delay dOutFIFO1 -startat 2205	; 0.050 s
Attach Delay dOutFIFO2 -startat 1940	; 0.056 s
Attach Delay dOutFIFO3 -startat 1720	; 0.061 s
Attach Delay dOutFIFO4 -startat 1411	; 0.068 s
Attach Delay dOutFIFO5 -startat 1235	; 0.072 s
Attach Delay dOutFIFO6 -startat  970	; 0.078 s

Expose dInFIFO DelIn OutFIFO
Attach Delay dInFIFO

; Hook up the rest of the audio path

; Connect	.	Input	0	Mix	Input	0
; We're commenting this out, and connecting a sample instead (see below).

Connect DelOut1	Output	0	Mix	Input	1
Connect DelOut2	Output	0	Mix	Input	2
Connect DelOut3	Output	0	Mix	Input	3
Connect DelOut4	Output	0	Mix	Input	4
Connect DelOut5	Output	0	Mix	Input	5
Connect DelOut6	Output	0	Mix	Input	6

Connect Mix	Output	0	.	Output	0
Connect Mix	Output	1	DelIn	Input	0

; Preset some of the mixer gains.  Assume an overall feedback gain of 0.83
; which gives about 2 seconds of overall decay given the delays above.

; Define knobs for tweaking mix
Knob Dry	1 	Signed 	0.7	; input->output
Knob Wet	1 	Signed 	-0.3	; delay line->output
Knob Fb1	1	Signed	-0.1009
Knob Fb2	1 	Signed 	-0.0983
Knob Fb3	1	Signed	-0.0956
Knob Fb4	1	Signed	-0.0943
Knob Fb5	1	Signed	-0.0930
Knob Fb6	1	Signed	-0.0903	; feedback gains

; We need to scale the "wet" knob to divide the value by the number of
; filters, so that the output doesn't go over 1.0

Instrument Scale multiply.dsp
Constant Scale InputB 0 0.16		; 1/6 is about 0.16

; Connect the knobs up
Connect .	Dry	0	Mix	Gain	0
Connect	.	Wet	0	Scale	InputA	0
Connect	Scale	Output	0	Mix	Gain	1
Connect	Scale	Output	0	Mix	Gain	2
Connect	Scale	Output	0	Mix	Gain	3
Connect	Scale	Output	0	Mix	Gain	4
Connect	Scale	Output	0	Mix	Gain	5
Connect	Scale	Output	0	Mix	Gain	6

Constant Mix	Gain	7	0.5	; input->delay line always half on

Connect	.	Fb1	0	Mix	Gain	8
Connect	.	Fb2	0	Mix	Gain	9
Connect	.	Fb3	0	Mix	Gain	10
Connect	.	Fb4	0	Mix	Gain	11
Connect	.	Fb5	0	Mix	Gain	12
Connect	.	Fb6	0	Mix	Gain	13

; That's it.  Here's some sample input.  Pick a sample off the command
; line.

; Instrument source sampler_16_f1.dsp
; Expose InFIFO source InFIFO

; Connect source	Output	0	Mix	Input	0

; Or alternatively, uncomment out below, comment out above, and hook
; something to the Input of the patch (say, line.in)

Connect	.	Input	0	Mix	Input	0
