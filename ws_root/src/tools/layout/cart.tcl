# @(#) cart.tcl 96/09/11 1.3
#
# This script sets the parameters for an M2 filesystem layout,
# and then invokes the script functions which drive the layout process
# itself.
#
# Copy this file and edit it according to your needs.
#
# It may be used under Unix via "layout < cdrom.tcl"
# It may be used under MPW via "laytool < cdrom.tcl"
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

# set imagefile "my.latest.image"

#
# label - set to the label (name) of the filesystem.  You should normally
# let the defaults be used.  See the CD-ROM Mastering Guide for exceptions.
#

# set label "MyDisk"

#
# numromtags - number of RomTag entries on the disc.
# You should not change this value.
#

set numromtags 2

#
# volflags - volume flags.
# See #defines in <file/discdata.h>
# You should not change this value.
#

set volflags 11

#
# kilobytes, megabytes, or blocks - set to the size of the desired
# filesystem.  Set only one of these.  If you request too small a
# value, the filesystem will run out of space and the layout process
# will abort.
#
# When doing a test layout of a filesystem, you can set the size field to
# a value somewhat larger than that of the "takeme" folder - 110% of the
# takeme folder size is a good starting point.
#
# When doing a final, optimized/clustered/Catapulted layout for submission
# to 3DO, it is best to specify a large filesystem size.  The larger the
# filesystem image (and the more "unused" space in excess of the size
# of your "takeme" folder), the better a job the optimizer can do in
# making multiple avatars of your commonly-used files, and creating a big
# Catapult file to speed up the CD-booting process.  Unless you have a
# severe space shortage on your development-station hard disk, I recommend
# that you set the filesystem size to 600 megabytes or larger when doing
# your final pre-encryption layout.
#

set kilobytes 512
# set megabytes 600
# set blocks 512

#
# blocksize - set to the desired blocksize for the filesystem.
# Usually does not need to be specified.
#

# set blocksize 4

#
# takedirectory - set to the pathname of the source directory which is
# to be mirrored onto the filesystem.
#
# Unix environment defaults to "takeme"
# Mac  environment defaults to ":takeme"
#

# set takedirectory "takeme"

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
# If the layout process fails with an "Unable to assign avatar for file XXX",
# and file XXX is a large one (say, more than 100 megabytes), you may
# wish to set directoryavatars to 1 and try the layout again.  This will
# eliminate free-space fragmentation due to the placement of multiple
# directory avatars.
#
# If you do this, please set directoryavatars back to a higher value
# (3 is good) when doing your final optimized layout per the instructions
# in the CD-ROM mastering guide.
#

# set directoryavatars 3

#
# rootavatars - set to the number of copies of the root directory that
# you wish to have scattered across the disc.  May be reduced to 2, but
# no lower.
#

# set rootavatars 7

#
# labelavatars - set to the number of copies of the label that you
# wish to have scattered across the disc.  Be careful about increasing
# labelavatars to any value greater than 2 - the avatars of the label
# are placed at fixed offsets in the filesystem image, and will tend to
# break up the free space in the image into chunks of less than 32
# megabytes each.  This can impact your ability to create CD-ROMs which
# have very large files on them.
#

# set labelavatars 2

#
# Define the size and characteristics of the Catapult file.  These
# parameters specify the number of megabytes of space which the Catapult
# file may use, the number of table-of-contents index pages which can
# be placed in the file, and the size of the longest "run" of
# consecutive blocks from a single file which can be placed in the
# file.
#
# If any of these parameters is left undefined, the Catapult file
# will not be built, and you will sacrifice the boot-time speedups which
# Catapult optimization can provide.
#
# See the CD-ROM mastering guide for information about the Catapult
# technology and tuning process.
#

# set catapultmegabytes 50
# set catapultpages 10
# set catapultrunlimit 5120

#
# At this point, you may include statements which exclude individual
# files, or entire directories from inclusion in the Catapult file.
# It is often desireable to exclude long FMV sequences, or long
# audio background-music tracks, from the Catapult file - this retains
# space in the Catapult file for smaller and more seek-intensive files.
#
# The following statement would exclude a specific file:
#
#    excludecatapult "data/intro.stream"
#
# The following statement would exclude all files in a specific directory:
#
#    excludecatapult "data/sounds/music/"

#
# OK, invoke deep magick and do the layout.
#

if {0 == [catch {doit} whoops]} then {
  echo ""
  echo "Layout successful."
  echo ""
} else {
  echo ""
  echo $whoops
  echo ""
  echo "***** Layout failed *****"
  echo ""
}
