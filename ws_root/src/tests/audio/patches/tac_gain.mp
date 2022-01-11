; @(#) tac_gain.mp 96/02/28 1.1
; Test accumulator multiplies

Instrument osc sawtooth.dsp
Instrument inA input_accum.dsp
Instrument multA multiply_accum.dsp
Instrument addA add_accum.dsp
Instrument outA output_accum.dsp
Knob Gain 1 Signed 0.0
Knob Offset 1 Signed 0.0
Output Output 1 Signed

Connect osc Output 0 inA Input 0
Connect outA Output 0 . Output 0
Connect . Gain 0 multA Input 0
Connect . Offset 0 addA Input 0

