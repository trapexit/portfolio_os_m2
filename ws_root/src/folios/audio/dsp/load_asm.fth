\ @(#) load_asm.fth 96/03/07 1.4

include? task-log_to_file.fth log_to_file.fth
\ include? task-DSPP.j DSPP.j
include? task-dspp_asm.fth dspp_asm.fth
\ include dspp_map.j
include? task-function_ids.j function_ids.j

c" dsppasm.dic" save-forth
