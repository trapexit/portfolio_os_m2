# @(#) Makefile 96/08/20 1.16

CDEFINES =		-DCONTROLPORT
LOADERTOOL_FLAGS =	-e main
NO_CHDIR_MODULE =	1
SHOW_MODULE_INFO =	1

ROMIFY_DIRECTORY =	${REMOTE_DIR}/System.m2/EventBroker
ROMIFY_NAMES =		driverlet_1 driverlet_41 driverlet_49 driverlet_4d driverlet_6
ROMIFIES =		${ROMIFY_NAMES:%=${ROMIFY_DIRECTORY}/%.eb}

include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/multilink

romify =		${hardlinkme}

${ROMIFY_DIRECTORY}:
	${MD} ${ROMIFY_DIRECTORY}

${ROMIFY_DIRECTORY}/driverlet_1.eb: ${OBJDIR}/StickDriver
	${romify}

${ROMIFY_DIRECTORY}/driverlet_6.eb: ${OBJDIR}/SteeringWheelDriver
	${romify}

${ROMIFY_DIRECTORY}/driverlet_49.eb: ${OBJDIR}/MouseDriver
	${romify}

${ROMIFY_DIRECTORY}/driverlet_41.eb: ${OBJDIR}/GlassesDriver
	${romify}

${ROMIFY_DIRECTORY}/driverlet_4d.eb: ${OBJDIR}/LightGunDriver
	${romify}

all.post: ${ROMIFY_DIRECTORY} ${ROMIFIES}

clean:
	${RM} ${ROMIFIES}
