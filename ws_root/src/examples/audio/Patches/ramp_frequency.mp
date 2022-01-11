
;******************************************************************************
;*
;*  @(#) ramp_frequency.mp 96/08/22 1.4
;*
;*  Tutorial 10: Use envelope.dsp as a ramp generator
;*
;******************************************************************************

;  As described in the autodocs on the envelope.dsp, you can generate ramps
;  on the fly without needing an attached envelope by dynamically setting
;  the target toward which to ramp (in the knob Env.Request) and the step
;  size to move with (in the knob Env.Incr).  Env.Incr effectively controls
;  the ramp rate - large values correspond to short times. Values at 0.0015 or
;  below are good for smoothing out abrupt volume changes.

Instrument ramp envelope.dsp
Instrument scalar multiply.dsp
Knob RampRate 1 Signed 0.1
Constant scalar InputB 0 0.001          ; scale knob down to usable values
Connect . RampRate 0 scalar InputA 0
Connect scalar Output 0 ramp Env.incr 0

    ; sound source
Instrument src sampler_16_v1.dsp
Sample samp /remote/Samples/PitchedL/FluteLite/Flute.As4LM44k.aiff
Expose InFIFO src InFIFO
Attach samp InFIFO

    ; output
Output Output 1 Signed

    ; random frequency control
    ; every 1 second, this generates a new target value
Instrument sh randomhold.dsp
Constant sh Frequency 0 1.0
Constant sh Amplitude 0 0.8

    ; control knobs
Knob Amplitude 1 Signed 1.0

    ; Envelopes work with signed values which range from -1.0 to 1.0.
    ; SamplerRate, however, is specified as unsigned values which range
    ; from 0.0 to 2.0. To avoid strange wrap-around effects, we will
    ; add 1.0 to the envelope output
Instrument  timesplus  timesplus_noclip.dsp
Constant    timesplus  InputB 0 1.0   ; Output = A*1.0 + 1.0
Constant    timesplus  InputC 0 1.0

    ; connections: use the envelope to control frequency
Connect .         Amplitude 0  src  Amplitude   0
Connect sh        Output   0   ramp env.request 0
Connect ramp      Output   0   timesplus InputA 0
Connect timesplus Output   0   src  SampleRate  0
Connect src       Output   0   .    Output      0
