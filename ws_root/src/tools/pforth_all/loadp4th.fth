\ @(#) loadp4th.fth 96/03/20 1.7
\ Load various files needed by PForth
\
\ Author: Phil Burk
\ Copyright 1994 3DO, Phil Burk, Larry Polansky, Devid Rosenboom
\
\ The pForth software code is dedicated to the public domain,
\ and any third party may reproduce, distribute and modify
\ the pForth software code or any derivative works thereof
\ without any compensation or license.  The pForth software
\ code is provided on an "as is" basis without any warranty
\ of any kind, including, without limitation, the implied
\ warranties of merchantability and fitness for a particular
\ purpose and their equivalents under the laws of any jurisdiction.

include? forget  forget.fth
include? >number numberio.fth
include? task-misc1.fth   misc1.fth
include? case    case.fth
include? $=      strings.fth
include? (local) ansilocs.fth
include? {       locals.fth
include? fm/mod  math.fth
include? task-misc2.fth misc2.fth
include? catch   catch.fth
include? task-quit.fth quit.fth
include? [if]    condcomp.fth

\ load floating point support if basic support is in kernel
exists? F*
    [IF]  include? task-floats.fth floats.fth
    [THEN]
    
map
include? s@      member.fth
include? :struct c_struct.fth
include? smif{   smart_if.fth


