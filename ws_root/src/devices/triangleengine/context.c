/* @(#) context.c 96/05/20 1.2 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <hardware/te.h>
#include <device/te.h>
#include <string.h>
#include <stdio.h>
#include "driver.h"
#include "hw.h"
#include "context.h"


/*****************************************************************************/


#define TRACE(x) /* printf x */


/*****************************************************************************/


typedef struct TEContext
{
    uint32 tc_MasterMode;
    uint32 tc_InterruptEnable;
    uint32 tc_SetupVertexState;
    uint32 tc_ESWalkerControl;
    uint32 tc_ESWalkerCapAddr;
    uint32 tc_SetupEngine[(TE_SETUP_END_ADDR - TE_SETUP_START_ADDR + 1) / sizeof(uint32)];
    uint32 tc_TextureMapper[(TE_TXT_END_ADDR - TE_TXT_START_ADDR + 1) / sizeof(uint32)];
    uint32 tc_DestinationBlender[(TE_DBLEND_END_ADDR - TE_DBLEND_START_ADDR + 1) / sizeof(uint32)];
    uint32 tc_TRAM[(TE_TRAM_END_ADDR - TE_TRAM_START_ADDR + 1) / sizeof(uint32)];
    uint32 tc_PIPRAM[(TE_PIPRAM_END_ADDR - TE_PIPRAM_START_ADDR + 1) / sizeof(uint32)];
    bool   tc_ContextValid;
} TEContext;


/*****************************************************************************/


Err AllocTEContext(DeviceState *devState)
{
    TRACE(("AllocTEContext: entering\n"));

    devState->ds_TEContext = SuperAllocMem(sizeof(TEContext), MEMTYPE_NORMAL);
    if (devState->ds_TEContext == NULL)
        return TE_ERR_NOMEM;

    devState->ds_TEContext->tc_ContextValid = FALSE;
    return 0;
}


/*****************************************************************************/


void FreeTEContext(DeviceState *devState)
{
    TRACE(("FreeTEContext: entering\n"));

    SuperFreeMem(devState->ds_TEContext, sizeof(TEContext));
    devState->ds_TEContext = NULL;
}


/*****************************************************************************/


void SaveTEContext(const DeviceState *devState)
{
TEContext *ctx;
uint32     reg;
uint32	   i;

    TRACE(("SaveTEContext: entering with devState 0x%x\n", devState));

    ctx                      = devState->ds_TEContext;
    ctx->tc_MasterMode       = ReadRegister(TE_MASTER_MODE_ADDR);
    ctx->tc_InterruptEnable  = ReadRegister(TE_INTERRUPT_ENABLE_ADDR);
    ctx->tc_SetupVertexState = ReadRegister(TE_SETUP_VERTEXSTATE);
    ctx->tc_ESWalkerControl  = ReadRegister(TE_ESWALKER_CONTROL);
    ctx->tc_ESWalkerCapAddr  = ReadRegister(TE_ESWALKER_CAPADDR);
    ctx->tc_ContextValid     = TRUE;

    reg = TE_SETUP_START_ADDR;
    for (i = 0; i < (sizeof(ctx->tc_SetupEngine) / sizeof(uint32)); i++)
    {
        ctx->tc_SetupEngine[i] = ReadRegister(reg);
        reg += sizeof(uint32);
    }

    reg = TE_TXT_START_ADDR;
    for (i = 0; i < (sizeof(ctx->tc_TextureMapper) / sizeof(uint32)); i++)
    {
        ctx->tc_TextureMapper[i] = ReadRegister(reg);
        reg += sizeof(uint32);
    }

    reg = TE_DBLEND_START_ADDR;
    for (i = 0; i < (sizeof(ctx->tc_DestinationBlender) / sizeof(uint32)); i++)
    {
        ctx->tc_DestinationBlender[i] = ReadRegister(reg);
        reg += sizeof(uint32);
    }

    reg = TE_TRAM_START_ADDR;
    for (i = 0; i < (sizeof(ctx->tc_TRAM) / sizeof(uint32)); i++)
    {
        ctx->tc_TRAM[i] = ReadRegister(reg);
        reg += sizeof(uint32);
    }

    reg = TE_PIPRAM_START_ADDR;
    for (i = 0; i < (sizeof(ctx->tc_PIPRAM) / sizeof(uint32)); i++)
    {
        ctx->tc_PIPRAM[i] = ReadRegister(reg);
        reg += sizeof(uint32);
    }
}


/*****************************************************************************/


void RestoreTEContext(const DeviceState *devState)
{
TEContext *ctx;
uint32     reg;
uint32	   i;

    TRACE(("RestoreTEContext: entering with devState 0x%x\n", devState));

    ctx = devState->ds_TEContext;
    if (!ctx || !ctx->tc_ContextValid)
    {
        InitTE();
        return;
    }

    WriteRegister(TE_MASTER_MODE_ADDR,      ctx->tc_MasterMode);
    WriteRegister(TE_INTERRUPT_ENABLE_ADDR, ctx->tc_InterruptEnable);
    WriteRegister(TE_SETUP_VERTEXSTATE,     ctx->tc_SetupVertexState);
    WriteRegister(TE_ESWALKER_CONTROL,      ctx->tc_ESWalkerControl);
    WriteRegister(TE_ESWALKER_CAPADDR,      ctx->tc_ESWalkerCapAddr);

    reg = TE_SETUP_START_ADDR;
    for (i = 0; i < (sizeof(ctx->tc_SetupEngine) / sizeof(uint32)); i++)
    {
        WriteRegister(reg, ctx->tc_SetupEngine[i]);
        reg += sizeof(uint32);
    }

    reg = TE_TXT_START_ADDR;
    for (i = 0; i < (sizeof(ctx->tc_TextureMapper) / sizeof(uint32)); i++)
    {
        WriteRegister(reg, ctx->tc_TextureMapper[i]);
        reg += sizeof(uint32);
    }

    reg = TE_DBLEND_START_ADDR;
    for (i = 0; i < (sizeof(ctx->tc_DestinationBlender) / sizeof(uint32)); i++)
    {
        WriteRegister(reg, ctx->tc_DestinationBlender[i]);
        reg += sizeof(uint32);
    }

    reg = TE_TRAM_START_ADDR;
    for (i = 0; i < (sizeof(ctx->tc_TRAM) / sizeof(uint32)); i++)
    {
        WriteRegister(reg, ctx->tc_TRAM[i]);
        reg += sizeof(uint32);
    }

    reg = TE_PIPRAM_START_ADDR;
    for (i = 0; i < (sizeof(ctx->tc_PIPRAM) / sizeof(uint32)); i++)
    {
        WriteRegister(reg, ctx->tc_PIPRAM[i]);
        reg += sizeof(uint32);
    }
}
