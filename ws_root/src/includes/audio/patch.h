#ifndef __AUDIO_PATCH_H
#define __AUDIO_PATCH_H


/****************************************************************************
**
**  @(#) patch.h 96/07/17 1.16
**
**  AudioPatch Folio
**
****************************************************************************/


#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/* -------------------- PatchCmd */

    /* forward typedef of PatchCmd union */
#ifdef __cplusplus
    union PatchCmd;
#else
    typedef union PatchCmd PatchCmd;
#endif

typedef struct PatchCmdAddTemplate {
    uint32      pc_CmdID;           /* PATCH_CMD_ADD_TEMPLATE */
    const char *pc_BlockName;
    Item        pc_InsTemplate;
    uint32      pc_Pad3;
    uint32      pc_Pad4;
    uint32      pc_Pad5;
    uint32      pc_Pad6;
} PatchCmdAddTemplate;

typedef struct PatchCmdDefinePort {
    uint32      pc_CmdID;           /* PATCH_CMD_DEFINE_PORT */
    const char *pc_PortName;
    uint32      pc_NumParts;
    uint32      pc_PortType;        /* AF_PORT_TYPE_ value */
    uint32      pc_SignalType;      /* AF_SIGNAL_TYPE value */
    uint32      pc_Pad5;
    uint32      pc_Pad6;
} PatchCmdDefinePort;

typedef struct PatchCmdDefineKnob {
    uint32      pc_CmdID;           /* PATCH_CMD_DEFINE_KNOB */
    const char *pc_KnobName;
    uint32      pc_NumParts;
    uint32      pc_KnobType;        /* AF_SIGNAL_TYPE value */
    float32     pc_DefaultValue;
    uint32      pc_Pad5;
    uint32      pc_Pad6;
} PatchCmdDefineKnob;

typedef struct PatchCmdExpose {
    uint32      pc_CmdID;           /* PATCH_CMD_EXPOSE */
    const char *pc_PortName;
    const char *pc_SrcBlockName;
    const char *pc_SrcPortName;
    uint32      pc_Pad4;
    uint32      pc_Pad5;
    uint32      pc_Pad6;
} PatchCmdExpose;

typedef struct PatchCmdConnect {
    uint32      pc_CmdID;           /* PATCH_CMD_CONNECT */
    const char *pc_FromBlockName;
    const char *pc_FromPortName;
    uint32      pc_FromPartNum;
    const char *pc_ToBlockName;
    const char *pc_ToPortName;
    uint32      pc_ToPartNum;
} PatchCmdConnect;

typedef struct PatchCmdSetConstant {
    uint32      pc_CmdID;           /* PATCH_CMD_SET_CONSTANT */
    const char *pc_BlockName;
    const char *pc_PortName;
    uint32      pc_PartNum;
    float32     pc_ConstantValue;
    uint32      pc_Pad5;
    uint32      pc_Pad6;
} PatchCmdSetConstant;

typedef struct PatchCmdSetCoherence {
    uint32      pc_CmdID;           /* PATCH_CMD_SET_COHERENCE */
    uint32      pc_State;           /* TRUE to set, FALSE to clear */
    uint32      pc_Pad2;
    uint32      pc_Pad3;
    uint32      pc_Pad4;
    uint32      pc_Pad5;
    uint32      pc_Pad6;
} PatchCmdSetCoherence;

typedef struct PatchCmdJump {
    uint32      pc_CmdID;           /* PATCH_CMD_JUMP */
    const PatchCmd *pc_NextPatchCmd;
    uint32      pc_Pad2;
    uint32      pc_Pad3;
    uint32      pc_Pad4;
    uint32      pc_Pad5;
    uint32      pc_Pad6;
} PatchCmdJump;

typedef struct PatchCmdGeneric {
    uint32      pc_CmdID;
    uint32      pc_Args[6];
} PatchCmdGeneric;

union PatchCmd {
    PatchCmdAddTemplate     pc_AddTemplate;
    PatchCmdDefinePort      pc_DefinePort;
    PatchCmdDefineKnob      pc_DefineKnob;
    PatchCmdExpose          pc_Expose;
    PatchCmdConnect         pc_Connect;
    PatchCmdSetConstant     pc_SetConstant;
    PatchCmdSetCoherence    pc_SetCoherence;
    PatchCmdJump            pc_Jump;
    PatchCmdGeneric         pc_Generic;
};

    /* PatchCmd IDs */
typedef enum PatchCmdID {
    PATCH_CMD_ADD_TEMPLATE = 256,
    PATCH_CMD_DEFINE_PORT,
    PATCH_CMD_DEFINE_KNOB,
    PATCH_CMD_EXPOSE,
    PATCH_CMD_CONNECT,
    PATCH_CMD_SET_CONSTANT,
    PATCH_CMD_SET_COHERENCE
} PatchCmdID;
#ifndef EXTERNAL_RELEASE
/* @@@ these must be kept in sync with enum */
#define PATCH_CMD_MIN   PATCH_CMD_ADD_TEMPLATE
#define PATCH_CMD_MAX   PATCH_CMD_SET_COHERENCE
#endif

    /* control cmds (defined just like TagArg stuff because they are used the same way) */
#define PATCH_CMD_END   TAG_END
#define PATCH_CMD_JUMP  TAG_JUMP
#define PATCH_CMD_NOP   TAG_NOP


/* -------------------- AudioPatch folio error codes */

#define MakeAudioPatchErr(svr,class,err)    MakeErr(ER_FOLI,ER_AUDIOPATCH,svr,ER_E_SSTM,class,err)

    /* standard error codes */
#define PATCH_ERR_BADITEM                   MakeAudioPatchErr(ER_SEVERE,ER_C_STND,ER_BadItem)
#define PATCH_ERR_NOMEM                     MakeAudioPatchErr(ER_SEVERE,ER_C_STND,ER_NoMem)

    /* Invalid name (e.g., contains illegal characters) */
#define PATCH_ERR_BAD_NAME                  MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,1)

    /* Invalid PatchCmd */
#define PATCH_ERR_BAD_PATCH_CMD             MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,2)

    /* Invalid port type */
#define PATCH_ERR_BAD_PORT_TYPE             MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,3)

    /* Invalid signal type */
#define PATCH_ERR_BAD_SIGNAL_TYPE           MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,4)

    /* Version mismatch between patchcmd folio and audio folio */
#define PATCH_ERR_INCOMPATIBLE_VERSION      MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,5)

    /* Named thing not found */
#define PATCH_ERR_NAME_NOT_FOUND            MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,6)

    /* Name is not unique */
#define PATCH_ERR_NAME_NOT_UNIQUE           MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,7)

    /* Name too long (contains more characters than AF_MAX_NAME_LENGTH) */
#define PATCH_ERR_NAME_TOO_LONG             MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,8)

    /* Value out of range */
#define PATCH_ERR_OUT_OF_RANGE              MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,9)

    /* Port already exposed */
#define PATCH_ERR_PORT_ALREADY_EXPOSED      MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,10)

    /* Port already connected */
#define PATCH_ERR_PORT_IN_USE               MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,11)

    /* Port not used */
#define PATCH_ERR_PORT_NOT_USED             MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,12)

    /* Patch requires too many resources */
#define PATCH_ERR_TOO_MANY_RESOURCES        MakeAudioPatchErr(ER_SEVERE,ER_C_NSTND,13)


/* -------------------- PatchCmdBuilder */

typedef struct PatchCmdBuilder PatchCmdBuilder;


/* -------------------- Functions */

#ifdef __cplusplus
extern "C" {
#endif

    /* AudioPatch Folio */
Err OpenAudioPatchFolio (void);
Err CloseAudioPatchFolio (void);

    /* Patch Template */
Item CreatePatchTemplate (const PatchCmd *, const TagArg *);
Item CreatePatchTemplateVA (const PatchCmd *, uint32 tag1, ...);
#define DeletePatchTemplate(patchTemplate) DeleteItem(patchTemplate)

    /* PatchCmdBuilder */
Err CreatePatchCmdBuilder (PatchCmdBuilder **resultBuilder);
void DeletePatchCmdBuilder (PatchCmdBuilder *);
const PatchCmd *GetPatchCmdList (const PatchCmdBuilder *);
Err GetPatchCmdBuilderError (const PatchCmdBuilder *);

Err AddTemplateToPatch (PatchCmdBuilder *, const char *blockName, Item insTemplate);
Err DefinePatchPort (PatchCmdBuilder *, const char *portName, uint32 numParts, uint32 portType, uint32 signalType);
Err DefinePatchKnob (PatchCmdBuilder *, const char *knobName, uint32 numParts, uint32 knobType, float32 defaultValue);
Err ExposePatchPort (PatchCmdBuilder *, const char *portName, const char *srcBlockName, const char *srcPortName);
Err ConnectPatchPorts (PatchCmdBuilder *, const char *fromBlockName, const char *fromPortName, uint32 fromPartNum,
                                          const char *toBlockName,   const char *toPortName,   uint32 toPartNum);
Err SetPatchConstant (PatchCmdBuilder *, const char *blockName, const char *portName, uint32 partNum, float32 value);
Err SetPatchCoherence (PatchCmdBuilder *builder, bool state);

    /* PatchCmd iterator */
PatchCmd *NextPatchCmd (const PatchCmd **patchCmdState);

    /* Debug */
void DumpPatchCmd (const PatchCmd *patchCmd, const char *banner);
void DumpPatchCmdList (const PatchCmd *patchCmdList, const char *banner);

#ifdef __cplusplus
}
#endif


/*****************************************************************************/


#endif /* __AUDIO_PATCH_H */
