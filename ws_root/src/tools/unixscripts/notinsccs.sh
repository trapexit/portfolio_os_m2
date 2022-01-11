#!/bin/csh -f
# @(#)notinsccs.sh 95/02/27 1.1
#
# given a list of files as args,
# echo the ones that do *not* have
# corresponding SCCS/s. files.
#
foreach p ($*)
  set h=$p:h t=$p:t
  if ( $h == $p ) set h=.
  if ( ! -f $h/SCCS/s.$t ) echo $p
end
