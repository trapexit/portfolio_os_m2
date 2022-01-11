; @(#) tac_simple.mp 96/02/28 1.1
; Test new accumulator instruments

Instrument inA input_accum.dsp
Instrument addA add_accum.dsp
Instrument subA subtract_from_accum.dsp
Instrument outA output_accum.dsp
Knob inVal 1 Signed 0.0
Knob Add 1 Signed 0.0
Knob Subtract 1 Signed 0.0
Output Output 1 Signed

Connect . inVal 0 inA Input 0
Connect . Add 0 addA Input 0
Connect . Subtract 0 subA Input 0
Connect outA Output 0 . Output 0
