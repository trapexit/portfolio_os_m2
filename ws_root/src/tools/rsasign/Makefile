# @(#) Makefile 96/09/06 1.6

UNREGISTERED_FILES = libbsafe2.a
ARCHIVE_DIR = /thdo/ssw/m2/examples_data/Tools/rsasign
VER = 01Sep96

include ${CODEMGR_WS}/ws_root/variables/hostexec
include ${CODEMGR_WS}/ws_root/rules/hostexec

HOSTCFLAGS += -DMEMMOVE_PRESENT=0
LINK_LIBS_DIR += .
HOST_LINK_LIBS = bsafe2

# Need to create the libbsafe2.a library before building.
# We only have this lib in binary form, so it's archived separately
# so we need to softlink to it.
all.here: libbsafe2.a
libbsafe2.a:
	${SLN} ${ARCHIVE_DIR}/${VER}/libbsafe2.a .

clean:
	rm -f libbsafe2.a
