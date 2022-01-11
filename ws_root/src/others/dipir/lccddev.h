/* $Id: lccddev.h,v 1.8 1994/11/07 22:55:21 markn Exp $
**
** Definition of Low-Cost CD-ROM interface (hardware/firmware).
**
**	Copyright 1994 by The 3DO Company Inc.
*/

#define	DMA_CH0			0x1
#define	DMA_CH1			0x2

/* CIP states (physical drive states). */
#define	DRV_OPEN		0
#define	DRV_STOP		1
#define	DRV_PAUSE		2
#define	DRV_PLAY		3
#define	DRV_OPENING		4
#define	DRV_STUCK		5
#define	DRV_CLOSING		6
#define	DRV_STOP_FOCUSED	7
#define	DRV_STOPPING		8
#define	DRV_FOCUSING		9
#define	DRV_FOCUSERROR		10
#define	DRV_SPINNINGUP		11
#define	DRV_UNREADABLE		12
#define	DRV_SEEKING		13
#define	DRV_SEEKFAILURE		14
#define	DRV_LATENCY		15

#define	DRV_OPEN_STATE(state)	((state) == DRV_OPEN || \
				 (state) == DRV_OPENING || \
				 (state) == DRV_STOPPING || \
				 (state) == DRV_STUCK)

#define	DRV_ERROR_STATE(state)	((state) == DRV_STUCK || \
				 (state) == DRV_FOCUSERROR || \
				 (state) == DRV_UNREADABLE || \
				 (state) == DRV_SEEKFAILURE)

/* Disc commands. */
#define	CMD_LED			0x01 /* Turn on/off LED */
#define	CMD_READ_ERROR		0x82 /* Read error byte */
#define	CMD_READ_ID		0x83 /* Read device ID */
#define	CMD_SET_SPEED		0x04 /* Set drive speed */
#define	CMD_SPIN_DOWN_TIME	0x05 /* Set spin-down timeout */
#define	CMD_SECTOR_FORMAT	0x06 /* Select sector format */
#define	CMD_SEND_BYTES		0x07 /* not used */
#define	CMD_PPSO		0x08 /* Pause / play / open / close */
#define	CMD_SEEK		0x09 /* Seek to a sector */
#define	CMD_CHECK_WO		0x0A /* not used */
#define	CMD_DRIVE_STATE_REPORT	0x0B /* Enable/disable CIP reports */
#define	CMD_QCODE_REPORT	0x0C /* Enable/disable QCode reports */
#define	CMD_SWITCH_REPORT	0x0D /* Enable/disable switch reports */
#define	CMD_READ_FIRMWARE	0x20 /* Read drive firmware */
#define	CMD_MECH_TYPE		0x21 /* Get type of drive mechanism */
#define	CMD_COPY_PROT_THRESH	0x23 /* not used */
#define	CMD_GET_WDATA		0x24 /* Read wdata */

/* Disc reports. */
#define	REPORT_TAG		0x10
#define	REPORT_DRIVE_STATE	(REPORT_TAG|CMD_DRIVE_STATE_REPORT)
#define	REPORT_QCODE		(REPORT_TAG|CMD_QCODE_REPORT)
#define	REPORT_SWITCH		(REPORT_TAG|CMD_SWITCH_REPORT)

/* Size (bytes) of various report packets. */
#define	READ_ID_REPORT_SIZE	12
#define	READ_ERROR_REPORT_SIZE	4
#define	CHECK_WO_REPORT_SIZE	9
#define	DRIVE_STATE_REPORT_SIZE	2
#define	QCODE_REPORT_SIZE	11
#define	SWITCH_REPORT_SIZE	3
#define	READ_FIRMWARE_RESP_SIZE	8
#define	MECH_TYPE_RESP_SIZE	8
#define	COPY_PROT_THRESH_RESP_SIZE 8
#define	GET_WDATA_RESP_SIZE	8

/* Second byte of CMD_xxx_REPORT */
#define	REPORT_NEVER		0
#define	REPORT_NOW		1
#define	REPORT_ENABLE		2
#define	REPORT_ENABLE_DETAIL	3

/* Second byte of CMD_PPSO */
#define	CMD_PPSO_OPEN		0
#define	CMD_PPSO_STOP		1
#define	CMD_PPSO_PAUSE		2
#define	CMD_PPSO_PLAY		3

/* Layout of a QCODE report */
#define	QR_ADRCTL		1
#define	QR_TNO			2
#define	QR_POINT		3
#define	QR_MIN			4
#define	QR_SEC			5
#define	QR_FRAME		6
#define	QR_PMIN			8
#define	QR_PSEC			9
#define	QR_PFRAME		10

/* Bits in second & third bytes of REPORT_SWITCH */
#define	OPEN_SWITCH		0x1
#define	CLOSE_SWITCH		0x2
#define	USER_SWITCH		0x4

/* Definitions for CMD_READ_FIRMWARE */
#define	FIRMWARE_BLOCK_SIZE	(2*1024)
#define	MIN_FIRMWARE_SIZE	(8*1024)

/* Bits in the third byte of the MECH_TYPE response. */
#define	MECH_TYPE_SPEED_8	0x08
#define	MECH_TYPE_SPEED_6	0x04
#define	MECH_TYPE_SPEED_4	0x02
#define	MECH_TYPE_SCAN		0x01

/* Bits in the FORMAT register. */
#define	FMT_AUDIO		(0<<5)
#define	FMT_MODE2_FORM0		(1<<5)
#define	FMT_MODE1		(2<<5)
#define	FMT_MODE1_AUXCH0	(3<<5)
#define	FMT_MODE2_FORM1		(4<<5)
#define	FMT_MODE2_FORM1_AUXCH0	(5<<5)
#define	FMT_MODE2_FORM2		(6<<5)
#define	FMT_MODE2_FORM2_AUXCH0	(7<<5)
#define	FMT_HEADER_CH0		0x10
#define	FMT_COMP_CH0		0x08
#define	FMT_ROM_CH1EN		0x04
#define	FMT_C2P0EN		0x02
#define	FMT_C2P0CHAN		0x01

/* The format of the data as it comes from the drive. */
typedef struct DiscBlock {
	uint32			db_Header;
	uint8			db_Data[DISC_BLOCK_SIZE];
	uint32			db_ECC[72];
	uint32			db_Completion;
} DiscBlock;

/* Bits in the completion word. */
#define	COMPL_ECC		0x02

/* An infinite timeout */
#define	NOTIMEOUT		0
#define	NOTIMEOUT_VALUE		0

