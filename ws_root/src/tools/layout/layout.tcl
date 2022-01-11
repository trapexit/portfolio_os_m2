# @(#) layout.tcl 96/09/11 1.12
#
# This file contains the TCL subroutines needed by the M2
# CD-ROM layout program.
#

echo Loading layout.tcl

proc diag {args} {
  global trace
  if {$trace != 0} then {
    eval {echo $args}
  }
}

set trace 0

if {0 == [catch {info library}]} then {
  if [file exists [info library]/init.tcl] then {
    diag Loading init.tcl
      source [info library]/init.tcl
      } else {
	diag There is no init.tcl file
	}
} else {
  diag No TCL library available
}

#
# The "include" proc is intended to be used to read .h #include files.
# It tosses everything other than simple #define commands;  when it
# finds a #define command it extracts the symbol name and expression
# and adds the symbol to the TCL global namespace.
#

proc include {filename} {
  if {0 == [catch {set file [open $filename "r"]}]} {
    echo #include $filename
    } else {
      if {0 == [catch {set file [open includes/$filename "r"]}]} {
	echo #include includes/$filename
	} else {
	  if {0 == [catch {set file [open :includes:$filename "r"]}]} {
	    echo #include :includes:$filename
	    } else {
	      error "Cannot find #include file $filename"
	      }
	}
    }
  for {set chars [gets $file theLine]} \
      {$chars >= 0} \
      {set chars [gets $file theLine]} {
     if {[regexp ^#define $theLine] && ![regexp \( $theLine] &&
	 ![regexp \+ $theLine]} then {
     	busy
       set varname [lindex $theLine 1]
       set varval [set varstring [lindex $theLine 2]]
       regsub -all ' $varstring {} varval
       if {"" != "$varval"} then {
         if {"$varval" == "0x80000000"} then {
           set varfixed -2147483648
         } else {
       	   set varfixed [expr {$varval}]
       	 }
         diag setting '$varname' to '$varval' as '$varfixed'
         eval { global $varname ; set $varname $varfixed }
#         echo Testing numericness of $varfixed
#         diag [expr {$varfixed + 0}]
       }
     }
   }
}

include "filesystem.h"
include "discdata.h"


#
# The following magic constants are not necessarily correct for
# production-system discs.  DISC_TOTAL_BLOCKS should be set to the
# desired number of data blocks to be placed in the CD-ROM image.
#
# Normally, DISC_LABEL_AVATAR_DELTA should not be set here;  the
# value from the include file "filesystem.h" is the right one to use
# for real Opera discs.  The delta can be set to a small number here
# when tiny images are being created in test mode;  however, the real
# Opera filesystem won't be able to find any but the first!
#

set maxavatars [expr {$FILE_HIGHEST_AVATAR + 1}]

#
# Register the avatar limit with the C driver so that space can be
# reserved appropriately.
#

avatarlimit $maxavatars

#
# Selective set/increment
#

proc bump {name args} {
  upvar name x
  if {[info exists x] == 0} then {
    set x 0
  }
  if {$args == ""} then {
    echo incr $name
    incr x
    echo $name is now $x
  } else {
    incr x $args
  }
}

proc base10 {args} {
  echo Base10 $numbers
  foreach foo $numbers {
    echo try adding 0 to $foo
    lappend result [expr {0+$foo}]
  }
  return $result
}

#
# The "addto" proc adds a file-object to a directory-object.
#

proc addto {directory file} {
  global dircontents
  global parent
  if [info exists dircontents($directory)] then {
    set dircontents($directory) [concat $dircontents($directory) $file]
  } else {
    set dircontents($directory) $file
  }
  set parent($file) $directory
}

#
# The "pathname" proc takes a file object and returns the full Opera
# pathname to the file.
#

proc pathname {file} {
  global parent
  global root
  if {[info exists parent($file)]} then {
    if {$parent($file) != $root} then {
      return "[pathname $parent($file)]/[nameof $file]"
    } else {
      return [nameof $file]
    }
  } else {
    return [nameof $file]
  }
}

#
# The "sortdir" proc sorts the file-objects in a directory-object
# into ascending name order.
#

proc sortdir {directory} {
  global dircontents
  set contents $dircontents($directory)
  set len [llength $contents]
  if {$len <= 1} then {return $contents}
  set $contents [eval sortnamesof $contents]
  set dircontents($directory) $contents
}

#
# The "mapnames" proc takes a directory-object containing one or
# more file-objects, and sets up the pathname-to-file-object mappings.
# It runs recursively on any subdirectories.

proc mapnames {directory} {
  global pathnametofile
  global dircontents
  set pathnametofile([pathname $directory]) $directory
  diag Mapping $directory which is [pathname $directory]
  set contents $dircontents($directory)
  busy
  foreach file $contents {
    if {[whatis $file] == "directory"} then {
      mapnames $file
    } else {
      set pathnametofile([pathname $file]) $file
    }
  }
}

#
# The "builddirectory" proc takes a directory-object containing one or
# more file-objects, and actually formats the directory.
# It runs recursively on any subdirectories.
#
# The directory is formatted into a buffer in memory.  It isn't
# actually written to disc at this time.
#

proc builddirectory {directory} {
  global dircontents
  global DIRECTORY_LAST_IN_DIR
  global DIRECTORY_LAST_IN_BLOCK
  global blocks blocksize
  global directoryavatars
  global disc
  global highwater
#  echo Sorting contents of [nameof $directory]
  sortdir $directory
  set contents $dircontents($directory)
  set needed 0
  foreach file $contents {
    if {[whatis $file] == "directory"} then {
      echo Constructing [pathname $file]
      set dirbuffer($file) [builddirectory $file]
      setfile $file -bytecount [bufferused $dirbuffer($file)]
      diag Directory [nameof $file] byte count is [getfile $file -bytecount]
    }
  }
  echo Formatting [pathname $directory]
  foreach file $contents {
    if {[whatis $file] == "directory"} then {
      set filebuffer $dirbuffer($file)
    } else {
      set filebuffer NONE
    }
    set filebytecount [getfile $file -bytecount]
    set fileblocksize [getfile $file -blocksize]
    set fileblockcount [expr {($filebytecount+$fileblocksize-1)/$fileblocksize}]
    diag File [nameof $file] byte count $filebytecount blocksize $fileblocksize blockcount $fileblockcount
    if {$fileblockcount == 0} then {set fileblockcount 1}
    setfile $file -blockcount $fileblockcount
    if {0 == [getfile $file -numavatars]} then {
      diag Doing default avatar assignment for [nameof $file]
      if {[whatis $file] == "directory"} then {
        assignavatars $file $directoryavatars
      } else {
        assignavatars $file 1 $highwater
      }
    }
    if {$filebuffer == "NONE"} then {
      diag Writing [nameof $file] from disk
      writefile $disc $file
    } else {
      diag Writing [nameof $file] from buffer $filebuffer
      writefile $disc $file $filebuffer
    }
    set lastfile $file
  }
  set directorypagesize [getfile $directory -blocksize]
  set buffer [makebuffer $directorypagesize]
  set remaining $directorypagesize
  set totalsize $directorypagesize
  set firstinblock 1
  set blocknum 0
  diag Existing block flags are [getfile $file -flags]
  diag Setting flags to [getfile $file -flags] | $DIRECTORY_LAST_IN_DIR | $DIRECTORY_LAST_IN_BLOCK
  setfile $file -flags [expr {[getfile $file -flags] | $DIRECTORY_LAST_IN_DIR | $DIRECTORY_LAST_IN_BLOCK}]
  set prev NONE
  diag Stuffing directory
  foreach file [concat $contents END] {
    if {$firstinblock} then {
      diag Initializing block $blocknum
      set firstinblock 0
# to stuff... next block num of -1, prev block num, flag (zeroes),
# first free byte (leave zero for now), first entry offset (fixed at 20)
      set remaining [expr {$remaining - 20}]
      set blockstartbyte [bufferused $buffer]
      stuffzeroes $buffer 20
      stufflongword $buffer $blockstartbyte -1
      stufflongword $buffer [expr {$blockstartbyte + 4}] [expr {$blocknum - 1}]
      stufflongword $buffer [expr {$blockstartbyte + 16}] 20
    }
    diag Stuffing file $file, prev is now $prev, bytes left $remaining
    if {$file == {END}} then {
      diag ## last file
      set need $directorypagesize
      diag Setting flags for $prev: old flags are [getfile $prev -flags]
      diag Adding flags $DIRECTORY_LAST_IN_DIR | $DIRECTORY_LAST_IN_BLOCK
      setfile $prev -flags [expr {[getfile $prev -flags] | $DIRECTORY_LAST_IN_DIR | $DIRECTORY_LAST_IN_BLOCK}]
    } else {
      set need [bufferused $file]
    }
    diag Need $need bytes
    if {$remaining < $need} then {
    diag ## End of block $blocknum
# note that this doesn't handle empty directories!
      if {$file != {END}} then {
        stufflongword $buffer $blockstartbyte [expr {$blocknum + 1}]
      }
      stufflongword $buffer [expr {$blockstartbyte + 12}] [expr {[bufferused $buffer] - $blockstartbyte }]
      if {$prev != {NONE}} then {
        setfile $prev -flags [expr {[getfile $prev -flags] | $DIRECTORY_LAST_IN_BLOCK}]
        stuffbytes $buffer $prev [bufferused $prev]
        stufflongword $buffer [expr {$blockstartbyte + 12}] [expr {[bufferused $buffer] - $blockstartbyte }]
        stuffzeroes $buffer $remaining
        set remaining 0
      }
      if {$file != {END}} then {
        diag Expanding buffer
        set totalsize [expr {$totalsize + $directorypagesize}]
        reallocbuffer $buffer $totalsize
        set remaining [expr {$totalsize - [bufferused $buffer]}]
        set firstinblock 1
        set blocknum [expr {$blocknum + 1}]
        diag $remaining bytes remaining in buffer
      }
    } else {
      if {$prev != {NONE}} then {
        stuffbytes $buffer $prev [bufferused $prev]
	}
    }
    set prev $file
    set remaining [expr {$remaining - $need}]
  }
  setfile $directory -bytecount [bufferused $buffer]
  return $buffer
}

#
# The "takefile" proc accepts the pathname (Unix or Mac) to a file
# which is to be included in an Opera filesystem.  It allocates,
# fills in, and returns a file-object which describes this file,
# and "remembers" the path needed to access the file.

proc takefile {unixpath {name ""} {type ""}} {
  global pathto
  global blocksize
  global environment
  if {![file exists $unixpath]} {
    error "File $unixpath does not exist"
  }
  set unixname [file tail $unixpath]
  set bytecount [file size $unixpath]
  set dirloc [file dirname $unixpath]
  set extension [file extension $unixname]
  set rootname [file rootname $unixname]
  if {$name == ""} {
    set name $unixname
  }
  if {$type == ""} {
    set type [string trim $extension .]
  }
  set thefile [makefile $name]
  set pathto($thefile) $unixpath
  setfile $thefile -type $type
  setfile $thefile -bytecount $bytecount
  setfile $thefile -blocksize $blocksize
  diag "Working with file $unixpath"
  if {$environment == "Mac"} then {
    set location "$dirloc:#$unixname"
  } else {
    set location "$dirloc/#$unixname"
  }
  diag Location-file would be in $location
  if {[file exists $location]} {
     set locfile [open $location "r"]
     set locstring [read $locfile nonewline]
     close $locfile
     diag "Found location file, contents is $locstring"
     eval setavatars $thefile $locstring
     if {0 != [catch {seizeavatars $thefile} whoops]} {
       error "Unable to honor avatar request for $unixpath, $whoops"
     }
  }
  return $thefile
}

#
# The "takedir" proc accepts the pathname (Unix or Mac), scans the
# directory looking for eligible files and subdirectories, creates
# file-objects and directory-objects for such eligible thingies, and
# adds these file-objects and directory-objects to a specified
# directory-object.
#

proc takedir {unixpath directory} {
  global pathto
  global environment
  global dircontents
  echo Processing $unixpath
  if {$environment == "Mac"} then {
    set contents [glob $unixpath:*]
  } else {
    set contents [glob $unixpath/*]
  }
  set contents [lsort $contents]
  foreach file $contents {
    busy
    diag Processing $file
    if {[regexp ~ $file] || [regexp % $file] || [regexp # $file]} then {
      continue
    }
    if {[file isdirectory $file]} then {
#       echo Recursive-take on $file
#       regsub /$ $file {} dirname
       set dirname [file tail $file]
       set it [makedirectory $dirname]
       if {[catch {takedir $file $it} whoops] == 0} then {
         addto $directory $it
       } else {
         error "Couldn't take $file: $whoops"
       }
    } else {
       diag Taking $file
       set it [takefile $file]
       addto $directory $it
    }
  }
  if {[info exists dircontents($directory)]} {
    return $directory
  } else {
    error "Empty directory: $unixpath"
  }
}

#
# The "makefile" proc creates and initializes a file-object.
#

proc makefile {name} {
  global itemtype
  global blocksize
  global FILE_IS_READONLY
  set it [newfile $name]
  set itemtype($it) "file"
  setfile $it -uniqueidentifier [unique]
  setfile $it -flags $FILE_IS_READONLY
  setfile $it -version 0
  setfile $it -blocksize $blocksize
  return $it
}

#
# The "makedirectory" proc creates and initializes a directory-object.
#

proc makedirectory {name} {
  global itemtype
  global FILE_IS_READONLY
  global FILE_IS_FOR_FILESYSTEM
  global FILE_IS_DIRECTORY
  global FILE_TYPE_DIRECTORY
  global directorysize
  set it [newfile $name]
  set itemtype($it) "directory"
  setfile $it -uniqueidentifier [unique]
  setfile $it -flags [expr {$FILE_IS_READONLY | $FILE_IS_FOR_FILESYSTEM |
                            $FILE_IS_DIRECTORY}]
  setfile $it -type $FILE_TYPE_DIRECTORY
  setfile $it -version 0
  setfile $it -blocksize $directorysize
  return $it
}

#
# The "makebuffer" proc creates and initializes a buffer-object of
# a specified size.
#

proc makebuffer {{size 0}} {
  global itemtype
  set it [newbuffer $size]
  set itemtype($it) "buffer"
  return $it
}

#
# The "whatis" proc returns the type (file, directory, buffer) of a
# specified object.

proc whatis {it} {
  global itemtype
  return $itemtype($it)
}

#
# The "unique" proc returns a unique random-number cookie.
#

proc unique {} {
  global uniques
  set it [random]
  while {0 == [catch {set discard $uniques($it)}]} {
    set it [random]
  }
  set uniques($it) 1
  return $it
}

#
# The "opendisc" proc deletes any existing cdrom.image file, and
# opens a new one for writing.
#

proc opendisc {{filename "cdrom.image"} {blocks 0}} {
  global freepool
  global DISC_TOTAL_BLOCKS
  global blocksize
  global preinitialize environment elephantstomp
  global workbuffersize
  if {$blocks == 0} then {
    set blocks $DISC_TOTAL_BLOCKS
  }
  echo Disc image $filename will have $blocks blocks
  set freepool [list [list 0 $blocks]]
  deletefile $filename
  set thefile [open $filename w+]
  set bytecount [expr {$blocks * $blocksize}]
  echo Allocating space: $blocks blocks of $blocksize bytes
  allocphysical $thefile $bytecount
  echo Setting EOF to end of file
  seteof $thefile $bytecount
  echo Setting file type
  settype $thefile IMAG qtop
  if {$preinitialize} then {
    echo Preinitializing image
    set zerosize $workbuffersize
    set buffer [makebuffer $zerosize]
    stuffzeroes $buffer $zerosize $elephantstomp
#    seek $thefile 0
    set offset 0
    while {$bytecount > 0} {
      if {$bytecount < $zerosize} then {
        set zerosize $bytecount
        killbuffer $buffer
        set buffer [makebuffer $zerosize]
        stuffzeroes $buffer $zerosize $elephantstomp
	}
      writebuffer $thefile $offset $buffer
      busy
      set bytecount [expr {$bytecount - $zerosize}]
      set offset [expr {$offset + $zerosize}]
    }
    killbuffer $buffer
  } else {
    if {$environment == "Mac"} then {
      echo "WARNING!  Filesystem image is not being preinitialized!"
    }
  }
  return $thefile
}

#
# The "seizeavatars" proc is called to reserve the disk space for
# or more avatars of a file-objects.  It simply calls "seizeavatar"
# for each of the file-object's avatars, passing the starting
# block number and block count of the avatar.
#

proc seizeavatars {file} {
  global blocksize
  if {0 == [getfile $file -numavatars]} then {return}
  echo Seizing avatars for [nameof $file]
  set avatarlist [getfile $file -avatars]
  set bytecount [getfile $file -bytecount]
  set fileblocksize [getfile $file -blocksize]
  set blockcount [expr {($bytecount+$fileblocksize-1)/$fileblocksize}]
  set discblockcount [expr {(($blockcount*$fileblocksize)+$blocksize-1)/
                            $blocksize}]
  diag "Byte count $bytecount, blocksize $fileblocksize, blockcount $blockcount"
  diag "Disc block size $blocksize"
  foreach avatar $avatarlist {
    diag "  seize $discblockcount blocks starting at $avatar"
    seize $avatar $discblockcount
  }
}

#
# The "seize" procedure seizes a specific range of blocks (defined
# by starting block number and count) from the free-block pool, and
# reports an error if this cannot be done for any reason.
#
# TBD:  this proc, and the "assign" proc which follows it, really
# must be rewritten.  Currently, the free-block pool is kept as a
# list containing block numbers and free-block run lengths.  It takes
# far too much CPU time to seize lots of avatars, and things will only
# get worse when interleaved files are supported.  Rewrite the whole
# mess to use a free-block bitmap and fast masking techniques, and do
# it in C fercryinoutloud!
#

proc seize {start count} {
  global freepool
  set probe 0
  diag Seize $start $count
  set after [expr {$start+$count}]
  foreach range $freepool {
    set freestart [lindex $range 0]
    set freecount [lindex $range 1]
   diag Checking block from $freestart size $freecount
    set freenot [expr {$freestart+$freecount}]
    if {$start >= $freestart && $start < $freenot} then {
      if {$after > $freenot} then {
        error [concat Can't allocate $count at $start, already in use]
      }
     diag This block applies
      if {$start == $freestart} then {
        if {$after == $freenot} then {
         diag Exact match
	  set freepool [lreplace $freepool $probe $probe]
        } else {
         diag Leading subsegment
          set freepool [lreplace $freepool $probe $probe \
                         [list $after [expr {$freecount-$count}]]]
        }
      } else {
        if {$after == $freenot} then {
         diag Trailing subsegment
          set freepool [lreplace $freepool $probe $probe \
                         [list $freestart [expr {$freecount-$count}]]]
        } else {
         diag In the middle
          set freepool [lreplace $freepool $probe $probe \
                          [list $freestart [expr {$start - $freestart}]] \
                          [list $after [expr {$freenot - $after}]]]
        }
      }
      diag Seize done, free pool is $freepool
      return [list $start $count]
    } else {
      if {$freestart > $start} then {
        error [concat Can't allocate $count at $start, in use]
      }
    }
    incr probe
  }
  error [concat Can't allocate $count at $start, out of range or in use]
}

#
# The "assign" procedure chooses a specific range of blocks (defined
# by starting block number and count) from the free-block pool, and
# reports an error if this cannot be done for any reason.  The range of
# blocks actually assigned will be at, or after, the starting point
# specified by the caller.
#
proc assign {start count} {
  global freepool
  set probe 0
  set after [expr {$start+$count}]
  foreach range $freepool {
    set freestart [lindex $range 0]
    set freecount [lindex $range 1]
   diag Checking block from $freestart size $freecount
    set freenot [expr {$freestart+$freecount}]
    if {$freenot >= $after && $freecount >= $count} then {
      if {$start < $freestart} then {
         set start $freestart
         set after [expr {$start+$count}]
      }
     diag This block applies
      if {$start == $freestart} then {
        if {$after == $freenot} then {
         diag Exact match
	  set freepool [lreplace $freepool $probe $probe]
        } else {
         diag Leading subsegment
          set freepool [lreplace $freepool $probe $probe \
                         [list $after [expr {$freecount-$count}]]]
        }
      } else {
        if {$after == $freenot} then {
         diag Trailing subsegment
          set freepool [lreplace $freepool $probe $probe \
                         [list $freestart [expr {$freecount-$count}]]]
        } else {
         diag In the middle
          set freepool [lreplace $freepool $probe $probe \
                          [list $freestart [expr {$start - $freestart}]] \
                          [list $after [expr {$freenot - $after}]]]
        }
      }
      diag Assign done, free pool is $freepool
      return [list $start $count]
    }
    incr probe
  }
  error "Can't allocate $count at $start, out of range or in use"
}

#
# The "preassign" procedure looks for a specific range of blocks (defined
# by starting block number and count) from the free-block pool, and
# reports an error if this cannot be done for any reason.  The range of
# blocks actually assigned will be at, or after, the starting point
# specified by the caller.  The blocks are not actually seized from
# the list.
#
proc preassign {start count} {
  global freepool
  set probe 0
  set after [expr {$start+$count}]
#  echo Preassign:  seek for $count from $start, end $after
  foreach range $freepool {
    set freestart [lindex $range 0]
    set freecount [lindex $range 1]
    set freenot [expr {$freestart+$freecount}]
#    echo "  Free area begins at $freestart, ends at $freenot"
    if {$freestart >= $start} then {
      if {$freecount >= $count} then {
#         echo Accepted block beginning at $freestart
         return $freestart
      }
    } else {
      if {$freenot >= $after} then {
#        echo Accepted partial block beginning at $start
        return $start
      }
    }
  }
  error "Can't allocate $count at $start, out of range or in use"
}

proc assignavatars {file {avatarswanted 1} {avatarbase 0}} {
  global blocks blocksize
  set filebytecount [getfile $file -bytecount]
  set fileblocksize [getfile $file -blocksize]
  set fileblockcount [expr {($filebytecount+$fileblocksize-1)/$fileblocksize}]
  set discblockcount [expr {($fileblockcount*$fileblocksize) /  $blocksize}]
  set avatardelta [expr {$blocks/$avatarswanted}]
  set avatarlist {}
  diag Want $avatarswanted avatars of $discblockcount each for [nameof $file]
  for {set avatarsgotten 0} \
      {$avatarswanted > 0} \
      {incr avatarswanted -1} \
  {
   if {[catch {set location [assign $avatarbase $discblockcount]}] == 0} then {
     set avatarloc [lindex $location 0]
     set avatarlist [concat $avatarlist $avatarloc]
     incr avatarsgotten
   }
   incr avatarbase $avatardelta
  }
  if {$avatarsgotten == 0} then {
    error "Could not assign avatar(s) for [nameof $file]"
  }
  eval setavatars $file $avatarlist
  diag Avatars for [getfile $file -name] are $avatarlist
}

#
# The "writefile" proc writes the contents of a file-object to disc.
# If a buffer-object is passed in, the contents of the buffer is
# assumed to be what the caller wants written.  If no buffer object
# is passed in, a temporary buffer is allocated and filled from the file.
# The buffer data is written to each avatar in the file (no provision
# yet for interleaved files, and there probably never will be... this
# feature is not well supported.
#

proc writeavatars {file} {
  global disc
  global blocksize
  global workbuffersize
  global pathto
  global parent
  global FILE_HAS_VALID_VERSION
  global FILE_CONTAINS_VERSIONED
  set chunksize $workbuffersize
  set bytecount [getfile $file -bytecount]
  set thefile [open $pathto($file) "r"]
  set avatarlist [getfile $file -avatars]
  set isfirstbuffer 1
  foreach avatar $avatarlist {
    diag Writing to avatar at block $avatar
    set offset [expr {$avatar*$blocksize}]
    set bytesleft $bytecount
    set readoffset 0
    while {$bytesleft > 0} {
    	if {$bytesleft > $chunksize} then {
    		set thischunk $chunksize
    	} else {
    		set thischunk $bytesleft
    	}
    	diag Slurping $thischunk bytes
	busy
	set thebuffer [makebuffer $thischunk]
    	readbuffer $thefile $readoffset $thebuffer
	if {$isfirstbuffer} {
	  set hasversion [extractversion $thebuffer]
          diag ELF info: $hasversion
          set isfirstbuffer 0
	  if {$hasversion != "none"} {
	    setfile $file -verrev $hasversion
	    setfile $file -flags [expr {[getfile $file -flags] |
                              $FILE_HAS_VALID_VERSION}]
            set myparent $parent($file)
	    setfile $myparent -flags [expr {[getfile $myparent -flags] |
                              $FILE_CONTAINS_VERSIONED}]
	  }
	}
	writebuffer $disc $offset $thebuffer
	killbuffer $thebuffer
	set bytesleft [expr {$bytesleft - $thischunk}]
	set offset [expr {$offset + $thischunk}]
	set readoffset [expr {$readoffset + $thischunk}]
   	}
  }
  close $thefile
}
proc writefile {disc file {buffer ""}} {
  global blocksize
  global pathto
  global workbuffersize
  global domapping outputmapfile
  global parent
  global FILE_HAS_VALID_VERSION
  global FILE_CONTAINS_VERSIONED
  set chunksize $workbuffersize

  set avatarlist [getfile $file -avatars]
  if {$domapping} then {
    puts $outputmapfile "\"[pathname $file]\" [getfile $file -blockcount] $avatarlist"
  }
  if {$buffer != ""} then {
    diag Writing [pathname $file]
    set hasversion [extractversion $buffer]
    diag ELF info: $hasversion
    diag Buffer has [bufferused $buffer] bytes
    if {$hasversion != "none"} {
      setfile $file -verrev $hasversion
      setfile $file -flags [expr {[getfile $file -flags] |
                                  $FILE_HAS_VALID_VERSION}]
      set myparent $parent($file)
      setfile $myparent -flags [expr {[getfile $myparent -flags] |
                                $FILE_CONTAINS_VERSIONED}]
    }
    foreach avatar $avatarlist {
      diag Writing to avatar at block $avatar
      busy
#      seek $disc [expr {$avatar*$blocksize}] start
      writebuffer $disc [expr {$avatar*$blocksize}] $buffer
    }
    return $file
    }
  if {$pathto($file) == ""} then {
    return
  }
  diag Opening $pathto($file)
  diag Writing file [nameof $file]
  if {[catch {writeavatars $file} whoops] != 0} then {
     set pathname [pathname $file]
     error [concat Error writing $pathname - $whoops]
  }
}

proc dupblocks {count from to} {
  global workbuffersize
  global disc
  global blocksize
  set chunksize $workbuffersize
  diag Copying $count blocks of $blocksize bytes from $from to $to
  set bytesleft [expr {$count * $blocksize}]
  set readoffset [expr {$from * $blocksize}]
  set writeoffset [expr {$to * $blocksize}]
  while {$bytesleft > 0} {
    if {$bytesleft > $chunksize} then {
      set thischunk $chunksize
    } else {
      set thischunk $bytesleft
    }
    diag Slurping $thischunk bytes
    set thebuffer [makebuffer $thischunk]
    diag Read from byte $readoffset
    readbuffer $disc $readoffset $thebuffer
    diag Write to byte $writeoffset
    writebuffer $disc $writeoffset $thebuffer
    killbuffer $thebuffer
    set bytesleft [expr {$bytesleft - $thischunk}]
    incr readoffset $thischunk
    incr writeoffset $thischunk
  }
}

proc firstavatar {file} {
  set avatars [getfile $file -avatars]
  regsub " .*$" $avatars {} first
  if {[info exists first]} then {
    return $first
  } else {
    return $avatars
  }
}

proc writecatapult {} {
  global catapultfile catapultoffset catapultcount catapultplace catapultindex
  global catapultplace
  global catapulting
  global catapultblocksused
  global catapult
  global blocksize
  global pathto pathnametofile
  global catapultentriesperpage
  global disc
  global FILE_TYPE_CATAPULT
  echo ""
  echo Constructing catapult file
  set remains 0
  set entries 0
  set catapultcursor 0
  set previndexlocation 0
  set catapultloc [firstavatar $catapult]
  set indexbuffer [makebuffer $blocksize]
  stuffzeroes $indexbuffer $blocksize
  for {set i 0} {$i <= $catapultindex} {incr i} {
    busy
    if {$remains == 0} then {
      if {[info exists indexlocation] != 0} then {
        echo "  Writing catapult index page at offset" $indexlocation
        echo "    contains" $entries "entries, next page is" $catapultcursor
        stufflongword $indexbuffer 0  0
        stufflongword $indexbuffer 4  $FILE_TYPE_CATAPULT
        stufflongword $indexbuffer 8  $entries
        stufflongword $indexbuffer 12 $catapultcursor
	writebuffer $disc [expr {$blocksize * ($catapultloc+$indexlocation)}] $indexbuffer
        set previndexlocation $indexlocation
      }
      echo "  Placing catapult index page at offset" $catapultcursor
      set indexlocation $catapultcursor
      incr catapultcursor
      set remains $catapultentriesperpage
      set entries 0
      for {set bufferindex 0} {$bufferindex < $blocksize} {incr bufferindex 4} {
        stufflongword $indexbuffer $bufferindex 0
      }
      set bufferindex 16
    }
    echo "  " $catapultfile($i), start at offset $catapultoffset($i) for $catapultcount($i) blocks at $catapultcursor
    set file $pathnametofile($catapultfile($i))
    set fileloc [firstavatar $file]
    stufflongword $indexbuffer $bufferindex [getfile $file -uniqueidentifier]
    incr bufferindex 4
    stufflongword $indexbuffer $bufferindex $catapultoffset($i)
    incr bufferindex 4
    stufflongword $indexbuffer $bufferindex $catapultcount($i)
    incr bufferindex 4
    stufflongword $indexbuffer $bufferindex $catapultcursor
    incr bufferindex 4
    diag File's first avatar is at $fileloc
    diag File's index is at $catapultoffset($i)
    dupblocks $catapultcount($i) \
         [expr {$fileloc + $catapultoffset($i)}] \
         [expr {$catapultloc + $catapultcursor}]
    incr catapultcursor $catapultcount($i)
    incr entries
  }
  echo "  Writing catapult index page at offset" $indexlocation
  echo "    contains" $entries "entries"
  stufflongword $indexbuffer 0  0
  stufflongword $indexbuffer 4  $FILE_TYPE_CATAPULT
  stufflongword $indexbuffer 8  $entries
  stufflongword $indexbuffer 12 -1
  writebuffer $disc [expr {$blocksize * ($catapultloc+$indexlocation)}] $indexbuffer
  echo Catapult file complete
  echo ""
}

proc setoldavatars {oldpath numblocks args} {
  global oldblocks
  global oldavatars
  global oldblocktofilemap
  global totaloldblocks
  set oldblocks($oldpath) $numblocks
  set oldavatars($oldpath) $args
  foreach avatar $args {
    set oldblocktofilemap($avatar) $oldpath
  }
  diag File $oldpath has $numblocks blocks, avatars at $args
  incr totaloldblocks $numblocks
}

proc sortoldblocks {} {
  global oldblocktofilemap
  global oldblockbase
  echo "Building fast file-location lookup table"
  set oldblockbase [makefastlookup [array size oldblocktofilemap]]
  set searchid [array startsearch oldblocktofilemap]
  while {[set next [array nextelement oldblocktofilemap $searchid]] != ""} {
    addfastlookup $oldblockbase $next
  }
  array donesearch oldblocktofilemap $searchid
  echo "Sorting fast file-location lookup table"
  sortfastlookup $oldblockbase
}

proc identifyfile {block} {
  global oldblocktofilemap
  global oldblocks
  global oldblockbase
  set base [getfastlookup $block]
  if [info exists oldblocktofilemap($base)] then {
    return [list $cachefile [expr {$block-$base}]]
  }
  error "No file located at $base!"
}

proc initcatapult {} {
  global catapultindex catapulting nocatapult catapultblocksused
  set catapultblocksused 0
  set catapultindex -1
  set catapulting 1
  set foo "Disc label"
  set nocatapult($foo) 1
  set foo "Filesystem root"
  set nocatapult($foo) 1
}

proc clipcatapult {} {
  global catapultfile catapultoffset catapultcount catapultplace catapultindex
  global catapultentries catapulting catapultrunlimit
  if {$catapultindex >= 0} then {
    if {$catapultcount($catapultindex) > $catapultrunlimit} then {
      echo Catapult of $catapultfile($catapultindex), offset $catapultoffset($catapultindex) count $catapultcount($catapultindex) is too big
      incr catapultindex -1
    }
  }
}

proc seedcatapult {path} {
  global seeded
  global oldblocks
  global file
  global pathnametofile
  busy
  diag "Seeding catapult entry for $path"
  set file $pathnametofile($path)
  set blocks [getfile $file -blockcount]
  if {$blocks == 0} then {
    diag "Existing $path is of size zero"
    set blocks $oldblocks($path)
    diag "Using size of $blocks from old image"
  }
  addcatapult $path 0 $blocks
  set seeded($path) 1
}

# proc preseedcatapult {} {
# }

proc addcatapult {file offset count} {
  global catapultfile catapultoffset catapultcount catapultplace catapultindex
  global catapultentries catapulting
  global nocatapult
  global seeded
  global catapultblocks catapultblocksused
  if {$catapulting} then {
    if {[info exists nocatapult($file)] == 1} then {
      echo Skip catapult for $file, not eligible
      return
    }
    if {[info exists seeded($file)] == 1} then {
      echo Skip catapult for $file, preseeded
      return
    }
    clipcatapult
    if {$catapultblocksused + $count > $catapultblocks} then {
      echo Catapult block limit exceeded, catapulting ends
      set catapulting 0
      return
    }
    incr catapultindex
    if {$catapultindex >= $catapultentries} then {
      echo Catapult entry count exceeded, catapulting ends
      set catapulting 0
      incr catapultindex -1
    } else {
      echo Add catapult($catapultindex) for $file, offset $offset count $count
      set catapultfile($catapultindex) $file
      set catapultoffset($catapultindex) $offset
      set catapultcount($catapultindex) $count
    }
  }
}

proc recordaccess {block numblocks delay time start readlen foffset} {
  global sleeplimit
  global blockspersecond
  global clusters
  global clusterstarttime clusterendtime
  global seeksincluster seekstofile clusterusesfile
  global catapultfile catapultoffset catapultcount catapultplace catapultindex
  global catapulting
  if {$numblocks < "0" || $numblocks > "A"} then {
    return
  }
  upvar endtime endtime
  upvar prevfile prevfile
  upvar nextblockinfile nextblockinfile
  busy
  set file [identifyfile $block]
  set blockoffset [lindex $file 1]
  set file [lindex $file 0]
  if {$file == ""} then {
    error "Time $start read $numblocks from UNKNOWN FILE at $block"
    return
  }
  diag Time $start read $numblocks from $file at $block
  set blocksread [expr {$readlen/2048}]
  diag blocks read is $blocksread
  if {$file == $prevfile} then {
    diag Same file as previous access, next available is $nextblockinfile, offset is $blockoffset
    if {$blockoffset == $nextblockinfile} then {
      diag Sequential access
      if {$catapulting} then {
        incr catapultcount($catapultindex) $blocksread
        diag Extend catapult of $catapultfile($catapultindex) offset $catapultoffset($catapultindex) to $catapultcount($catapultindex) blocks
      }
    } else {
      diag Nonsequential access
      incr seeksincluster($clusters)
      diag seeksincluster($clusters) is now $seeksincluster($clusters)
      addcatapult $file $blockoffset $blocksread
    }
    set nextblockinfile [expr {$blockoffset+$blocksread}]
    diag Next block expected is $nextblockinfile
    return
  }
  addcatapult $file $blockoffset $blocksread
  set prevfile $file
  set nextblockinfile [expr {$blockoffset+$blocksread}]
  if {$start > $endtime + $sleeplimit} then {
#    echo Cluster break due to inactivity!
     set clusterendtime($clusters) $start
    incr clusters
#    echo Creating cluster $clusters
    set seeksincluster($clusters) 1
    set clusterstarttime($clusters) $start
    set clusterendtime($clusters) 999999999
  } else {
    incr seeksincluster($clusters)
  }
  set clusterusesfile($clusters,$file) 1
  if {[info exists seekstofile($file,$clusters)]} then {
    incr seekstofile($file,$clusters)
  } else {
    set seekstofile($file,$clusters) 1
  }
#  echo seekstofile($file,$clusters) is now $seekstofile($file,$clusters)
  set endtime $start
  incr endtime [expr {$delay * 60 / 1000}]
  incr endtime [expr {$blocksread * 60 / $blockspersecond}]
  return
}

proc flushdeferredfiles {cluster defer} {
  global fileavatars
  global clusterused clusterlocation
  global oldblocks
  foreach file $defer {
    set abs [expr {$clusterlocation($cluster)+$clusterused($cluster)}]
    echo "  $file offset $clusterused($cluster) block $abs"
    lappend fileavatars($file) [format "%012o" $abs]
    incr clusterused($cluster) $oldblocks($file)
  }
}

proc nailaccess {block numblocks delay time start readlen foffset} {
  global clusters
  global clusterstarttime clusterendtime clusterlocation clusterused
  global placefileincluster
  global fileavatars oldblocks
  global itemtype
  global pathnametofile
  global deferbig
  upvar prevfile prevfile
  upvar cluster cluster
  upvar defer defer
  if {$numblocks < "0" || $numblocks > "A"} then {
    return
  }
  busy
  set file [lindex [identifyfile $block] 0]
  if {$file == $prevfile} then {
    return
  }
#  echo Read from $file at time $start
  set prevfile $file
  set thefile $pathnametofile($file)
  while {$start >= $clusterendtime($cluster)} {
 #   echo Cluster $cluster ended at $clusterendtime($cluster)
    flushdeferredfiles $cluster $defer
    incr cluster
    echo ""
    echo Cluster $cluster:
    set defer {}
  }
  if {$itemtype($thefile) == "directory" ||
      [getfile $thefile -numavatars] > 0} then {
    return
  }
  if {[info exists placefileincluster($file,$cluster)] == 0} then {
    return
  }
  unset placefileincluster($file,$cluster)
  if {$oldblocks($file) >= $deferbig} then {
    echo "  $file deferred"
    lappend defer $file
  } else {
    set abs [expr {$clusterlocation($cluster)+$clusterused($cluster)}]
    echo "  $file offset $clusterused($cluster) block $abs"
    lappend fileavatars($file) [format "%012o" $abs]
    incr clusterused($cluster) $oldblocks($file)
  }
}

proc nailavatars {} {
  global fileavatars
  global pathnametofile
  global oldblocks
  set searchid [array startsearch fileavatars]
  while {[set next [array nextelement fileavatars $searchid]] != ""} {
    busy
    set fileid $pathnametofile($next)
    set avlist [lsort $fileavatars($next)]
    set blocks $oldblocks($next)
#    echo Grab $next, $blocks blocks at $avlist
    foreach avatar $avlist {
      set location [expr {0+$avatar}]
#      echo Grab $blocks at $location
      seize $location $blocks
    }
#    echo Setting avatars to $avlist
    eval setavatars $fileid $avlist
  }
  array donesearch fileavatars $searchid
}

proc summarizeaccesses {} {
  global clusters
  global seeksincluster
  global seekstofile
  for {set cluster 1} {$cluster <= $clusters} {incr cluster} {
    echo ""
    echo Cluster $cluster had $seeksincluster($cluster) nonsequential reads
    echo involving the following files:
    set searchid [array startsearch seekstofile]
    while {[set next [array nextelement seekstofile $searchid]] != ""} {
      busy
      regexp ^(.*),(.*)$ $next allhit filehit clusterhit
      if {$clusterhit == $cluster} then {
        echo "   " $filehit was sought $seekstofile($next) time(s)
      }
    }
    array donesearch seekstofile $searchid
  }
  echo ""
}

proc summarizecatapult {} {
  global catapultfile catapultoffset catapultcount catapultplace catapultindex
  global catapultplace
  global catapulting
  global catapultblocksused
  global catapult
  global blocksize
  global pathto pathnametofile
  global catapultentriesperpage
  global catapultblocks
  global FILE_IS_READONLY FILE_IS_FOR_FILESYSTEM FILE_TYPE_CATAPULT
  if {$catapultblocks == 0} then {
    return "no catapult"
  }
  echo Catapult layout
  set remains 0
  set catapultblocksused 0
  for {set i 0} {$i <= $catapultindex} {incr i} {
    busy
    if {$remains == 0} then {
      echo "  Catapult index page at offset" $catapultblocksused
      incr catapultblocksused
      set remains $catapultentriesperpage
    }
    echo "  " $catapultfile($i), start at offset $catapultoffset($i) for $catapultcount($i) blocks at $catapultblocksused
    set catapultplace($i) $catapultblocksused
    incr catapultblocksused $catapultcount($i)
  }
  echo ""
  echo The catapult file requires $catapultblocksused blocks
  set catapult [makefile "Catapult"]
  set pathto($catapult) {}
  set pathnametofile([getfile $catapult -name]) $catapult
  setfile $catapult -blocksize $blocksize
  setfile $catapult -blockcount $catapultblocksused
  setfile $catapult -bytecount [expr {$catapultblocksused*$blocksize}]
  setfile $catapult -type $FILE_TYPE_CATAPULT
  setfile $catapult -flags [expr {[getfile $catapult -flags] |
                              $FILE_IS_READONLY | $FILE_IS_FOR_FILESYSTEM}]
# Leave some headspace before the catapult file, in the hope that the root
# will sneak in here
  assignavatars $catapult 1 30
  echo The catapult file is located at [getfile $catapult -avatars]
  echo ""
}

proc sortfileclusters {} {
  global clusters
  global seekstofile
  global filehitspercluster
  global filegoesintocluster
  global placefileincluster
  global alternatives
  global oldblocks
  global pathnametofile
  global takedirectory
  global itemtype
  global alternativeblocks
  set fudgefactor 1000
  echo Selecting primary avatar location for each file used
  echo ""
##  set searchid [array startsearch pathnametofile]
##  while {[set next [array nextelement pathnametofile $searchid]] != ""} {
##    echo File $next is path object $pathnametofile($next)
##  }
##  array donesearch pathnametofile $searchid
  set searchid [array startsearch seekstofile]
  while {[set next [array nextelement seekstofile $searchid]] != ""} {
    regexp ^(.*),(.*)$ $next allhit filehit clusterhit
    set token [format "%012o,%d" $seekstofile($next) $clusterhit]
    lappend filehitspercluster($filehit) $token
  }
  array donesearch seekstofile $searchid
#  echo File hits, by cluster:
  set searchid [array startsearch filehitspercluster]
  while {[set next [array nextelement filehitspercluster $searchid]] != ""} {
    busy
    if {[info exists pathnametofile($next)] == 0} then {
      error "ERROR!  File $next was used, but is NOT in $takedirectory!!!"
    }
    set thefile $pathnametofile($next)
    if {$itemtype($thefile) == "directory" ||
        [getfile $thefile -numavatars] > 0} then {
#      echo Avatars for $next will be placed later or are already in place
    } else {
      set hitlist $filehitspercluster($next)
      set sortlist [lsort $hitlist]
      set $filehitspercluster($next) $sortlist
      set last [llength $sortlist]
      incr last -1
      set primary [lindex $sortlist $last]
      regexp ^(.*),(.*)$ $primary allhit accesses clusterhit
      echo First avatar of $next goes into cluster $clusterhit
      set placefileincluster($next,$clusterhit) 1
      lappend filegoesintocluster($next) $clusterhit
      set blocks $oldblocks($next)
      while {$last > 0} {
        incr last -1
        set secondary [lindex $sortlist $last]
        regexp ^(.*),(.*)$ $secondary allhit accesses clusterhit
        set benefit [expr {$accesses * $fudgefactor / $blocks}]
#        echo "  " Secondary candidate in cluster $clusterhit benefit $benefit
        set candidate [format "%012o,%s,%s" $benefit $next $clusterhit]
        lappend alternatives $candidate
      }
    }
  }
  busy
  array donesearch filehitspercluster $searchid
  echo ""
  if {[info exists alternatives] == 0} then {
    echo No alternative avatars are required
  } else {
    echo Placing additional avatars:
    set alternatives [lsort $alternatives]
    set last [llength $alternatives]
    set added 0
    set addedspace 0
    set faired 0
    set fairedspace 0
    set toobig 0
    set toobigspace 0
    while {$last > 0} {
      busy
      incr last -1
      set candidate [lindex $alternatives $last]
#      echo "  Candidate $candidate"
      regexp ^(.*),(.*),(.*)$ $candidate allhit benefit path clusterhit
#      echo Benefit $benefit, path $path, cluster $clusterhit
      set blocks $oldblocks($path)
#      echo Blocks required $blocks
      if {[llength $filegoesintocluster($path)] >= 7} then {
        echo "  File $path already has 7 avatars, no more will be added"
        incr faired
        incr fairedspace $blocks
      } else {
        if {$alternativeblocks >= $blocks} then {
          echo " $path added to cluster $clusterhit"
          lappend filegoesintocluster($path) $clusterhit
          set placefileincluster($path,$clusterhit) 1
          incr alternativeblocks -$blocks
          incr added
          incr addedspace $blocks
        } else {
	  echo "  Insufficient space to add an avatar of $path to cluster $clusterhit"
	  incr toobig
          incr toobigspace $blocks
	}
      }
    }
    echo ""
    if {$added > 0} then {
      echo $added avatars ($addedspace blocks) were added
    }
    if {$faired > 0} then {
      echo $faired avatars ($fairedspace blocks) were not added due to 7-avatar limit
    }
    if {$toobig > 0} then {
      echo $toobig avatars ($toobigspace blocks) were not added due to lack of free space
    }
  }
}

proc allocateclusters {} {
  global clusters
  global filegoesintocluster
  global clustersize
  global oldblocks
  global seeksincluster
  global clusterlocation clusterused clusterpadblocks
  global highwater
  global clusterweighting
  for {set cluster 1} \
      {$cluster <= $clusters} \
      {incr cluster} {
    set clustersize($cluster) 0
  }
  echo ""
  set searchid [array startsearch filegoesintocluster]
  while {[set next [array nextelement filegoesintocluster $searchid]] != ""} {
    busy
    set blocks $oldblocks($next)
    set clusterlist [lsort $filegoesintocluster($next)]
    echo File $next lives in clusters $clusterlist
    foreach cluster $clusterlist {
      incr clustersize($cluster) $blocks
    }
  }
  echo ""
  array donesearch filegoesintocluster $searchid
  for {set cluster 1} \
      {$cluster <= $clusters} \
      {incr cluster} {
#    echo Cluster $cluster requires $clustersize($cluster) blocks
    set weightedseeks [expr {$seeksincluster($cluster) / $clusterweighting}]
    set invertedpos [expr {$clusters - $cluster}]
    set hitsies [format "%012o,%d" $weightedseeks $invertedpos]
    lappend seekfast $hitsies
  }
  set seekfast [lsort $seekfast]
  set cluster $clusters
  set highwater 1
  while {$cluster > 0} {
    incr cluster -1
    set seekpair [lindex $seekfast $cluster]
    regexp ^(.*),(.*)$ $seekpair allhit seeks clusterhit
    set clusterhit [expr {$clusters - $clusterhit}]
#    echo Cluster $clusterhit had $seeksincluster($clusterhit) nonsequential read(s)
    set blocks $clustersize($clusterhit)
    if {[catch {set clusterstart [preassign $highwater $blocks]} whoops] == 0} then {
      echo Cluster $clusterhit will begin at block $clusterstart for $blocks
      set clusterlocation($clusterhit) $clusterstart
      set clusterused($clusterhit) 0
      set highwater [expr {$clusterstart + $blocks + $clusterpadblocks}]
    } else {
      error "Cannot get a contiguous group of $blocks blocks for cluster $clusterhit"
    }
  }
}

proc analyze {} {
  global inputmapfile inputmapfilename
  global accesslogfile accesslogfilename
  global domapping
  global cachefile cachebase cacheblocks
  global clusters
  global blocks
  global holdbackpercentage
  global totaloldblocks
  global alternativeblocks
  global deferbig
  global catapultblocks
  global clusterweighting
  set cachebase -1
  set cacheblocks 0
  set endtime -9999
  set clusters 0
  set prevfile none
  set nextblockinfile -999
  set totaloldblocks 0
  set clusterweighting 10
  set inputmapfile [open $inputmapfilename r]
  set accesslogfile [open $accesslogfilename r]
  echo Reading $inputmapfilename to locate files in old image file
  while {[set line [gets $inputmapfile]] != ""} {
    busy
    eval setoldavatars $line
  }
  close $inputmapfile
  echo ""
  echo Existing image contains $totaloldblocks worth of files and directories
  echo The new image has $blocks blocks of space within it
  set potentialfreeblocks [expr {$blocks-$totaloldblocks}]
  if {$catapultblocks > 0} then {
    if {$potentialfreeblocks > 0 && $potentialfreeblocks < $catapultblocks} then {
      set catapultblocks [expr {$potentialfreeblocks / 2}]
      echo Reduced size of catapult file to $catapultblocks blocks
    } else {
      echo Catapult file requires $catapultblocks blocks
    }
     set potentialfreeblocks [expr {$potentialfreeblocks - $catapultblocks}]
  }
  if {$potentialfreeblocks <= 0} then {
    error "There is no free space available for layout optimization!!"
  }
  set alternativeblocks [expr {
    $potentialfreeblocks * (100 - $holdbackpercentage) / 100}]
  echo There are $potentialfreeblocks blocks of potentially usable optimization space
  echo The optimizer will hold back $holdbackpercentage% of this space
  echo The optimizer will use up to $alternativeblocks blocks for optimization
  echo Files greater than $deferbig blocks will be slid to the end of the cluster
  echo ""
  sortoldblocks
  echo "Initializing catapult file"
  initcatapult
#  echo ""
#  echo "Pre-seeding catapult file with critical startup entries"
#  echo ""
#  preseedcatapult
  echo ""
  echo Reading $accesslogfilename to identify and cluster file accesses
  echo ""
  while {[set line [gets $accesslogfile]] != "" || ![eof $accesslogfile]} {
    set phrase [concat list $line]
    set list [expr {$phrase}]
    if {[llength $list] == 8} then {
      eval recordaccess $line
    }
  }
  close $accesslogfile
  clipcatapult
  summarizeaccesses
  summarizecatapult
  sortfileclusters
  allocateclusters
  echo ""
  echo Re-reading $accesslogfilename to assign the file avatars
  echo ""
  echo Cluster 1:
  set defer {}
  set accesslogfile [open $accesslogfilename r]
  set prevfile none
  set cluster 1
  while {[set line [gets $accesslogfile]] != "" || ![eof $accesslogfile]} {
    set phrase [concat list $line]
    set list [expr {$phrase}]
    if {[llength $list] == 8} then {
      eval nailaccess $line
    }
  }
  flushdeferredfiles $cluster $defer
  close $accesslogfile
  nailavatars
}

proc setup {} {
  global fstype
  global uniques
  global label defaultLabel
  global kilobytes megabytes blocks defaultBlocks
  global blocksize defaultBlocksize
  global directorysize defaultDirectorysize
  global takedirectory defaultTakedirectory
  global preinitialize defaultPreinitialize
  global directoryavatars defaultDirectoryavatars
  global rootavatars defaultRootavatars
  global labelavatars defaultLabelavatars
#  global romTagsSize defaultRomTagsSize
  global catapultmegabytes defaultCatapultMegabytes
  global catapultpages defaultCatapultPages
  global catapultentries catapultentriesperpage
  global catapultrunlimit defaultCatapultRunLimit
  global numromtags
  global volflags
  global catapultblocks
  global labelAtZero imagefile
  global elephantstomp
  global environment
  global workbuffersize
  global outputmapfile outputmapfilename
  global inputmapfile inputmapfilename
  global accesslogfile accesslogfilename
  global domapping
  global sleeplimit blockspersecond
  global holdbackpercentage clusterpadblocks
  global highwater deferbig
  global DISC_TOTAL_BLOCKS
  if {[info exists fstype] == 0} then {
    error "You did not set 'fstype'"
  }
  set uniques(0) 1
  case $fstype in {
    cd-rom {
      set defaultLabel "CD-ROM"
      set defaultBlocks 1024
      set defaultBlocksize 2048
      set defaultDirectorysize 2048
      set deviceBlocksize 2048
      set defaultDirectoryavatars 3
      set defaultRootavatars 7
      set defaultLabelavatars 2
      set defaultImagefile "cdrom.image"
      set defaultCatapultMegabytes 0
      set defaultCatapultPages 1
      set defaultCatapultRunLimit 300
#      set defaultRomTagsSize 2048
      set labelAtZero 1
      set domapping 1
      set elephantstomp "iamaduck"
      }
    ramdisk {
      set defaultLabel "RAMdisk"
      set defaultBlocks 262144
      set defaultBlocksize 1
      set defaultDirectorysize 512
      set deviceBlocksize 1
      set defaultDirectoryavatars 1
      set defaultRootavatars 1
      set defaultLabelavatars 1
      set defaultImagefile "ramdisk.image"
      set defaultCatapultMegabytes 0
      set defaultCatapultPages 0
      set defaultCatapultRunLimit 0
#      set defaultRomTagsSize 2048
      set labelAtZero 1
      set domapping 0
      set elephantstomp 0
      }
    romdisk {
      set defaultLabel "ROMdisk"
      set defaultBlocks 65536
      set defaultBlocksize 4
      set defaultDirectorysize 512
      set deviceBlocksize 1
      set defaultDirectoryavatars 1
      set defaultRootavatars 1
      set defaultLabelavatars 1
      set defaultImagefile "romdisk.image"
      set defaultCatapultMegabytes 0
      set defaultCatapultPages 0
      set defaultCatapultRunLimit 0
#      set defaultRomTagsSize 2048
      set labelAtZero 1
      set domapping 0
      set elephantstomp 0xFFFFFFFF
      }
    default {
      error "Illegal fstype '$fstype'"
      }
    }
  if {$environment == "Mac"} then {
    set defaultTakedirectory ":takeme"
    set defaultPreinitialize 1
  } else {
    set defaultTakedirectory "takeme"
    set defaultPreinitialize 0
  }
  echo ""
  echo Creating a $fstype filesystem.
  echo ""
  if {[info exists label] == 0} then {
    set label $defaultLabel
  }
  if {[info exists blocksize] == 0} then {
    set blocksize $defaultBlocksize
  }
  if {[info exists directorysize] == 0} then {
    set directorysize $defaultDirectorysize
  }
  if {$directorysize < $blocksize} then {
    set directorysize $blocksize
  }
  if {[info exists blocks] == 0} then {
    if {[info exists kilobytes]} then {
      set blocks [expr {$kilobytes*1024/$blocksize}]
    } else {
      if {[info exists megabytes]} then {
	if {$megabytes > 1024} then {
	  error "Image size of $megabytes megabytes exceeds 1024-megabyte limit"
        }
        set blocks [expr {$megabytes*1024*1024/$blocksize}]
      } else {
        set blocks $defaultBlocks
      }
    }
  }
  if {$fstype == "cd-rom" && ($blocks % 16) != 0} then {
    error "CD-ROM image size is not a multiple of 32k bytes (16 blocks)"
  }
  if {[info exists preinitialize] == 0} then {
    set preinitialize $defaultPreinitialize
  }
  if {[info exists directoryavatars] == 0} then {
    set directoryavatars $defaultDirectoryavatars
  }
  if {[info exists rootavatars] == 0} then {
    set rootavatars $defaultRootavatars
  }
  if {[info exists labelavatars] == 0} then {
    set labelavatars $defaultLabelavatars
  }
  if {[info exists takedirectory] == 0} then {
    set takedirectory $defaultTakedirectory
  }
  if {[info exists imagefile] == 0} then {
    set imagefile $defaultImagefile
  }
#  if {[info exists romTagsSize] == 0} then {
#    set romTagsSize $defaultRomTagsSize
#  }
  if {[info exists outputmapfilename] == 0} then {
    set outputmapfilename "filemap.out"
  }
  if {[info exists inputmapfilename] == 0} then {
    set inputmapfilename "filemap.in"
  }
  if {[info exists accesslogfilename] == 0} then {
    set accesslogfilename "CD_Access.Log"
  }
  if {[info exists workbuffersize] == 0} then {
    set workbuffersize 65536
  }
  if {[info exists sleeplimit] == 0} then {
    set sleeplimit 60
  }
  if {[info exists blockspersecond] == 0} then {
    set blockspersecond 300
  }
  if {[info exists holdbackpercentage] == 0} then {
    set holdbackpercentage 10
  }
  if {[info exists clusterpadblocks] == 0} then {
    set clusterpadblocks 10
  }
  if {[info exists deferbig] == 0} then {
    set deferbig 1000
  }
  if {[info exists catapultmegabytes] == 0} then {
    set catapultmegabytes $defaultCatapultMegabytes
  }
  if {[info exists catapultpages] == 0} then {
    set catapultpages $defaultCatapultPages
  }
  if {[info exists catapultrunlimit] == 0} then {
    set catapultrunlimit $defaultCatapultRunLimit
  }
  if {[info exists numromtags] == 0} then {
    set numromtags 8
  }
  set catapultentriesperpage [expr {$blocksize / 16 - 1}]
  set catapultentries [expr {$catapultpages * $catapultentriesperpage}]
  set catapultblocks [expr {$catapultmegabytes*1024*1024/$blocksize}]
  set highwater 0
  echo The filesystem will be labeled '$label'
  echo It will be written to a file called '$imagefile'
  echo It will have $blocks blocks of $blocksize bytes each
  echo Directories will be written in blocks of $directorysize bytes each
  echo There will be up to $labelavatars avatar(s) of the filesystem label.
  echo There will be up to $rootavatars avatar(s) of the root directory.
  echo There will be up to $directoryavatars avatar(s) of all other directories.
  echo Files will be taken from '$takedirectory'
  if {$domapping} then {
    echo "File map will be written to $outputmapfilename"
    echo "Old file map will be read from $inputmapfilename"
    echo "Access log will be read from $accesslogfilename"
    echo "There will be $clusterpadblocks blocks between clusters"
    if {$catapultblocks > 0 && $catapultpages > 0 && $catapultrunlimit > 0} then {
      echo "A catapult file of at most $catapultblocks blocks will be written"
      echo "At most $catapultpages catapult index pages will be written"
      echo "At most $catapultentries catapult entries will be written"
      echo "Catapult entries will be at most $catapultrunlimit blocks long"
    } else {
      echo "No catapult file will be created"
      set catapultblocks 0
    }
  }
#  echo The ROM tags file will be $romTagsSize bytes in length
  echo Copying/preinitializing buffer will be $workbuffersize bytes
  echo ""
  if {$fstype == "cd-rom"} then {
    if {$blocks > $DISC_TOTAL_BLOCKS} then {
      echo "*** Warning!  CD-ROM image size exceeds $DISC_TOTAL_BLOCKS blocks."
      echo "*** This image may be too large to master on today's equipment."
      echo ""
    }
  }
}

#
# Here's where the top-level layout process driver lives.
#

proc doit {} {
  global label
  global blocks
  global blocksize
  global directorysize
  global takedirectory
  global preinitialize
  global directoryavatars
  global rootavatars
  global labelavatars
#  global romTagsSize
  global disc
  global outputmap
  global pathto pathnametofile
  global imagefile
  global labelAtZero
  global numromtags
  global volflags
  global catapultindex catapulting
  global FILE_TYPE_LABEL
  global FILE_IS_READONLY FILE_IS_FOR_FILESYSTEM
  global DISC_LABEL_OFFSET DISC_LABEL_AVATAR_DELTA
  global outputmapfile outputmapfilename
  global inputmapfile inputmapfilename
  global accesslogfile accesslogfilename
  global domapping
  global root
  global catapult catapultblocksused
  global applicationid
  setup
#
# Make a root directory for the filesystem
#
  echo Creating root directory...
  set root [makedirectory "Filesystem root"]
#
# Open a CD-ROM disc image file
#
  echo Creating disc file
  set disc [opendisc $imagefile $blocks]
#
# Create file map
#
  if {$domapping} then {
    echo Creating $outputmapfilename
    deletefile $outputmapfilename
    set outputmapfile [open $outputmapfilename w+]
  }
#
# Build a label structure
#
  echo Creating disc label...
  set labelinfo [newlabel $label $blocks [unique] $blocksize]
  set label [lindex $labelinfo 0]
#
# According to the Rules Of The Game, every block allocated on the
# disc must lie within precisely one file which is accessible from
# the root of the filesystem.  Hence, the label itself must be framed
# by a file.  Create such a file, and allocate space for it in the
# magic set of locations known to the low-level disc-label-searching
# code in the mount-filesystem code.
#
  echo Creating label-file placeholder...
  set labelfile [makefile "Disc label"]
  set pathto($labelfile) {}
  set pathnametofile([getfile $labelfile -name]) $labelfile
  setfile $labelfile -blocksize $blocksize
  setfile $labelfile -type [expr {$FILE_TYPE_LABEL}]
  setfile $labelfile -flags [expr {[getfile $labelfile -flags] |
                              $FILE_IS_READONLY | $FILE_IS_FOR_FILESYSTEM}]
#  set romtagsfile [makefile "ROM tags"]
#  set pathto($romtagsfile) {}
#  setfile $romtagsfile -blocksize $blocksize
#  setfile $romtagsfile -type ""
#  setfile $romtagsfile -flags [expr {[getfile $labelfile -flags] |
#                              $FILE_IS_READONLY | $FILE_IS_FOR_FILESYSTEM}]
#  setfile $romtagsfile -bytecount $romTagsSize
#  set romtagsblockcount [expr {($romTagsSize+$blocksize-1)/$blocksize}]
#  setfile $romtagsfile -blockcount $romtagsblockcount
  if {$labelAtZero} then {
    set labelbase 0
  } else {
    set labelbase $DISC_LABEL_OFFSET
  }
  set labelavatarlist {}
#  set romtagsavatarlist {}
  set labelsize [bufferused $labelinfo]
  set labelblocksize [getfile $labelfile -blocksize]
  diag Label block size is $labelblocksize
  diag Label size is $labelsize
  set labelblockcount [expr {($labelsize+$labelblocksize-1)/$labelblocksize}]
  diag Label block count calculated to be $labelblockcount
  set labeldiscblockcount [expr {(($labelblockcount*$labelblocksize)+$blocksize-1)/
                            $blocksize}]
  diag Label disc block count calculated to be $labeldiscblockcount
#  diag ROMtag block count calculated to be [getfile $romtagsfile -blockcount]
  for {set avatarindex 0} \
      {$avatarindex < $labelavatars} \
     {incr avatarindex} \
      {
	if {[catch {seize $labelbase $labeldiscblockcount} whoops] == 0} then {
	  set labelavatarlist [concat $labelavatarlist $labelbase]
#          set romtagsavatar [expr {$labelbase + $labeldiscblockcount}]
#          diag Got label avatar at $labelbase, trying tags at $romtagsavatar
#          if {[catch {seize $romtagsavatar $romtagsblockcount} whoops] == 0} then {
#            set romtagsavatarlist [concat $romtagsavatarlist $romtagsavatar]
#          } else {
#            echo ROMtags error: $whoops
#          }
	}
	if {$labelAtZero} then {
          set labelbase $DISC_LABEL_OFFSET
	  set labelAtZero 0
	} else {
	  set labelbase [expr {$labelbase + $DISC_LABEL_AVATAR_DELTA}]
	}
      }
  echo Label located at $labelavatarlist
  eval setavatars $labelfile $labelavatarlist
#  echo ROM tags located at $romtagsavatarlist
#  eval setavatars $romtagsfile $romtagsavatarlist
  setfile $labelfile -bytecount $labelsize
#
# Arrange the files in a suitable fashion
#
  echo Scanning '$takedirectory' for files and subdirectories
  takedir $takedirectory $root
  echo Putting files into root directory
  addto $root $labelfile
  echo Mapping pathnames
  mapnames $root
#  addto $root $romtagsfile
#
# Attempt use-analysis based on prior layout history and access log,
# if they exist
#
  if {$domapping} then {
    echo ""
    echo Attempting layout analysis and optimization
    echo ""
    if {0 == [catch {analyze} whoops]} then {
      echo ""
      echo "Optimization successful."
      echo ""
    } else {
      echo ""
      echo $whoops
      echo ""
      echo "***** Analysis and optimization failed *****"
      echo ""
    }
    if {[info exists catapult] != 0} then {
      addto $root $catapult
    }
  }
#
# Lay out the root directory
#
  echo Formatting root directory...
  set rootbuffer [builddirectory $root]
  set rootbytes [bufferused $rootbuffer]
  set rootblocksize [getfile $root -blocksize]
  set rootblocks [expr {($rootbytes+$rootblocksize-1)/$rootblocksize}]
  assignavatars $root $rootavatars 1
  setfile $root -blockcount $rootblocks
  echo Root has $rootblocks block(s) of $rootblocksize bytes
  set root_avatar_list [getfile $root -avatars]
  echo Filesystem root located at $root_avatar_list
#
# Store the information about the root directory into the label
# structure.
#
  echo Completing label...
  set rootid [getfile $root -uniqueidentifier]
  eval setroot $label $rootblocks $rootblocksize $rootid $root_avatar_list
  setromtags $label $numromtags
  if {[info exists applicationid] != 0} then {
    setappid $label $applicationid
  }
  if {[info exists volflags] != 0} then {
    setvolflags $label $volflags
  }
#
# Write the label and root to the disc image.
#
  writefile $disc $labelfile $label
  writefile $disc $root $rootbuffer
#  set romtagsbuffer [makebuffer $romTagsSize]
#  stuffzeroes $romtagsbuffer $romTagsSize
#  writefile $disc $romtagsfile $romtagsbuffer
#
# If a catapult file was allocated, construct and write it
#
  if {[info exists catapult]} then {
    writecatapult
  }
  echo Closing disc file
  close $disc
  if {$domapping} then {
    close $outputmapfile
  }
#
# That's all she wrote!
#
}
