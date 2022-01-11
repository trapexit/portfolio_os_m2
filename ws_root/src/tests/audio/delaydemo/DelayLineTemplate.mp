; @(#) DelayLineTemplate.mp 96/07/02 1.2
; Demonstrates how using a Delay Line Template solves problems demonstrated in DelayLine.patch

coherence off


; --------------------- top level

; ports and knobs
Knob Frequency 1 OscFreq 261.63 ; default to middle C
Knob Amplitude 1 Signed 1.0     ; default to full amplitude
Output Output 1 Signed

; knob routing
Connect . Frequency 0 osc Frequency 0
Connect . Amplitude 0 ampenv Amplitude 0

; signal routing
Connect filter Output 0 DelaySendMixer Input 0      ; source -> delay
Connect filter Output 0 DelayOutputMixer Input 0    ; source -> delay
Connect DelayOutputMixer Output 0 . Output 0        ; delay -> output


; --------------------- sound source
;
; Knobs
;
;   osc Frequency 0
;       Instrument frequency.
;
;   ampenv Amplitude 0
;       Instrument amplitude.
;
; Outputs
;
;   filter Output 0
;       Instrument output.

; instruments
Instrument ampenv envelope.dsp
Instrument filterenv envelope.dsp
Instrument filter svfilter.dsp
Instrument osc sawtooth.dsp

; expose so we can attach envelope below
Expose AmpEnv ampenv Env
Expose FilterEnv filterenv Env

; connect envelopes
Connect ampenv Output 0 Filter Amplitude 0
Connect filterenv Output 0 Filter Frequency 0
Constant filter Resonance 0 0.5

; signal routing
Connect osc Output 0 filter Input 0

; envelopes
;                                                   0       1         2         3        4
Envelope AmpEnv Signed -PitchTimeScaling 48 24      0 0.01  1.0 0.01  0.61 0.7  0.0 0.1  0 0
EnvReleaseJump AmpEnv 3            ; jump immediately to release, even if not at sustain yet.
Attach AmpEnv AmpEnv

;                                                   0         1         2          3        4         5
Envelope FilterEnv Signed -PitchTimeScaling 48 24   0.1 0.07  1.0 0.10  0.75 0.10  0.4 0.10 0.15 0.1  0.15 0
EnvReleaseJump FilterEnv 4         ; jump immediately to release, even if not at sustain yet.
Attach FilterEnv FilterEnv


; --------------------- 1-tap delay
;
; Inputs
;
;   DelaySendMixer Input 0, DelayOutputMixer Input 0
;       Input signal.
;
; Outputs
;
;   DelayOutputMixer Output 0
;       Output signal.

Mixer       DelaySendMixer   2 1
Instrument  DelaySend        delay_f1.dsp
Instrument  DelayTap         sampler_raw_f1.dsp
Instrument  DelayAmpEnv      envelope.dsp
Mixer       DelayOutputMixer 2 1 -Amplitude

Expose DelaySendFIFO DelaySend OutFIFO
Expose DelayTapFIFO  DelayTap  InFIFO
Expose DelayAmpEnv   DelayAmpEnv Env

Connect DelaySendMixer   Output 0   DelaySend        Input 0        ; send mixer -> send
Connect DelayTap         Output 0   DelayOutputMixer Input 1        ; tap -> output mixer
Connect DelayTap         Output 0   DelaySendMixer   Input 1        ; delay feedback
Connect DelayAmpEnv      Output 0   DelayOutputMixer Amplitude 0    ; output amplitude envelope

Constant DelayOutputMixer Gain 0 +0.99  ; bypass output level
Constant DelayOutputMixer Gain 1 -0.99  ; delay output level
Constant DelaySendMixer   Gain 0 +0.5   ; input level
Constant DelaySendMixer   Gain 1 -0.999 ; feedback level

; 1/125 min delay line
DelayLineTemplate DelayLine 42436 1
Attach DelayLine DelaySendFIFO
Attach DelayLine DelayTapFIFO -startat 50

; envelopes
;                               0      1
Envelope DelayAmpEnv Signed     1 2.0  0 0
EnvSustainLoop DelayAmpEnv 0 0 0.0
Attach DelayAmpEnv DelayAmpEnv -FatLadySings
