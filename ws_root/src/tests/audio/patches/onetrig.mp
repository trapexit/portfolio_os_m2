; @(#) onetrig.mp 95/12/22 1.1
;
; Play with triggers. Use this with ta_trigger_recv to watch triggers arrive.

Instrument trig schmidt_trigger.dsp

Expose Trigger trig Trigger

Knob Test 1 Signed 0
Knob Level 1 Signed .1

Connect . Level 0 trig SetLevel 0
Connect . Level 0 trig ResetLevel 0
Connect . Test 0 trig Input 0
