\ @(#) function_ids.j 96/08/01 1.28
\ $Id: function_ids.j,v 1.10 1995/01/23 18:36:25 phil Exp $
\ Function IDs used to positively identify functionality for BullDog
\
\ Never change the order or value of these constants once released!!!!!!!!
\ Only add to the end.
\
\ Copyright 1993-4 3DO
\ Author: Phil Burk

anew task-function_ids

: newfunc:  ( n <name> -- n+1 )
	dup 1+ swap constant
;

0  \ starting value
newfunc: DFID_ZERO      \ 0 = illegal value
newfunc: DFID_TEST      \ 1 = test instrument, only allowed in development mode
newfunc: DFID_PATCH     \ 2 = patch constructed by ARIA or patch builder
newfunc: DFID_MIXER     \ 3 = mixer constructed by CreateMixerTemplate()
drop
10 constant DFID_FIRST_LOADABLE  \ start at 10 so we have room for more reserved IDs
DFID_FIRST_LOADABLE
newfunc: DFID_ADD
newfunc: DFID_ADD_NOCLIP
newfunc: DFID_ADD_UNSIGNED
newfunc: DFID_BENCHMARK
newfunc: DFID_CUBIC_AMPLIFIER
newfunc: DFID_DEEMPHCD
newfunc: DFID_DELAY4
newfunc: DFID_DELAY_F1
newfunc: DFID_DELAY_F2
newfunc: DFID_DEPOPPER
newfunc: DFID_ENVELOPE
newfunc: DFID_ENVFOLLOWER
newfunc: DFID_EXPMOD_UNSIGNED
newfunc: DFID_FILTER1POLE
newfunc: DFID_FILTER1POLE1ZERO
newfunc: DFID_FILTER2POLE
newfunc: DFID_FILTER2POLE2ZERO
newfunc: DFID_FILTER2ZERO
newfunc: DFID_FILTEREDNOISE
newfunc: DFID_FILTER_1O1Z
newfunc: DFID_FILTER_3D
newfunc: DFID_IMPULSE
newfunc: DFID_INTEGRATOR
newfunc: DFID_INTERP_TABLE17
newfunc: DFID_LATCH
newfunc: DFID_LINE_IN
newfunc: DFID_LINE_OUT
newfunc: DFID_MAXIMUM
newfunc: DFID_MINIMUM
newfunc: DFID_MULTIPLY
newfunc: DFID_MULTIPLY_UNSIGNED
newfunc: DFID_NANOKERNEL
newfunc: DFID_NOISE
newfunc: DFID_PULSE
newfunc: DFID_PULSE_LFO
newfunc: DFID_RANDOMHOLD
newfunc: DFID_REDNOISE
newfunc: DFID_REDNOISE_LFO
newfunc: DFID_RX1-X
newfunc: DFID_SAMPLER_16_F1
newfunc: DFID_SAMPLER_16_F2
newfunc: DFID_SAMPLER_16_V1
newfunc: DFID_SAMPLER_16_V2
newfunc: DFID_SAMPLER_8_F1
newfunc: DFID_SAMPLER_8_F2
newfunc: DFID_SAMPLER_8_V1
newfunc: DFID_SAMPLER_8_V2
newfunc: DFID_SAMPLER_ADP4_V1
newfunc: DFID_SAMPLER_ADP4_V2
newfunc: DFID_SAMPLER_ADP4_V4
newfunc: DFID_SAMPLER_ADP4_V8
newfunc: DFID_RESERVED
newfunc: DFID_SAMPLER_CBD2_F1
newfunc: DFID_SAMPLER_CBD2_F2
newfunc: DFID_SAMPLER_CBD2_V1
newfunc: DFID_SAMPLER_CBD2_V2
newfunc: DFID_SAMPLER_CBD2_V4
newfunc: DFID_SAMPLER_DRIFT_V1
newfunc: DFID_SAMPLER_DUCK_V1
newfunc: DFID_SAMPLER_DUCK_V2
newfunc: DFID_SAMPLER_RAW_F1
newfunc: DFID_SAMPLER_SQS2_F1
newfunc: DFID_SAMPLER_SQS2_V1
newfunc: DFID_SAWTOOTH
newfunc: DFID_SAWTOOTH_LFO
newfunc: DFID_SCHMIDT_TRIGGER
newfunc: DFID_SINE_SVF
newfunc: DFID_SLEW_RATE_LIMITER
newfunc: DFID_SQUARE
newfunc: DFID_SQUARE_LFO
newfunc: DFID_SUBTRACT
newfunc: DFID_SUB_DECODE_ADP4
newfunc: DFID_SUB_ENVELOPE
newfunc: DFID_SUB_SAMPLER_16_V1
newfunc: DFID_SUB_SAMPLER_ADP4_V1
newfunc: DFID_SVFILTER
newfunc: DFID_TAPOUTPUT
newfunc: DFID_TIMES_256
newfunc: DFID_TIMESPLUS
newfunc: DFID_TIMESPLUS_NOCLIP
newfunc: DFID_TIMESPLUS_UNSIGNED
newfunc: DFID_TRIANGLE
newfunc: DFID_TRIANGLE_LFO
newfunc: DFID_CHAOS_1D
newfunc: DFID_INPUT_ACCUM
newfunc: DFID_OUTPUT_ACCUM
newfunc: DFID_ADD_ACCUM
newfunc: DFID_MULTIPLY_ACCUM
newfunc: DFID_SUBTRACT_ACCUM
newfunc: DFID_SUBTRACT_FROM_ACCUM
." MEI Reserved 0 = " dup . cr
newfunc: DFID_RESERVED_MEI_0
newfunc: DFID_RESERVED_MEI_1
newfunc: DFID_RESERVED_MEI_2
newfunc: DFID_RESERVED_MEI_3
newfunc: DFID_RESERVED_MEI_4
newfunc: DFID_RESERVED_MEI_5
newfunc: DFID_RESERVED_MEI_6
newfunc: DFID_RESERVED_MEI_7
newfunc: DFID_RESERVED_MEI_8
newfunc: DFID_RESERVED_MEI_9
drop
