#ifndef	__HARDWARE_PPCASM_H
#define __HARDWARE_PPCASM_H


/******************************************************************************
**
**  @(#) PPCasm.h 96/09/18 1.20
**
******************************************************************************/


#ifndef	__HARDWARE_PPC_H
#include <hardware/PPC.h>
#endif

#ifdef __DCC__

__asm uint32 _mfsr14(void)
{
        mfsr    r3,SR14
}

__asm uint32 _mtsr14(uint32 val)
{
%       reg     val
        mtsr    SR14,val
}

__asm uint32 _mfsr13(void)
{
        mfsr    r3,SR13
}

__asm uint32 _mtsr13(uint32 val)
{
%       reg     val
        mtsr    SR13,val
}

__asm uint32 _mfsr12(void)
{
        mfsr    r3,SR12
}

__asm uint32 _mtsr12(uint32 val)
{
%       reg     val
        mtsr    SR12,val
}

__asm uint32 _mfsr11(void)
{
        mfsr    r3,SR11
}

__asm uint32 _mtsr11(uint32 val)
{
%       reg     val
        mtsr    SR11,val
}

__asm uint32 _mfsr10(void)
{
        mfsr    r3,SR10
}

__asm uint32 _mtsr10(uint32 val)
{
%       reg     val
        mtsr    SR10,val
}

__asm uint32 _mfdmiss(void)
{
	mfspr	r3,DMISS
}

__asm uint32 _mfhid0(void)
{
	mfspr	r3,HID0
}

__asm void _mthid0(uint32 val)
{
%	reg val;
	mtspr HID0,val
	isync
}

__asm uint32 _mfibr(void)
{
	mfspr	r3,IBR
}

__asm void _mtsprg0(uint32 val)
{
%	reg val;
	mtsprg 0,val
}

__asm void _mtsprg1(uint32 val)
{
%	reg val;
	mtsprg 1,val
}

__asm void _mtsprg2(uint32 val)
{
%	reg val;
	mtsprg 2,val
}

__asm void _mtsprg3(uint32 val)
{
%	reg val;
	mtsprg 3,val
}

__asm void _sync(void)
{
	sync
}

__asm void _isync(void)
{
	isync
}

__asm void _esa(void)
{
        .long   0x7c0004a8
}

__asm void _dsa(void)
{
        .long   0x7c0004e8
}
__asm uint32 _mfesasrr(void)
{
        mfspr   r3,ESASRR
}

__asm void _mtesasrr(uint32 val)
{
%	reg val;
        mtspr   ESASRR,val
}

/* IBAT control routines */

__asm long long _mfibat0(void)
{
	mfspr	r3,IBAT0U
	mfspr	r4,IBAT0L
}
__asm long long _mfibat1(void)
{
	mfspr	r3,IBAT1U
	mfspr	r4,IBAT1L
}
__asm long long _mfibat2(void)
{
	mfspr	r3,IBAT2U
	mfspr	r4,IBAT2L
}
__asm long long _mfibat3(void)
{
	mfspr	r3,IBAT3U
	mfspr	r4,IBAT3L
}
__asm void _mtibat0(uint32 upper, uint32 lower)
{
%	reg	upper;	reg	lower;
	mtspr	IBAT0U,upper
	mtspr	IBAT0L,lower
}
__asm void _mtibat1(uint32 upper, uint32 lower)
{
%	reg	upper;	reg	lower;
	mtspr	IBAT1U,upper
	mtspr	IBAT1L,lower
}
__asm void _mtibat2(uint32 upper, uint32 lower)
{
%	reg	upper;	reg	lower;
	mtspr	IBAT2U,upper
	mtspr	IBAT2L,lower
}
__asm void _mtibat3(uint32 upper, uint32 lower)
{
%	reg	upper;	reg	lower;
	mtspr	IBAT3U,upper
	mtspr	IBAT3L,lower
}

/* DBAT control routines */

__asm long long _mfdbat0(void)
{
	mfspr	r3,DBAT0U
	mfspr	r4,DBAT0L
}
__asm long long _mfdbat1(void)
{
	mfspr	r3,DBAT1U
	mfspr	r4,DBAT1L
}
__asm long long _mfdbat2(void)
{
	mfspr	r3,DBAT2U
	mfspr	r4,DBAT2L
}
__asm long long _mfdbat3(void)
{
	mfspr	r3,DBAT3U
	mfspr	r4,DBAT3L
}
__asm void _mtdbat0(uint32 upper, uint32 lower)
{
%	reg	upper;	reg	lower;
	mtspr	DBAT0U,upper
	mtspr	DBAT0L,lower
}
__asm void _mtdbat1(uint32 upper, uint32 lower)
{
%	reg	upper;	reg	lower;
	mtspr	DBAT1U,upper
	mtspr	DBAT1L,lower
}
__asm void _mtdbat2(uint32 upper, uint32 lower)
{
%	reg	upper;	reg	lower;
	mtspr	DBAT2U,upper
	mtspr	DBAT2L,lower
}
__asm void _mtdbat3(uint32 upper, uint32 lower)
{
%	reg	upper;	reg	lower;
	mtspr	DBAT3U,upper
	mtspr	DBAT3L,lower
}

__asm uint32 _mftbl(void)
{
	mftb	r3
}

__asm uint32 _mftbu(void)
{
	mftbu	r3
}

__asm uint32 _mfsp(void)
{
	mfspr	r3,fSP
}

__asm void _mtsp(uint32 val)
{
%	reg val;
	mtspr	fSP,val
}

__asm uint32 _mfsprg0(void)
{
	mfsprg	r3,0
}

__asm uint32 _mfsprg1(void)
{
	mfsprg	r3,1
}

__asm uint32 _mfsprg2(void)
{
	mfsprg	r3,2
}

__asm uint32 _mfsprg3(void)
{
	mfsprg	r3,3
}

__asm uint32 _mflt(void)
{
	mfspr	r3,LT
}

__asm void _mtlt(uint32 val)
{
%	reg val;
	mtspr	LT,val
	isync
}

__asm uint32 _mfpvr(void)
{
	mfspr	r3,PVR
}

__asm uint32 _mfmsr(void)
{
	mfmsr	r3
}

__asm void _mtmsr(uint32 val)
{
%	reg val;
	mtmsr	val
	isync
}

__asm uint32 _mfdec(void)
{
	mfdec	r3
}

__asm void _mtdec(uint32 val)
{
%	reg val;
	mtdec	val
}

__asm void _mttbu(uint32 val)
{
%	reg val;
	mttbu	val
}

__asm void _mttbl(uint32 val)
{
%	reg val;
	mttbl	val
}

__asm void _mtdsisr(uint32 val)
{
%	reg val;
	mtspr	DSISR,val
}

__asm uint32 _mfdsisr(void)
{
	mfspr	r3,DSISR
}

__asm uint32 _mfdar(void)
{
	mfspr	r3,DAR
}

__asm void _mtdar(uint32 val)
{
%	reg val;
	mtspr	DAR,val
}

__asm void _mttcr(uint32 val)
{
%	reg val;
	mtspr	TCR,val
}

__asm uint32 _mftcr(void)
{
	mfspr	r3,TCR
}

__asm uint32 _mfsrr1(void)
{
	mfsrr1	r3
}

__asm uint32 _mfsrr0(void)
{
	mfsrr0	r3
}

__asm void _mtsrr0(uint32 val)
{
%	reg val;
	mtsrr0	val
}

__asm void _mtsrr1(uint32 val)
{
%	reg val;
	mtsrr1	val
}

__asm void _mtser(uint32 val)
{
%	reg val;
	mtspr	SER,val
}

__asm uint32 _mfser(void)
{
	mfspr	r3,SER
}

__asm void _mtsebr(uint32 val)
{
%	reg val;
	mtspr	SEBR,val
}

__asm uint32 _mfsebr(void)
{
	mfspr	r3,SEBR
}

/* no leading _ for historical reasons... */
__asm void eieio(void)
{
	eieio
}

__asm void _dcbf(void *addr)
{
%	reg addr;
	dcbf	0,addr
}

__asm void _dcbst(void *addr)
{
%	reg addr;
	dcbst	0,addr
}

__asm void _dcbt(void *addr)
{
%	reg addr;
	dcbt	0,addr
}

__asm void _dcbz(void *addr)
{
%	reg addr;
	dcbz	0,addr
}

__asm void _dcbi(void *addr)
{
%	reg addr;
	dcbi	0,addr
}

__asm void _dcbtst(void *addr)
{
%	reg addr;
	dcbtst	0,addr
}

/* This function clears the following bits from FPSCR:
 *
 *   FPSCR_FX
 *   FPSCR_OX
 *   FPSCR_UX
 *   FPSCR_ZX
 *   FPSCR_XX
 *   FPSCR_VXNAN
 *   FPSCR_VXISI
 *   FPSCR_VXIDI
 *   FPSCR_VXZDZ
 *   FPSCR_VXIMZ
 *   FPSCR_VXVC
 *   FPSCR_VXSOFT
 *   FPSCR_VXSQRT
 *   FPSCR_VXCVI
 */
__asm void _ClrFPExc(void)
{
	mtfsb0	0
	mtfsb0	3
	mtfsb0	4
	mtfsb0	5
	mtfsb0	6
	mtfsb0	7
	mtfsb0	8
	mtfsb0	9
	mtfsb0	10
	mtfsb0	11
	mtfsb0	12
	mtfsb0	21
	mtfsb0	22
	mtfsb0	23
}

__asm void _EnableVE(void)
{
	mtfsb1  24
}

__asm void _EnableOE(void)
{
	mtfsb1  25
}

__asm void _EnableUE(void)
{
	mtfsb1  26
}

__asm void _EnableZE(void)
{
	mtfsb1  27
}

__asm void _EnableXE(void)
{
	mtfsb1  28
}

__asm void _DisableVE(void)
{
	mtfsb0  24
}

__asm void _DisableOE(void)
{
	mtfsb0  25
}

__asm void _DisableUE(void)
{
	mtfsb0  26
}

__asm void _DisableZE(void)
{
	mtfsb0  27
}

__asm void _DisableXE(void)
{
	mtfsb0  28
}

#endif /* __DCC__ */

#endif /* __HARDWARE_PPCASM_H */
