; @(#) 3osc.mp 96/02/15 1.2
;
; 3-oscillator synth w/ vibrato

    ; ports
Knob Frequency 1 OscFreq 440
Knob Amplitude 1 Signed 1.0
Knob ModRate 1 LFOFreq 5.0
Knob ModDepth 1 Signed 0
Output Output 1 Signed

    ; templates
Instrument freq_lfo triangle_lfo.dsp
Instrument freq_mod timesplus.dsp
Mixer      freq_scaler 1 3
Instrument saw_osc sawtooth.dsp
Instrument tri_osc triangle.dsp
Instrument pulse_osc pulse.dsp
Mixer      mixer 3 1 -amplitude

    ; freq mod
Connect . ModRate 0 freq_lfo Frequency 0
Connect . ModDepth 0 freq_lfo Amplitude 0
Connect freq_lfo Output 0 freq_mod InputA 0
Constant freq_mod InputB 0 0.01      ; scale ModDepth reasonably for vibrato
Connect . Frequency 0 freq_mod InputC 0

    ; freq scaler -> frequency knobs
Connect freq_mod Output 0 freq_scaler Input 0
Constant freq_scaler Gain 0 1.0
Constant freq_scaler Gain 1 0.5
Constant freq_scaler Gain 2 0.25

Connect freq_scaler Output 0 saw_osc Frequency 0
Connect freq_scaler Output 1 tri_osc Frequency 0
Connect freq_scaler Output 2 pulse_osc Frequency 0

    ; outputs -> mixer
Connect saw_osc   Output 0 mixer Input 0
Connect pulse_osc Output 0 mixer Input 1
Connect tri_osc   Output 0 mixer Input 2

    ; preset oscillator levels
Constant mixer Gain 0 0.5
Constant mixer Gain 1 0.25
Constant mixer Gain 2 0.75

    ; amplitude knob -> mixer
Connect . Amplitude 0 mixer Amplitude 0
Connect mixer Output 0 . Output 0
