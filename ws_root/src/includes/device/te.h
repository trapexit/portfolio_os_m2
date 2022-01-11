#ifndef __DEVICE_TE_H
#define __DEVICE_TE_H


/******************************************************************************
**
**  @(#) te.h 96/08/21 1.5
**
**  Triangle engine interface definitions.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif


/*****************************************************************************/


/* Options for the TE_CMD_EXECUTELIST command */
#define TE_CLEAR_FRAME_BUFFER		0x00000001
#define TE_CLEAR_Z_BUFFER		0x00000002
#define TE_WAIT_UNTIL_OFFSCREEN		0x00000004
#define TE_ABORT_AT_VBLANK		0x00000008
#define TE_LIST_FLUSHED			0x00000010


/*****************************************************************************/


/* Render status, returned in io_Actual for TE_CMD_EXECUTELIST commands */
#define TE_STATUS_ANYRENDER        0x00000001 /* rendering occured           */
#define TE_STATUS_ZFUNC_GT         0x00000002 /* z depth > current Z value   */
#define TE_STATUS_ZFUNC_EQ         0x00000004 /* z depth == current Z value  */
#define TE_STATUS_ZFUNC_LT         0x00000008 /* z depth < current Z value   */
#define TE_STATUS_ALUSTAT_BLUE_GT  0x00000010 /* blue component > 0          */
#define TE_STATUS_ALUSTAT_BLUE_EQ  0x00000020 /* blue component == 0         */
#define TE_STATUS_ALUSTAT_BLUE_LT  0x00000040 /* blue component < 0          */
#define TE_STATUS_ALUSTAT_GREEN_GT 0x00000080 /* green component > 0         */
#define TE_STATUS_ALUSTAT_GREEN_EQ 0x00000100 /* green component == 0        */
#define TE_STATUS_ALUSTAT_GREEN_LT 0x00000200 /* green component < 0         */
#define TE_STATUS_ALUSTAT_RED_GT   0x00000400 /* red component > 0           */
#define TE_STATUS_ALUSTAT_RED_EQ   0x00000800 /* red component == 0          */
#define TE_STATUS_ALUSTAT_RED_LT   0x00001000 /* red component < 0           */
#define TE_STATUS_ZBUFFERCLIP      0x00002000 /* z buffer discard occured    */
#define TE_STATUS_WINDOWCLIP       0x00004000 /* window clipping occured     */
#define TE_STATUS_FRAMEBUFFERCLIP  0x00008000 /* frame buffer clipping       */


/*****************************************************************************/


/* for use with TE_CMD_SETFRAMEBUFFER */
typedef struct TEFrameBufferInfo
{
    float32 tefbi_Red;
    float32 tefbi_Green;
    float32 tefbi_Blue;
    float32 tefbi_Alpha;
} TEFrameBufferInfo;

/* for use with TE_CMD_SETZBUFFER */
typedef struct TEZBufferInfo
{
    float32 tezbi_FillValue;
} TEZBufferInfo;

/* For use with TE_CMD_SPEEDCONTROL */
#define TE_FULLSPEED 0
#define TE_STOPPED   0xffffffff


/*****************************************************************************/


/* Error codes */

#define MakeTEErr(svr,class,err) MakeErr(ER_DEVC,ER_TE,svr,ER_E_SSTM,class,err)

#define TE_ERR_NOMEM         MakeTEErr(ER_SEVERE,ER_C_STND,ER_NoMem)
#define TE_ERR_ABORTED       MakeTEErr(ER_SEVERE,ER_C_STND,ER_Aborted)
#define TE_ERR_BADITEM       MakeTEErr(ER_SEVERE,ER_C_STND,ER_BadItem)
#define TE_ERR_BADPTR        MakeTEErr(ER_SEVERE,ER_C_STND,ER_BadPtr)
#define TE_ERR_BADIOARG      MakeTEErr(ER_SEVERE,ER_C_STND,ER_BadIOArg)
#define TE_ERR_TIMEOUT       MakeTEErr(ER_SEVERE,ER_C_NSTND,1)
#define TE_ERR_ILLEGALINSTR  MakeTEErr(ER_SEVERE,ER_C_NSTND,2)
#define TE_ERR_NOLIST        MakeTEErr(ER_SEVERE,ER_C_NSTND,3)
#define TE_ERR_BADOPTIONS    MakeTEErr(ER_SEVERE,ER_C_NSTND,4)
#define TE_ERR_BADBITMAP     MakeTEErr(ER_SEVERE,ER_C_NSTND,5)
#define TE_ERR_NOFRAMEBUFFER MakeTEErr(ER_SEVERE,ER_C_NSTND,6)
#define TE_ERR_NOTPAUSED     MakeTEErr(ER_SEVERE,ER_C_NSTND,7)
#define TE_ERR_NOVIEW        MakeTEErr(ER_SEVERE,ER_C_NSTND,8)


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


Item OpenTEDevice(void);
#define CloseTEDevice(dev) CloseDeviceStack(dev)

void  SetTEReadPointer(void *ptr);
void *GetTEReadPointer(void);
void  SetTEWritePointer(void *ptr);
void *GetTEWritePointer(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __DEVICE_TE_H */
