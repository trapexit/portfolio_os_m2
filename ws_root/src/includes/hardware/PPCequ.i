#ifndef __HARDWARE_PPCEQU_I
#define __HARDWARE_PPCEQU_I


/******************************************************************************
**
**  @(#) PPCequ.i 96/04/24 1.26
**
**  Common PowerPC equates and macros.
**
******************************************************************************/



/* MSR bit definitions */
#define MSR_AP		0x00800000
#define MSR_SA		0x00400000
#define MSR_POW		0x00040000
#define MSR_TGPR	0x00020000
#define MSR_ILE		0x00010000
#define MSR_EE		0x00008000
#define MSR_PR		0x00004000
#define MSR_FP		0x00002000
#define MSR_ME		0x00001000
#define MSR_FE0		0x00000800
#define MSR_SE		0x00000400
#define MSR_BE		0x00000200
#define MSR_FE1		0x00000100
#define MSR_IP		0x00000040
#define MSR_IR		0x00000020
#define MSR_DR		0x00000010
#define MSR_RI		0x00000002
#define MSR_LE		0x00000001

/* Various SPRs */
#define DSISR	18
#define DAR	19
#define SDR1	25
#define PVR	287
#define IBAT0U	528
#define IBAT0L	529
#define IBAT1U	530
#define IBAT1L	531
#define IBAT2U	532
#define IBAT2L	533
#define IBAT3U	534
#define IBAT3L	535
#define DBAT0U	536
#define DBAT0L	537
#define DBAT1U	538
#define DBAT1L	539
#define DBAT2U	540
#define DBAT2L	541
#define DBAT3U	542
#define DBAT3L	543
#define DMISS	976
#define DCMP	977
#define HASH1	978
#define HASH2	979
#define IMISS	980
#define ICMP	981
#define RPA	982
// Diab compiler is broken
#define TCR	984
#define IBR	986
#define ESASRR	987
#define HID0	1008
#define HID1	1009
#define IABR	1010
#define fSP	1021    /* We use this name because SP is taken */
#define LT	1022

/* HID bit definitions */
#define HID_DCI		0x00000400
#define HID_ICFI	0x00000800
#define HID_DLOCK	0x00001000
#define HID_ILOCK	0x00002000
#define HID_DCE		0x00004000
#define HID_ICE		0x00008000
#define HID_NHR		0x00010000
#define HID_DPM		0x00100000
#define HID_SLEEP	0x00200000
#define HID_NAP		0x00400000
#define HID_DOZE	0x00800000
#define HID_XMODE	0x00000080
#define HID_SL		0x00000020
#define HID_EMCP	0x80000000
#define HID_WIMG	0x0000000F
#define HID_WIMG_WRTHU	0x00000008
#define HID_WIMG_INHIB	0x00000004
#define HID_WIMG_MCOHR	0x00000002
#define HID_WIMG_GUARD	0x00000001

/* UBAT definitions */
#define UBAT_BL_256M	0x00001FFC
#define UBAT_BL_128M	0x00000FFC
#define UBAT_BL_64M	0x000007FC
#define UBAT_BL_32M	0x000003FC
#define UBAT_BL_16M	0x000001FC
#define UBAT_BL_8M	0x000000FC
#define UBAT_BL_4M	0x0000007C
#define UBAT_BL_2M	0x0000003C
#define UBAT_BL_1M	0x0000001C
#define UBAT_BL_512K	0x0000000C
#define UBAT_BL_256K	0x00000004
#define UBAT_BL_128K	0x00000000
#define UBAT_VS		0x00000002
#define UBAT_VP		0x00000001

/* LBAT definitions */
#define LBAT_NE		0x00000400
#define LBAT_SE		0x00000200
#define LBAT_WIMG_WRTHU	0x00000040
#define LBAT_WIMG_INHIB	0x00000020
#define LBAT_WIMG_MCOHR	0x00000010
#define LBAT_WIMG_GUARD	0x00000008
#define LBAT_PP_READWR	0x00000002
#define LBAT_PP_READ	0x00000001
#define LBAT_PP_NOACC	0x00000000

// PPC-mode
#define LPTE_NE		0x00000400
#define LPTE_SE		0x00000200
#define LPTE_C		0x00000080
#define LPTE_W		0x00000040
#define LPTE_I		0x00000020
#define LPTE_M		0x00000010
#define LPTE_G		0x00000008
#define LPTE_READONLY	3
#define LPTE_READWRITE	2

// X-mode

#define NSEGREGS        16

#define XITLB_EXEC_OK   0
#define XITLB_NO_EXEC   1

#define XDTLB_WRITE_OK  1
#define XDTLB_NO_WRITE  0

#define XMODE_PAGESIZE          0x1000
#define XMODE_PAGEMASK          0xFFF
#define XMODE_PAGESHIFT         12

#define XMODE_REGIONSIZE        0x20000
#define XMODE_REGIONMASK        0x1FFFF
#define XMODE_REGIONSHIFT       17

#define XMODE_NTLBS             32

#define ESASRR_EE 0x00000001
#define ESASRR_SA 0x00000002
#define ESASRR_AP 0x00000004
#define ESASRR_PR 0x00000008

#define SRR1_WAY	0x00020000

#define ICMP_V		0x80000000

#define DCMP_V		0x80000000


#define PPC602ID	0x00050000

#endif /* __HARDWARE_PPCEQU_I */
