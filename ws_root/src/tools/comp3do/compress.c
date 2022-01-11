/*  @(#) compress.c 96/04/23 1.17 */

#if !defined(macintosh)
#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/tags.h>
#include <misc/compression.h>
#elif !defined(__DCC__)
/* only dcc knows how to include headers with hierarchical path names */
#include "compress_noportfolio.h"
#else
#include <:kernel:types.h>
#include <:kernel:mem.h>
#include <:kernel:tags.h>
#include <:misc:compression.h>
#endif

#include <string.h>
#include <stdlib.h>
#include "lzss.h"

/*****************************************************************************/


/* This is the compression module which implements an LZ77-style (LZSS)
 * compression algorithm. As implemented here it uses a 12 bit index into the
 * sliding window, and a 4 bit length, which is adjusted to reflect phrase
 * lengths of between 3 and 18 bytes.
 *
 * WARNING: Check with the legal department before making any changes to
 *          the algorithms used herein in order to ensure that the code
 *          doesn't infrige on the multiple gratuitous patents on
 *          compression code.
 */


/*****************************************************************************/


#define LOOK_AHEAD_SIZE ((1 << LENGTH_BIT_COUNT) + BREAK_EVEN)
#define TREE_ROOT       WINDOW_SIZE
#define UNUSED          0


/* The ch_Tree array contains the binary tree of all of the strings in the
 * window sorted in order.
 */
typedef struct CompNode
{
    uint16 cn_Parent;
    uint16 cn_LeftChild;
    uint16 cn_RightChild;
} CompNode;


/*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
typedef void (* CompFuncClone)(void *userData, uint32 word);
#ifdef __cplusplus
}
#endif

typedef struct OutputStream
{
    CompFuncClone  os_OutputWord;
    void          *os_UserData;
    uint8          os_WriteBackBuffer[21];
    uint32         os_ControlFlags;
    uint32         os_NextFlag;
    uint32         os_BufferIndex;
    uint32         os_ControlByteIndex;
} OutputStream;


/*****************************************************************************/


struct Compressor
{
    unsigned char ch_Window[WINDOW_SIZE];
    CompNode      ch_Tree[WINDOW_SIZE+1];
    int32         ch_LookAhead;
    uint32        ch_MatchLen;
    uint32        ch_MatchPos;
    uint32        ch_CurrentPos;
    uint32        ch_ReplaceCnt;
    OutputStream     ch_OutputStream;
    bool          ch_SecondPass;
    bool          ch_AllocatedStructure;
    void         *ch_Cookie;
};

/*****************************************************************************/


static void InitOutputStream(OutputStream *os, CompFuncClone cf, void *userData)
{
    os->os_OutputWord       = cf;
    os->os_UserData         = userData;
    os->os_NextFlag         = 128;
    os->os_BufferIndex      = 1;
    os->os_ControlByteIndex = 0;
    os->os_ControlFlags     = 0;
}


/*****************************************************************************/


static void WriteByte(OutputStream *os, uint8 byte)
{
    os->os_WriteBackBuffer[os->os_BufferIndex++] = byte;
}


/*****************************************************************************/


static void WriteWord(OutputStream *os, uint16 word)
{
#ifndef NO_PORTFOLIO
uint16 *p;

    p  = (uint16 *)&os->os_WriteBackBuffer[os->os_BufferIndex];
    *p = word;
    os->os_BufferIndex += 2;
#else
    os->os_WriteBackBuffer[os->os_BufferIndex++] = word / 256;
    os->os_WriteBackBuffer[os->os_BufferIndex++] = word % 256;
#endif
}


/*****************************************************************************/


static void FlushOutputStream(OutputStream *os)
{
uint32 numWords, i;
uint32 numBytes;
uint32 start;

    os->os_WriteBackBuffer[os->os_ControlByteIndex] = os->os_ControlFlags;

    numWords = os->os_BufferIndex / 4;
    for (i = 0; i < numWords; i++)
        (* os->os_OutputWord)(os->os_UserData,*(uint32 *)&os->os_WriteBackBuffer[i*4]);

    numBytes = os->os_BufferIndex % 4;
    start    = os->os_BufferIndex - numBytes;
    for (i = 0; i < numBytes; i++)
        os->os_WriteBackBuffer[i] = os->os_WriteBackBuffer[start + i];

    os->os_ControlByteIndex = numBytes;
    os->os_BufferIndex      = numBytes + 1;
    os->os_ControlFlags     = 0;
    os->os_NextFlag         = 128;
}


/*****************************************************************************/


static void CleanupOutputStream(OutputStream *os)
{
    while (os->os_BufferIndex % 4)
        os->os_WriteBackBuffer[os->os_BufferIndex++] = 0;

    FlushOutputStream(os);
}


/*****************************************************************************/


static void WriteOne(OutputStream *os)
{
    if (os->os_NextFlag == 0)
        FlushOutputStream(os);

    os->os_ControlFlags |= os->os_NextFlag;
    os->os_NextFlag >>= 1;
}


/*****************************************************************************/


static void WriteZero(OutputStream *os)
{
    if (os->os_NextFlag == 0)
        FlushOutputStream(os);

    os->os_NextFlag >>= 1;
}


/*****************************************************************************/


/* This is where most of the encoder's work is done. This routine is
 * responsible for adding the new node to the binary tree. It also has to
 * find the best match among all the existing nodes in the tree, and return
 * that to the calling routine. To make matters even more complicated, if
 * the newNode has a duplicate in the tree, the oldNode is deleted, for
 * reasons of efficiency.
 */

static uint32 AddString(CompNode *tree, unsigned char *window,
                        uint32 newNode, uint32 *matchPos)
{
uint32    i;
uint32    testNode;
uint32    parentNode;
int32     delta;
uint32    matchLen;
CompNode *node;
CompNode *parent;
CompNode *test;

    if (newNode == END_OF_STREAM)
        return (0);

    testNode = tree[TREE_ROOT].cn_RightChild;
    node     = &tree[newNode];
    matchLen = 0;
    delta    = 0;

    while (TRUE)
    {
        for (i = 0; i < LOOK_AHEAD_SIZE; i++)
        {
            delta = window[MOD_WINDOW(newNode + i)] - window[MOD_WINDOW(testNode + i)];
            if (delta)
                break;
        }

        test = &tree[testNode];

        if (i >= matchLen)
        {
            matchLen  = i;
            *matchPos = testNode;
            if (matchLen >= LOOK_AHEAD_SIZE)
            {
                parentNode = test->cn_Parent;
                parent = &tree[parentNode];

                if (parent->cn_LeftChild == testNode)
                    parent->cn_LeftChild = newNode;
                else
                    parent->cn_RightChild = newNode;

                *node                               = *test;
                tree[node->cn_LeftChild].cn_Parent  = newNode;
                tree[node->cn_RightChild].cn_Parent = newNode;
                test->cn_Parent                     = UNUSED;

                return (matchLen);
            }
        }

        if (delta >= 0)
        {
            if (test->cn_RightChild == UNUSED)
            {
                test->cn_RightChild = newNode;
                node->cn_Parent     = testNode;
                node->cn_LeftChild  = UNUSED;
                node->cn_RightChild = UNUSED;
                return (matchLen);
            }
            testNode = test->cn_RightChild;
        }
        else
        {
            if (test->cn_LeftChild == UNUSED)
            {
                test->cn_LeftChild  = newNode;
                node->cn_Parent     = testNode;
                node->cn_LeftChild  = UNUSED;
                node->cn_RightChild = UNUSED;
                return (matchLen);
            }
            testNode = test->cn_LeftChild;
        }
    }
}


/*****************************************************************************/


/* This routine performs a classic binary tree deletion.
 * If the node to be deleted has a null link in either direction, we
 * just pull the non-null link up one to replace the existing link.
 * If both links exist, we instead delete the next link in order, which
 * is guaranteed to have a null link, then replace the node to be deleted
 * with the next link.
 */
static void DeleteString(CompNode *tree, uint16 node)
{
uint16 parent;
uint16 newNode;
uint16 next;

    parent = tree[node].cn_Parent;
    if (parent != UNUSED)
    {
        if (tree[node].cn_LeftChild == UNUSED)
        {
            newNode                 = tree[node].cn_RightChild;
            tree[newNode].cn_Parent = parent;
        }
        else if (tree[node].cn_RightChild == UNUSED)
        {
            newNode                 = tree[node].cn_LeftChild;
            tree[newNode].cn_Parent = parent;
        }
        else
        {
            newNode = tree[node].cn_LeftChild;
            next = tree[newNode].cn_RightChild;
            if (next != UNUSED)
            {
                do
                {
                    newNode = next;
                    next = tree[newNode].cn_RightChild;
                }
                while (next != UNUSED);

                tree[tree[newNode].cn_Parent].cn_RightChild = UNUSED;
                tree[newNode].cn_Parent                     = tree[node].cn_Parent;
                tree[newNode].cn_LeftChild                  = tree[node].cn_LeftChild;
                tree[newNode].cn_RightChild                 = tree[node].cn_RightChild;
                tree[tree[newNode].cn_LeftChild].cn_Parent  = newNode;
                tree[tree[newNode].cn_RightChild].cn_Parent = newNode;
            }
            else
            {
                tree[newNode].cn_Parent                     = parent;
                tree[newNode].cn_RightChild                 = tree[node].cn_RightChild;
                tree[tree[newNode].cn_RightChild].cn_Parent = newNode;
            }
        }

        if (tree[parent].cn_LeftChild == node)
            tree[parent].cn_LeftChild = newNode;
        else
            tree[parent].cn_RightChild = newNode;

        tree[node].cn_Parent = UNUSED;
    }
}


/*****************************************************************************/


Err CreateCompressor(Compressor **compr, CompFunc cf, const TagArg *tags)
{
bool    allocated;
void   *buffer;
#ifndef NO_PORTFOLIO
TagArg *tag;
#endif
void   *userData;

    if (!compr)
        return COMP_ERR_BADPTR;

    *compr = NULL;

    if (!cf)
        return COMP_ERR_BADPTR;

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
        buffer = AllocMem(sizeof(Compressor),MEMTYPE_ANY);
        if (!buffer)
            return COMP_ERR_NOMEM;

        allocated = TRUE;
    }

#else

    buffer = (void *)malloc(sizeof(Compressor));
    if (!buffer)
        return COMP_ERR_NOMEM;

    allocated = TRUE;
    userData  = (void *)tags;

#endif


    (*compr)                        = (Compressor *)buffer;
    (*compr)->ch_LookAhead          = 1;
    (*compr)->ch_CurrentPos         = 1;
    (*compr)->ch_MatchPos           = 0;
    (*compr)->ch_MatchLen           = 0;
    (*compr)->ch_ReplaceCnt         = 0;
    (*compr)->ch_SecondPass         = FALSE;
    (*compr)->ch_Cookie             = *compr;
    (*compr)->ch_AllocatedStructure = allocated;

    InitOutputStream(&(*compr)->ch_OutputStream,cf,userData);

    /* To make the tree usable, everything must be set to UNUSED, and a
     * single phrase has to be added to the tree so it has a root node.
     */
    memset((*compr)->ch_Tree, UNUSED, sizeof((*compr)->ch_Tree));
    (*compr)->ch_Tree[TREE_ROOT].cn_RightChild = 1;
    (*compr)->ch_Tree[1].cn_Parent             = TREE_ROOT;

    return (0);
}

/*****************************************************************************/


static void FlushCompressor(Compressor *compr)
{
int32          lookAhead;
uint32         currentPos;
uint32         replaceCnt;
uint32         matchLen;
uint32         matchPos;
CompNode      *tree;
unsigned char *window;
OutputStream  *os;

    tree         = compr->ch_Tree;
    window       = compr->ch_Window;
    os           = &compr->ch_OutputStream;
    lookAhead    = compr->ch_LookAhead;
    currentPos   = compr->ch_CurrentPos;
    matchLen     = compr->ch_MatchLen;
    matchPos     = compr->ch_MatchPos;
    replaceCnt   = compr->ch_ReplaceCnt;

    if (compr->ch_SecondPass)
        goto newData;

    while (lookAhead >= 0)
    {
        if (matchLen > lookAhead)
            matchLen = lookAhead;

        if (matchLen <= BREAK_EVEN)
        {
            WriteOne(os);
            WriteByte(os, window[currentPos]);
            replaceCnt = 1;
        }
        else
        {
            WriteZero(os);
            WriteWord(os, (matchPos << LENGTH_BIT_COUNT) | (matchLen - (BREAK_EVEN + 1)));
            replaceCnt = matchLen;
        }

        while (replaceCnt--)
        {
            DeleteString(tree, MOD_WINDOW(currentPos + LOOK_AHEAD_SIZE));
            lookAhead--;
newData:    currentPos = MOD_WINDOW(currentPos + 1);

            if (lookAhead)
                matchLen = AddString(tree, window, currentPos, &matchPos);
        }
    }
}


/*****************************************************************************/


Err DeleteCompressor(Compressor *compr)
{
    if (!compr || (compr->ch_Cookie != compr))
        return (COMP_ERR_BADPTR);

    compr->ch_Cookie = NULL;

    FlushCompressor(compr);
    WriteZero(&compr->ch_OutputStream);
    WriteWord(&compr->ch_OutputStream, END_OF_STREAM);
    CleanupOutputStream(&compr->ch_OutputStream);

#ifndef NO_PORTFOLIO

    if (compr->ch_AllocatedStructure)
        FreeMem(compr,sizeof(Compressor));

#else

    free(compr);

#endif

    return (0);
}


/*****************************************************************************/


/* This is the compression routine. It has to first load up the look
 * ahead buffer, then go into the main compression loop. The main loop
 * decides whether to output a single character or an index/length
 * token that defines a phrase. Once the character or phrase has been
 * sent out, another loop has to run. The second loop reads in new
 * characters, deletes the strings that are overwritten by the new
 * characters, then adds the strings that are created by the new
 * characters.
 */

Err FeedCompressor(Compressor *compr, const void *data, uint32 numDataWords)
{
uint32         currentPos;
uint32         replaceCnt;
uint32         matchLen;
uint32         matchPos;
uint8         *src;
CompNode      *tree;
unsigned char *window;
OutputStream  *os;
uint32         numBytes;

    if (!compr || (compr->ch_Cookie != compr))
        return (COMP_ERR_BADPTR);

    tree         = compr->ch_Tree;
    window       = compr->ch_Window;
    os           = &compr->ch_OutputStream;
    currentPos   = compr->ch_CurrentPos;
    matchLen     = compr->ch_MatchLen;
    matchPos     = compr->ch_MatchPos;
    replaceCnt   = compr->ch_ReplaceCnt;
    numBytes     = numDataWords * 4;
    src          = (uint8 *)data;

    if (!numBytes)
        return (0);

    if (compr->ch_SecondPass)
        goto newData;

    while (compr->ch_LookAhead <= LOOK_AHEAD_SIZE)
    {
        if (!numBytes)
            return (0);

        window[compr->ch_LookAhead++] = *src++;
        numBytes--;
    }

    while (TRUE)
    {
        if (matchLen > LOOK_AHEAD_SIZE)
            matchLen = LOOK_AHEAD_SIZE;

        if (matchLen <= BREAK_EVEN)
        {
            WriteOne(os);
            WriteByte(os, window[currentPos]);
            replaceCnt = 1;
        }
        else
        {
            WriteZero(os);
            WriteWord(os, (matchPos << LENGTH_BIT_COUNT) | (matchLen - (BREAK_EVEN + 1)));
            replaceCnt = matchLen;
        }

        while (replaceCnt--)
        {
            DeleteString(tree, MOD_WINDOW(currentPos + LOOK_AHEAD_SIZE));

            if (!numBytes)
            {
                /* We ran out of data. Save all the state, and exit. If
                 * we are called with more data, we'll jump right back in
                 * this loop, and continue processing
                 */

                compr->ch_LookAhead  = LOOK_AHEAD_SIZE;
                compr->ch_CurrentPos = currentPos;
                compr->ch_MatchLen   = matchLen;
                compr->ch_MatchPos   = matchPos;
                compr->ch_ReplaceCnt = replaceCnt;
                compr->ch_SecondPass = TRUE;
                return (0);
            }
newData:
            numBytes--;
            window[MOD_WINDOW(currentPos + LOOK_AHEAD_SIZE)] = *src++;

            currentPos = MOD_WINDOW(currentPos + 1);
            matchLen = AddString(tree, window, currentPos, &matchPos);
        }
    }
}


/*****************************************************************************/


int32 GetCompressorWorkBufferSize(const TagArg *tags)
{
    if (tags)
        return COMP_ERR_BADTAG;

    return sizeof(Compressor);
}
