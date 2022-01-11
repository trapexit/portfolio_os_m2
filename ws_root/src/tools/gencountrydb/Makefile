# @(#) Makefile 95/11/30 1.14
#
# Generate CountryDatabase

default: all

include ${CODEMGR_WS}/ws_root/variables/generic

HINCDIRS =		${SRC}/folios/jstring \
			${SRC}/includes

TARGET_NAME =		CountryDatabase
TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/International
FULL_TARGET_PATH =	${TARGET_DIRECTORY}/${TARGET_NAME}
FTP_TRANSFER =		${FULL_TARGET_PATH}

GEN_COUNTRY_DB =	${TARGET_NAME:%=${HOSTOSBIN}/%}
COUNTRY_DB =		${TARGET_NAME:%=objects/%}

include ${CODEMGR_WS}/ws_root/rules/generic

all.post: ${FULL_TARGET_PATH}

makedirs.here:
	@[ -d objects ] || ${MD} objects

# Normally, we would generate the file into its
# destination, but in this case we would have up to
# four copies of the same identical file after a
# complete build (since ${BUILD_TYPE} does not
# alter the contents of CountryDatabase), so for
# this file, we keep a local link and hardlink to
# the destination.

${FULL_TARGET_PATH}: ${COUNTRY_DB}
	${hardlinkme}

${COUNTRY_DB}: ${GEN_COUNTRY_DB}
	${GEN_COUNTRY_DB} $@

${GEN_COUNTRY_DB}: gencountrydb.c
	${HOSTCC} -o $@ ${HOSTCFLAGS} $< ${HOSTOSLIB:%=-L%} ${LINK_LIBS:%=-l%}

clean:
	${RM} ${GEN_COUNTRY_DB} ${COUNTRY_DB} ${FULL_TARGET_PATH}
