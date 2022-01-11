# @(#) Makefile 96/05/19 1.14
# Makefile to trigger all source directories to be built.

include ${CODEMGR_WS}/ws_root/variables/generic

default: all

# Build the "tools" and "libs" trees first: tools should come first,
# since it creates build tools needed for the rest of the build. Libs
# should come next, since is builds libraries needed by the rest of
# the build. We automatically build in the other directories
# afterward.
#
# We build vectors immediately after libraries, because the rest of
# the build links with the vector files

BUILD_IN_FIRST =	tools libs vectors
BUILD_IN_LAST =		others

include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/generic
