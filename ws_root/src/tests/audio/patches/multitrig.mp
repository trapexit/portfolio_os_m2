; @(#) multitrig.mp 95/12/20 1.1
;
; Play with triggers. Use this with ta_trigger_recv to watch triggers arrive.

Instrument trig1 schmidt_trigger.dsp
Instrument trig2 schmidt_trigger.dsp
Instrument trig3 schmidt_trigger.dsp
Instrument trig4 schmidt_trigger.dsp

Expose Trigger1 trig1 Trigger
Expose Trigger2 trig2 Trigger
Expose Trigger3 trig3 Trigger
Expose Trigger4 trig4 Trigger

Knob Test 1 Signed 0
Knob Level1 1 Signed .1
Knob Level2 1 Signed .2
Knob Level3 1 Signed .3
Knob Level4 1 Signed .4

Connect . Level1 0 trig1 SetLevel 0
Connect . Level1 0 trig1 ResetLevel 0
Connect . Level2 0 trig2 SetLevel 0
Connect . Level2 0 trig2 ResetLevel 0
Connect . Level3 0 trig3 SetLevel 0
Connect . Level3 0 trig3 ResetLevel 0
Connect . Level4 0 trig4 SetLevel 0
Connect . Level4 0 trig4 ResetLevel 0

Connect . Test 0 trig1 Input 0
Connect . Test 0 trig2 Input 0
Connect . Test 0 trig3 Input 0
Connect . Test 0 trig4 Input 0
