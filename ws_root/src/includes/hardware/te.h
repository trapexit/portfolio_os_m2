#ifndef __HARDWARE_TE_H
#define __HARDWARE_TE_H


/******************************************************************************
**
**  @(#) te.h 96/05/09 1.9
**
**  Definitions related to Triangle Engine Hardware.
**
******************************************************************************/


/*
 * Macros for Reading and Writing Registers
 */


#define ReadRegister(RegisterAddress)		*((volatile uint32 *)RegisterAddress)

#define WriteRegister(RegisterAddress, data)	\
				*((volatile uint32 *)RegisterAddress) = (uint32)data

#define SetRegister(RegisterAddress, data)	\
			*((volatile uint32 *)(RegisterAddress+0x1000)) = (uint32)data

#define ClearRegister(RegisterAddress, data)	\
			*((volatile uint32 *)(RegisterAddress+0x1800)) = (uint32)data


/*
 * Definitions for the Address of Various Registers
 */

/********************************
 * Vertex Unit			*
 ********************************/

/*
 * General Control Registers
 */

/* Master Mode Register RWSC$ */
#define TE_MASTER_MODE_ADDR			0x00040000

#define		TE_MM_DISABLE_DITHERING		0x0020
#define		TE_MM_DISABLE_ZBUFFER		0x0010
#define		TE_MM_DISABLE_DESTBLEND		0x0008
#define		TE_MM_DISABLE_SHADING		0x0004
#define		TE_MM_DISABLE_TEXTURING		0x0002
#define		TE_MM_RESET_TE			0x0001


/* Immediate Instruction Data Register RWSC$ */
#define	TE_IMMINSTR_DATA_ADDR			0x00040008

/* Immediate Instruction Control Register RWSC$ */
#define TE_IMMINSTR_CNTRL_ADDR			0x0004000C

#define		TE_IMMINSTR_START		0x0020
#define		TE_IMMINSTR_CONTINUE		0x0010
#define		TE_IMMINSTR_STOP_CRNTINSTR	0x0008
#define		TE_IMMINSTR_STOP_LISTEND	0x0004
#define		TE_IMMINSTR_STEP		0x0002
#define		TE_IMMINSTR_INTERRUPT		0x0001

/* Instruction Write Pointer Register RW$ */
#define	TE_INSTR_WRITE_PTR_ADDR			0x00040018

/* Instruction Read Pointer Register R$ */
#define	TE_INSTR_READ_PTR_ADDR			0x0004001C

/* Interrupt Enable Register RWSC$ */
#define TE_INTERRUPT_ENABLE_ADDR		0x00040020

#define		TE_INTREN_IMMEDIATE		0x00080000
#define		TE_INTREN_FBCLIP		0x00040000
#define		TE_INTREN_ALUSTAT		0x00020000
#define		TE_INTREN_ZFUNC			0x00010000
#define		TE_INTREN_ANYRENDER		0x00008000
#define		TE_INTREN_SUPERVISOR		0x00004000
#define		TE_INTREN_ILLINSTR		0x00002000
#define		TE_INTREN_SPLINSTR		0x00001000

/* enable TE_INTREN_SPLINSTR for the final release */
#define		TE_INTREN_INIT			(TE_INTREN_SUPERVISOR | \
						TE_INTREN_ILLINSTR )

#define		TE_INTREN_ALL_INTERRUPTS	( TE_INTREN_IMMEDIATE | \
						TE_INTREN_FBCLIP | \
						TE_INTREN_ALUSTAT | \
						TE_INTREN_ZFUNC | \
						TE_INTREN_ANYRENDER | \
						TE_INTREN_SUPERVISOR | \
						TE_INTREN_ILLINSTR | \
						TE_INTREN_SPLINSTR)

/* Interrupt Status Register RWSC$ */
#define	TE_INTERRUPT_STATUS_ADDR		0x00040024

#define		TE_INTRSTAT_IMMEDIATE		0x00080000
#define		TE_INTRSTAT_FBCLIP		0x00040000
#define		TE_INTRSTAT_ALUSTAT		0x00020000
#define		TE_INTRSTAT_ZFUNC		0x00010000
#define		TE_INTRSTAT_ANYRENDER		0x00008000
#define		TE_INTRSTAT_SUPERVISOR		0x00004000
#define		TE_INTRSTAT_ILLINSTR		0x00002000
#define		TE_INTRSTAT_SPLINSTR		0x00001000
#define		TE_INTRSTAT_WINCLIP		0x00000800
#define		TE_INTRSTAT_LISTEND		0x00000400
#define		TE_INTRSTAT_IMMINSTR		0x00000200
#define		TE_INTRSTAT_DEFINSTR		0x00000100
#define		TE_INTRSTAT_DEFINSTR_VECTOR	0x000000FF

#define		TE_INTRSTAT_ALL_INTERRUPTS	( TE_INTRSTAT_IMMEDIATE | \
					 TE_INTRSTAT_FBCLIP | \
					TE_INTRSTAT_ALUSTAT | \
					TE_INTRSTAT_ZFUNC | \
					TE_INTRSTAT_ANYRENDER | \
					TE_INTRSTAT_SUPERVISOR | \
					TE_INTRSTAT_ILLINSTR | \
					TE_INTRSTAT_SPLINSTR | \
					TE_INTRSTAT_WINCLIP | \
					TE_INTRSTAT_LISTEND | \
					TE_INTRSTAT_IMMINSTR | \
					TE_INTRSTAT_DEFINSTR | \
					TE_INTRSTAT_DEFINSTR_VECTOR)

#define		TE_INTRSTAT_INVALID	(TE_INTRSTAT_ILLINSTR | \
					 TE_INTRSTAT_LISTEND | \
					TE_INTRSTAT_SUPERVISOR | \
					TE_INTRSTAT_DEFINSTR )

/* Vertex Control Register RWSC$ */
#define TE_VERTEX_CONTROL_ADDR			0x00040028

#define		TE_VRTXCNTRL_SNOOP_ENABLE	0x00000001

/********************************
 * SetUp Engine			*
 ********************************/

#define TE_SETUP_START_ADDR			0x00042000
#define TE_SETUP_END_ADDR			0x000420BC
#define TE_SETUP_VERTEXSTATE			0x00042100

/********************************
 * Edge Walker/Span Walker 	*
 ********************************/

#define TE_ESWALKER_CONTROL			0x00044000
#define TE_ESWALKER_CAPADDR			0x00044004

/********************************
 * Texture Mapper	 	*
 ********************************/

#define TE_TXT_START_ADDR			0x00046400
#define TE_TXT_END_ADDR				0x00046450

#define TE_TRAM_START_ADDR			0x000C0000
#define TE_TRAM_END_ADDR			0x000C3FFF

#define TE_PIPRAM_START_ADDR			0x00046000
#define TE_PIPRAM_END_ADDR			0x000463FF

#define TE_TXT_MASTER_CONTROL_ADDR		0x00046400

#define		TE_TXT_MC_LOOKUP_TRAM_ON	0x00000001
#define		TE_TXT_MC_LOADER_TRAM_ON	0x00000002
#define		TE_TXT_MC_TRAM_DEST_ON		0x00000004
#define		TE_TXT_MC_PIPRAM_DEST_ON	0x00000008
#define		TE_TXT_MC_SNOOP_ON		0x00000020

#define 	TE_TXT_MC_ENABLE		( TE_TXT_MC_TRAM_DEST_ON | \
						TE_TXT_MC_TRAM_DEST_ON )


/********************************
 * Destination Blender		*
 ********************************/

#define TE_DBLEND_START_ADDR			0x00048000
#define TE_DBLEND_END_ADDR			0x00048088

#define TE_DBLNDR_SNOOP_CNTRL_ADDR		0x00048000

#define		TE_DBLNDRSC_ZREAD		0x00000008
#define		TE_DBLNDRSC_ZWRITE		0x00000004
#define		TE_DBLNDRSC_SOURCE_READ		0x00000002
#define		TE_DBLNDRSC_DEST_WRITE		0x00000001

#define		TE_DBLNDRSC_ENABLE		(TE_DBLNDRSC_ZREAD | \
						TE_DBLNDRSC_ZWRITE | \
						TE_DBLNDRSC_SOURCE_READ | \
						TE_DBLNDRSC_DEST_WRITE )

#define	TE_DBLNDR_SCNTRL_ADDR			0x00048004

#define		TE_DBLNDRSCNTRL_FBEN		0x00000001
#define		TE_DBLNDRSCNTRL_WRFB16EN	0x00000002
#define		TE_DBLNDRSCNTRL_WRZ16EN		0x00000004

#define	TE_DBLNDRSTATUS_ADDR			0x00048010

#define		TE_DBLNDRSTATUS_FBCLIP		 0x00008000
#define 	TE_DBLNDRSTATUS_WINCLIP		 0x00004000
#define		TE_DBLNDRSTATUS_ZCLIP		 0x00002000
#define		TE_DBLNDRSTATUS_ALUSTAT_RED_LT   0x00001000
#define		TE_DBLNDRSTATUS_ALUSTAT_RED_EQ   0x00000800
#define		TE_DBLNDRSTATUS_ALUSTAT_RED_GT   0x00000400
#define		TE_DBLNDRSTATUS_ALUSTAT_GREEN_LT 0x00000200
#define		TE_DBLNDRSTATUS_ALUSTAT_GREEN_EQ 0x00000100
#define		TE_DBLNDRSTATUS_ALUSTAT_GREEN_GT 0x00000080
#define		TE_DBLNDRSTATUS_ALUSTAT_BLUE_LT  0x00000040
#define		TE_DBLNDRSTATUS_ALUSTAT_BLUE_EQ  0x00000020
#define		TE_DBLNDRSTATUS_ALUSTAT_BLUE_GT  0x00000010
#define		TE_DBLNDRSTATUS_ZFUNC_LT         0x00000008
#define		TE_DBLNDRSTATUS_ZFUNC_EQ         0x00000004
#define		TE_DBLNDRSTATUS_ZFUNC_GT         0x00000002
#define		TE_DBLNDRSTATUS_ANYREND		 0x00000001

/* composites */
#define		TE_DBLNDRSTATUS_ALUSTAT		0x00001FF0
#define		TE_DBLNDRSTATUS_ZFUNC		0x0000000E

/* blender interrupt enable register RWSC$ */
#define TE_DBLNDR_INTERRUPT_ENABLE_ADDR		0x00048014

#define		TE_DBLNDR_INTREN_FBCLIP			0x80000000
#define		TE_DBLNDR_INTREN_WINCLIP		0x40000000
#define		TE_DBLNDR_INTREN_ALUSTAT		0x20000000
#define		TE_DBLNDR_INTREN_ZFUNC			0x10000000
#define		TE_DBLNDR_INTREN_ANYRENDER		0x08000000

/*
 * Z Buffer
 */

/* Z Buffer Base Address */
#define TE_ZBUFFER_BASE_ADDR			0x00048044

/* Z Buffer Clip Values */
#define TE_ZBUFFER_CLIP_ADDR			0x0004804C

#define		TE_ZBFR_XLIP_MASK		0x07FF0000
#define		TE_ZBFR_YLIP_MASK		0x000007FF
#define		TE_ZBFR_XLIP_SHIFT		16
#define		TE_ZBFR_YLIP_SHIFT		0

/*
 * Frame Buffer
 */

/* Frame Buffer Clip Values */
#define	TE_FBUFFER_CLIP_ADDR			0x00048018

#define		TE_FBFR_XLIP_MASK		0x07FF0000
#define		TE_FBFR_YLIP_MASK		0x000007FF
#define		TE_FBFR_XLIP_SHIFT		16
#define		TE_FBFR_YLIP_SHIFT		0

/* Frame Buffer Control Register */
#define TE_FBUFFER_CNTRL_ADDR			0x00048024

#define		TE_FBFR_BPPIXEL_MASK		0x00000001

/* definitions for GFX_TAG_FB_BPPIXEL tag argument */
#define BITSPERPIXEL_16 0x00000000      /* 16 bits per pixel */
#define BITSPERPIXEL_32 0x00000001      /* 32 bits per pixel */


/* Frame Buffer Base Address */
#define TE_FBUFFER_BASE_ADDR			0x00048028

/* Frame Buffer Width */
#define TE_FBUFFER_WIDTH_ADDR			0x0004802C

#define 	TE_FBFR_WIDTH_MASK		0x000007FF




#define  TE_INTERRUPT 0

#endif /* __HARDWARE_TE_H */
