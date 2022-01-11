; @(#) phone.mp 96/02/13 1.3
;
; Telephone noise
;
; Author: rnm
; Created: 1/26/96

Instrument osc sawtooth.dsp
Instrument lfo square_lfo.dsp
Instrument timesplus timesplus.dsp
Instrument timesplus2 timesplus.dsp
Instrument multiply multiply.dsp
Instrument onoff square_lfo.dsp
Knob LFORate 1 LFOFreq 20.0
Knob LFODepth 1 OscFreq 160.0
Knob SampleRate 1 OscFreq 1680.0
Knob Amplitude 1 Signed 0.4
Knob OnRate 1 LFOFreq 0.17
Output Output 1 Signed

Connect	osc	Output		0	.		Output		0
Connect	.	LFORate		0	lfo		Frequency	0
Connect	lfo	Output		0	timesplus	InputA		0
Connect	.	LFODepth	0	timesplus	InputB		0
Connect	.	SampleRate	0	timesplus	InputC		0
Connect	timesplus	Output	0	osc		Frequency	0

Connect	.	OnRate		0	onoff		Frequency	0
Connect	onoff	Output		0	timesplus2	InputA		0
Constant				timesplus2	InputB		0	0.5
Constant				timesplus2	InputC		0	0.5
Connect	timesplus2	Output	0	multiply	InputA		0
Connect	.	Amplitude	0	multiply	InputB		0
Connect	multiply	Output	0	osc		Amplitude	0
