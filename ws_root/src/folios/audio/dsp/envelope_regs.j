\ @(#) envelope_regs.j 96/02/26 1.1
\
\ Envelope registers
\
\ Author: Phil Burk
\ Copyright 1993,4,5 3DO
\ All Rights Reserved


0 constant ENV_BASE_DATA_REG
ENV_BASE_DATA_REG
dup 1+ swap constant  ENV_REG_REQUEST
dup 1+ swap constant  ENV_REG_INCR
dup 1+ swap constant  ENV_REG_AMPLITUDE
dup 1+ swap constant  ENV_REG_PHASE
dup 1+ swap constant  ENV_REG_SOURCE
dup 1+ swap constant  ENV_REG_TARGET
dup 1+ swap constant  ENV_REG_CURRENT
dup 1+ swap constant  ENV_REG_OUTPUT
ENV_BASE_DATA_REG - constant ENV_NUM_DATA_REGS
