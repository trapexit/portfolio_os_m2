/* @(#) decompress.c 96/05/01 1.9 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/tags.h>
#include <misc/compression.h>
#include <stdlib.h>
#include "lzss.h"


/*****************************************************************************/


/* WARNING: Check with the legal department before making any changes to
 *          the algorithms used herein in order to ensure that the code
 *          doesn't infringe on the multiple gratuitous patents on
 *          compression code.
 */


/*****************************************************************************/


typedef void (* CompFuncClone)(void *userData, uint32 word);

typedef enum DecompStates
{
    STATE_CONTROLBYTE,
    STATE_LITTERALBYTE,
    STATE_FIRSTMATCHBYTE,
    STATE_SECONDMATCHBYTE
} DecompStates;

struct Decompressor
{
    CompFuncClone  dh_OutputWord;
    void          *dh_UserData;
    uint8          dh_Window[WINDOW_SIZE];
    uint32         dh_WordBuffer;        /* buffer of decompressed bytes   */
    uint8          dh_BytesLeft;         /* bytes available in word buffer */
    uint32         dh_Pos;
    DecompStates   dh_State;
    uint32         dh_MatchWord;
    Err            dh_Error;
    uint8          dh_ControlFlags;
    uint8          dh_NextFlag;
    bool           dh_EOS;
    bool           dh_AllocatedStructure;
    void          *dh_Cookie;
};


/*****************************************************************************/


#define OutputByte(b) if (bytesLeft == 0)\
                      {\
                          (* cf)(userData, wordBuffer);\
                          wordBuffer = b;\
                          bytesLeft  = 3;\
                      }\
                      else\
                      {\
                          wordBuffer = (wordBuffer << 8) | b;\
                          bytesLeft--;\
                      }

#define GetByte()     {if (numBytes == 0) goto exit; byte = *buffer++; numBytes--;}


/*****************************************************************************/


/* This routine implements a state machine to decode the input data. The states
 * are:
 *
 *    STATE_CONTROLBYTE
 *    The parser is in need of a control byte which contains eight flag bits
 *    describing the next 8 operations to be performed. If a bit is set within
 *    the control byte, it means that a litteral byte is to be fetched from
 *    the input, and a clear bit means that an index/length pair is to be
 *    fetched.
 *
 *    STATE_LITTERALBYTE
 *    The parser is in need of a litteral byte. Once the byte is gotten, we
 *    look at the current control byte to determine what the next state
 *    should be. If the control byte has been exhausted, the STATE_CONTROLBYTE
 *    state is entered. Otherwise we either remain in STATE_LITTERALBYTE
 *    or switch to STATE_FIRSTMATCHBYTE.
 *
 *    STATE_FIRSTMATCHBYTE
 *    The parser is in need of the first byte of an index/length pair. Once the
 *    byte is gotten, the STATE_SECONDMATCHBYTE is entered.
 *
 *    STATE_SECONDMATCHBYTE
 *    The parser is in need of the second byte of an index/length pair. Once
 *    the byte is gotten and processed, we look at the current control byte to
 *    determine what the next operation should be. If the control byte has been
 *    exhausted, the STATE_CONTROLBYTE state is entered.
 *
 * The state machine uses fall through cases in order to avoid reentering the
 * switch statement needlessly. The states are still clearly maintained though,
 * since we can exit/enter the machine at any time depending on when we run
 * out of input.
 */

Err FeedDecompressor(Decompressor *decomp, const void *data, uint32 numDataWords)
{
uint32         i;
uint32         pos;
uint32         matchLen;
uint32         matchPos;
uint32         wordBuffer;
uint8          bytesLeft;
CompFunc       cf;
uint8         *window;
void          *userData;
uint32         matchWord;
DecompStates   state;
uint32         numBytes;
uint8         *buffer;
uint8          controlFlags;
uint8          nextFlag;
uint8          byte;

    if (!decomp || (decomp->dh_Cookie != decomp))
        return (COMP_ERR_BADPTR);

    if (decomp->dh_Error)
        return decomp->dh_Error;

    if (decomp->dh_EOS)
        return COMP_ERR_DATAREMAINS;

    /* setup local vars */
    cf           = decomp->dh_OutputWord;
    userData     = decomp->dh_UserData;
    window       = decomp->dh_Window;
    wordBuffer   = decomp->dh_WordBuffer;
    bytesLeft    = decomp->dh_BytesLeft;
    pos          = decomp->dh_Pos;
    state        = decomp->dh_State;
    matchWord    = decomp->dh_MatchWord;
    controlFlags = decomp->dh_ControlFlags;
    nextFlag     = decomp->dh_NextFlag;

    numBytes     = numDataWords * 4;
    buffer       = (uint8 *)data;

    while (TRUE)
    {
        GetByte();

        switch (state)
        {
            case STATE_CONTROLBYTE     : nextFlag     = 64;
                                         controlFlags = byte;

                                         if ((controlFlags & 128) == 0)
                                         {
                                             state = STATE_FIRSTMATCHBYTE;
                                             break;
                                         }

                                         state = STATE_LITTERALBYTE;
                                         GetByte();

            case STATE_LITTERALBYTE    : OutputByte(byte);
                                         window[pos] = byte;
                                         pos = MOD_WINDOW(pos + 1);

                                         if (nextFlag == 0)
                                         {
                                             state = STATE_CONTROLBYTE;
                                             break;
                                         }

                                         if (controlFlags & nextFlag)
                                         {
                                             state      = STATE_LITTERALBYTE;
                                             nextFlag >>= 1;
                                             break;
                                         }

                                         state      = STATE_FIRSTMATCHBYTE;
                                         nextFlag >>= 1;
                                         GetByte();

            case STATE_FIRSTMATCHBYTE  : matchWord = byte << 8;
                                         state     = STATE_SECONDMATCHBYTE;
                                         GetByte();

            case STATE_SECONDMATCHBYTE : matchWord |= byte;
                                         matchPos = matchWord >> LENGTH_BIT_COUNT;
                                         if (matchPos == END_OF_STREAM)
                                         {
                                             decomp->dh_EOS = TRUE;
                                             if (numBytes > 3)
                                                 decomp->dh_Error = COMP_ERR_DATAREMAINS;

                                             decomp->dh_BytesLeft = bytesLeft;

                                             return decomp->dh_Error;
                                         }

                                         matchLen = (matchWord & ((1 << LENGTH_BIT_COUNT) - 1)) + BREAK_EVEN;

                                         for (i = matchPos; i <= matchPos + matchLen; i++)
                                         {
                                             byte = window[MOD_WINDOW(i)];
                                             OutputByte(byte);
                                             window[pos] = byte;
                                             pos = MOD_WINDOW(pos + 1);
                                         }

                                         if (nextFlag == 0)
                                         {
                                             state = STATE_CONTROLBYTE;
                                             break;
                                         }

                                         if (controlFlags & nextFlag)
                                             state = STATE_LITTERALBYTE;
                                         else
                                             state = STATE_FIRSTMATCHBYTE;

                                         nextFlag >>= 1;
                                         break;

        }
    }

exit:

    /* preserve the state for later */
    decomp->dh_WordBuffer   = wordBuffer;
    decomp->dh_BytesLeft    = bytesLeft;
    decomp->dh_Pos          = pos;
    decomp->dh_State        = state;
    decomp->dh_MatchWord    = matchWord;
    decomp->dh_ControlFlags = controlFlags;
    decomp->dh_NextFlag     = nextFlag;

    return (0);
}


/*****************************************************************************/


Err CreateDecompressor(Decompressor **decomp, CompFunc cf, const TagArg *tags)
{
bool    allocated;
void   *buffer;
#ifndef NO_PORTFOLIO
TagArg *tag;
#endif
void   *userData;

    if (!decomp)
        return COMP_ERR_BADPTR;

    *decomp = NULL;

    if (!cf)
        return (COMP_ERR_BADPTR);

#ifndef NO_PORTFOLIO

    buffer   = NULL;
    userData = NULL;

    while ((tag = NextTagArg(&tags)) != NULL)
    {
        switch (tag->ta_Tag)
        {
            case COMP_TAG_WORKBUFFER: buffer = tag->ta_Arg;
                                      break;

            case COMP_TAG_USERDATA  : userData = tag->ta_Arg;
                                      break;

            default                 : return COMP_ERR_BADTAG;
        }
    }

    allocated = FALSE;
    if (!buffer)
    {
        buffer = AllocMem(sizeof(Decompressor),MEMTYPE_FILL|MEMTYPE_ANY);
        if (!buffer)
            return COMP_ERR_NOMEM;

        allocated = TRUE;
    }

#else

    buffer = (void *)malloc(sizeof(Decompressor));
    if (!buffer)
        return COMP_ERR_NOMEM;

    allocated = TRUE;
    userData  = (void *)tags;

#endif

    (*decomp)                        = (Decompressor *)buffer;
    (*decomp)->dh_OutputWord         = cf;
    (*decomp)->dh_UserData           = userData;
    (*decomp)->dh_WordBuffer         = 0;
    (*decomp)->dh_BytesLeft          = 4;
    (*decomp)->dh_Pos                = 1;
    (*decomp)->dh_Error              = 0;
    (*decomp)->dh_State              = STATE_CONTROLBYTE;
    (*decomp)->dh_EOS                = FALSE;
    (*decomp)->dh_Cookie             = *decomp;
    (*decomp)->dh_AllocatedStructure = allocated;

    return (0);
}


/*****************************************************************************/


Err DeleteDecompressor(Decompressor *decomp)
{
Err result;

    if (!decomp || (decomp->dh_Cookie != decomp))
        return (COMP_ERR_BADPTR);

    decomp->dh_Cookie = NULL;

    if (decomp->dh_BytesLeft == 0)
        (* decomp->dh_OutputWord)(decomp->dh_UserData, decomp->dh_WordBuffer);

    result = decomp->dh_Error;
    if ((result >= 0) && !decomp->dh_EOS)
        result = COMP_ERR_DATAMISSING;

#ifndef NO_PORTFOLIO

    if (decomp->dh_AllocatedStructure)
        FreeMem(decomp,sizeof(Decompressor));

#else

    free(decomp);

#endif

    return (result);
}


/*****************************************************************************/


int32 GetDecompressorWorkBufferSize(const TagArg *tags)
{
    if (tags)
        return COMP_ERR_BADTAG;

    return sizeof(Decompressor);
}
