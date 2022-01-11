# @(#) sysdisk.tcl 96/09/11 1.6
#
#

#
#
# This script sets the parameters for an Opera filesystem layout,
# and then invokes the script functions which drive the layout process
# itself.
#
# Copy this file and edit it according to your needs.
#

#
# fstype - set this to either "ramdisk", "romdisk" or "cd-rom".  This will
# result in the use of reasonable defaults for any parameters which are not
# explicitly specified.
#

set fstype "romdisk"

#
# imagefile - set to the name of the Mac or Unix file into which the
# new filesystem image should be written.  Defaults to "cdrom.image",
# "ramdisk.image", or "romdisk.image" depending on the fstype.
#

set imagefile "../release/sysdisk.image"

#
# label - set to the label (name) of the filesystem.  You should normally
# let the defaults be used.
#

set label "rom"

#
# kilobytes, megabytes, or blocks - set to the size of the desired
# filesystem.  Set only one of these.  If you request too small a
# value, the filesystem will run out of space and the layout process
# will abort.
#

# set kilobytes 300
#set megabytes 1
set megabytes 2
# set blocks 512

#
# blocksize - set to the desired blocksize for the filesystem.
# Usually does not need to be specified.
#

# set blocksize 2048

#
# takedirectory - set to the pathname of the source directory which is
# to be mirrored onto the filesystem.
#
# Unix environment defaults to "takeme"
# Mac  environment defaults to ":takeme"
#

set takedirectory "../release/remote"

#
# preinitialize - set to 1 if the storage for the filesystem should
# be initialized with known valuesbefore data is laid out on it.  Set
# to 0 if the filesystem should not be preinitialized.
#
# The default for this parameter is 1 in the Macintosh environment and
# 0 in the Unix environment.
#
#
# Mac users - you may set this parameter to 0 to speed up filesystem
# layout during the development process.  However, DO NOT EVER lay out a
# filesystem for general distribution (i.e. to the consumer market) with
# preinitialization turned off.  If you do, then "residue" from previous
# uses of blocks on your hard drive may appear in "unused" portions of the
# Opera filesystem.  This would be bad.  You could end up inadvertently
# publishing your source code this way.  You do not want this to occur.
#
# Sun users do not need to worry about this, because SunOS scrubs all
# hard-disk blocks the first time they are used.  I believe this is true
# of all BSD systems.  It wouldn't hurt to turn preinitialization on
# during your final layout run, just to make sure.
#
# You have been warned.
#
# Writers of ROMdisks may wish to set this parameter to 1.  The default
# preinitialization value for ROMdisks is 0xFF, which happens to be the
# value stored in a standard EPROM which has just been erased.  Many
# PROM-programmers are intelligent enough to skip over bytes which
# contain 0xFF - hence, you can probably speed up the PROM-programming
# process by making the filesystem image size identical to that of the
# EPROM and turning on preinitialization.

set preinitialize 1

#
# elephantstomp - set to the value to which you wish to have unused
# blocks bytes in the filesystem image initialized.  Defaults to 0
# for RAM disks, 0xFF for ROMdisks, and "iamaduck" for CD-ROMs.  Can be
# set to a one-byte value (0x00 - 0xFF), a 32-bit integer value, or
# a string.
#

# set elephantstomp 0x00

#
# directoryavatars - set to the number of copies of each directory
# (other than the root) you wish to have scattered across the filesystem.
#

# set directoryavatars 3

#
# rootavatars - set to the number of copies of the root directory that
# you wish to have scattered across the disc.
#

# set rootavatars 7

#
# labelavatars - set to the number of copies of the label that you
# wish to have scattered across the disc.
#

# set labelavatars 7

#
# OK, invoke deep magick and do the layout.
#

if {0 == [catch {doit} whoops]} then {
  echo ""
  echo "Layout successful."
  echo ""
} else {
  echo ""
  echo "***** Layout failed *****"
  echo ""
  echo $whoops
  echo ""
}
