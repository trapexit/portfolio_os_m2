\ @(#) dspp_map.j 95/04/11 1.3
\ $Id: dspp_map.j,v 1.2 1994/05/12 00:01:00 peabody Exp $
anew task-dspp_map.j

\ Internal DSPP Variables, reserve $100-$110
\ DSPP variables
$ 0 constant EI_IfScaleOutput
$ 1 constant EI_OutputScalar

$ 100 constant I_Reserved0
$ 101 constant I_Reserved1
$ 102 constant I_Scratch	
$ 103 constant I_Clock
$ 104 constant I_Bench	
$ 105 constant I_FrameCount
$ 106 constant I_MixLeft         \ mixed before output to DAC
$ 107 constant I_MixRight

\ These must match dspp.h
$ 0300 constant EO_BenchMark
$ 0301 constant EO_MaxTicks
$ 0302 constant EO_FrameCount
