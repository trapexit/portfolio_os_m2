; @(#) sawenv.mp 95/11/20 1.3
;
; sawtooth oscillator with amplitude envelope

; instruments
Instrument env envelope.dsp
Instrument osc sawtooth.dsp

; ports and knobs
Knob Frequency 1 OscFreq 261.63 ; default to middle C
Knob Amplitude 1 Signed 1.0     ; default to full amplitude
Output Output 1 Signed

; expose so we can attach envelope below
Expose AmpEnv env Env

; route patch's Frequency and Amplitude knobs
Connect . Frequency 0 osc Frequency 0
Connect . Amplitude 0 env Amplitude 0

; connect envelope output to oscillator amplitude
Connect env Output 0 osc Amplitude 0

; connect oscillator output to patch's output
Connect osc Output 0 . Output 0

; envelope
;                       A        D    S    R
Envelope env Signed  0 0.2  1.0 0.5  0.61 1.0  0 0
EnvSustainLoop env 2 2 0.0      ; sustain at single point.
EnvReleaseJump env 2            ; jump immediately to release, even if not
                                ; at sustain yet.
Attach env AmpEnv -FatLadySings ; -FatLadySings causes instrument to stop
                                ; when envelope completes.
