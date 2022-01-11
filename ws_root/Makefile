# @(#) Makefile 96/09/30 1.31
# Top-level makefile for the Portfolio system


###############################################################################
# generic prolog

include ${CODEMGR_WS}/ws_root/variables/generic


###############################################################################
#
# Main entry point, does a full build of the system

default: all sources.list


###############################################################################
# We mainly trigger builds in our subdirectories

BUILD_IN_FIRST =	variables rules

include ${CODEMGR_WS}/ws_root/rules/subtree


###############################################################################
# generic epilog

include ${CODEMGR_WS}/ws_root/rules/generic


###############################################################################
# hey, from the top, we get shortcuts!

include ${CODEMGR_WS}/ws_root/rules/shortcut


###############################################################################
# get rid of generated files

clean.here:
	@echo ... `date '+%T'` Cleaning out ${RELEASE_DIR} ...
	${RM} ${RELEASE_DIR}
	@echo ... `date '+%T'` Cleaning out ${LINK_LIB_DIR} ...
	${RM} ${LINK_LIB_DIR}
	@echo ... `date '+%T'` Cleaning out ${VECTOR_DIR} ...
	${RM} ${VECTOR_DIR}
	@echo ... `date '+%T'` Cleaning out hostbin/${HOSTOS} ...
	${RM} hostbin/${HOSTOS}
	@echo ... `date '+%T'` Cleaning out hostlib/${HOSTOS} ...
	${RM} hostlib/${HOSTOS}
	@echo ... `date '+%T'` Cleaning out release/mac ...
	${RM} release/mac
	@echo ... `date '+%T'` Cleaning out release/docs ...
	${RM} release/docs
	${RM} sources.list autodoc.tmp

clean:
	@echo ... `date '+%T'` Removing leftover empty directories ...
	-find . -depth \( ! -name . \) \( ! -name SCCS \) -type d -print | xargs rmdir > /dev/null 2>&1
	@echo ... `date '+%T'` Clean as a whistle ...


###############################################################################
# copy the release hierarchy from the reference build

clone_release:
	@echo ... `date '+%T'` Copying reference release/3do_os/${BUILD_TYPE} directory ...
	-${MD} ${RELEASE_DIR}
	${GNUCP} -prd ${REFERENCE_ROOT}/release/3do_os/${BUILD_TYPE}/* ${RELEASE_DIR}

###############################################################################
#
# Note that this is actually a MAC cdrommaster directory
#
cdrommaster:
	@echo ... `date '+%T'` Creating cdrommaster folder ...
	-${MD} release/cdrommaster
	ln -s ${SRC}/others/roms/objects/unenc/${OPTIONS_PATH}/takeme.cdrom release/cdrommaster/takeme
	${SANITIZE} < ${SRC}/includes/file/discdata.h | ${FIXCOPYR} | ${FIXINCLUDES} | ${FIXEOL} > release/cdrommaster/discdata.h
	ln -s ../mac/includes/file/filesystem.h release/cdrommaster/filesystem.h
	${SANITIZE} < ${SRC}/tools/layout/cdrom.tcl | ${FIXCOPYR} | ${FIXEOL} > release/cdrommaster/cdrom.tcl
	${SANITIZE} < ${SRC}/tools/layout/layout.tcl | ${FIXCOPYR} | ${FIXEOL} > release/cdrommaster/layout.tcl
	@echo ... Mac tools needed: LayTool and MakeBannerScreen ...
	@echo ... `date '+%T'` Cdrommaster ready for use ...

