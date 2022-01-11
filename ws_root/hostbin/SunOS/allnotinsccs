#!/bin/csh -f
# @(#)allnotinsccs.sh 95/02/27 1.1
#
# given a list of directories as args,
# find all files under them that do *not* have
# corresponding SCCS/s. files.
foreach t ($*)
  find $t -type f -print | grep -v '/SCCS/s\.' | xargs notinsccs
end
