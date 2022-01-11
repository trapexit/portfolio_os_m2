/*
	File:		unix.c

	Contains:	 

	Written by:	 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<3+>	 9/18/95	TMA		Fixed Mac I/O bugs.
		<1+>	 7/11/95	TMA		Fix include files for brain dead Macs.

	To Do:
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __MWERKS__
#ifndef applec
#define applec
#endif
#endif

#ifdef applec
#ifndef __SCRIPT__
#include <Script.h>
#endif
#endif

#ifdef applec

#ifdef __MWERKS__
#pragma only_std_keywords off
#endif

#ifndef __FILES__
#include <Files.h>
#endif

#include <Errors.h>
#include <CursorCtl.h>

#ifdef __MWERKS__
#pragma only_std_keywords reset
#pragma ANSI_strict reset
#endif

#endif

#include "ifflib.h"

#ifndef applec
void FreeMem(void *p, int32 size)
{
  free(p);
}

#endif


/* #include <kernel/kernelnodes.h> */

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

/*****************************************************************************/

bool PrintFault( int32 code, char *header );
int32 IoErr( void );

/*****************************************************************************/

static int strcasecmp(const char *s1, const char *s2)
{
    return strcmp(s1,s2);
}

void PrepList(List *l)
{
    l->l_Flags  = 0;
    l->l_Head   = (Link *)&l->l_Filler;
    l->l_Filler = NULL;
    l->l_Tail   = (Link *)&l->l_Head;
}

void AddHead( List *l, Node *new )
{
    ADDHEAD(l,new);
}

void AddTail( List *l, Node *new )
{
    ADDTAIL(l,new);
}

Node *RemHead(List *l)
{
Node *base;
Node *head;

    base = (Node *)&l->ListAnchor.head.links;
    head = base->n_Next;
    if (head->n_Next)
    {
        base->n_Next         = head->n_Next;
        head->n_Next->n_Prev = base;
        return head;
    }

    return NULL;
}

Node *RemTail(List *l)
{
Node *base;
Node *tail;

    base = (Node *)&l->ListAnchor.tail.links;
    tail = base->n_Prev;
    if (tail->n_Prev)
    {
        base->n_Prev         = tail->n_Prev;
        tail->n_Prev->n_Next = base;
        return tail;
    }

    return NULL;
}

void RemNode( Node *n )
{
    REMOVENODE(n);
}

void InsertNodeFromHead( List *l, Node *new )
{
Node *n;

    ScanList(l,n,Node)
    {
        if (new->n_Priority >= n->n_Priority)
        {
            new->n_Next         = n;
            new->n_Prev         = n->n_Prev;
            new->n_Prev->n_Next = new;
            n->n_Prev           = new;
            return;
        }
    }
    ADDTAIL(l,new);
}

void InsertNodeFromTail(List *l, Node *new)
{
Node *n;

    ScanListB(l,n,Node)
    {
        if (n->n_Priority >= new->n_Priority)
        {
            new->n_Prev         = n;
            new->n_Next         = n->n_Next;
            new->n_Next->n_Prev = new;
            n->n_Next           = new;
            return;
        }
    }
    ADDHEAD(l,new);
}

void UniversalInsertNode (List *l, Node *new, bool (*f) (Node *n, Node *m))
{
Node *n;

    ScanList(l,n,Node)
    {
        if ((*f)(new, n))
        {
            new->n_Next         = n;
            new->n_Prev         = n->n_Prev;
            new->n_Prev->n_Next = new;
            n->n_Prev           = new;
            return;
        }
    }
    ADDTAIL(l, new);
}

Node *FindNamedNode (const List *l, const char *s)
{
Node *n;
char *name;

    ScanList(l,n,Node)
    {
        name = n->n_Name;
        if (s && name)
        {
            if (strcasecmp(s, name) == 0)
                return n;
        }
        else if (s == name)      /* are both NULL? */
        {
            return n;
        }
    }

    return NULL;
}

uint8 SetNodePri (Node *n, uint8 pri)
{
  List  *l;
  Node  *lastnode;
  uint8  oldpri = n->n_Priority;
  

    /* Find the list where this node is */
    lastnode = NextNode(n);
    while (lastnode->n_Next)
        lastnode = NextNode(lastnode);

    /* lastnode now points to the tail.links */
    l = (List *)((uint32)lastnode - offsetof(List,ListAnchor.tail.links));

    REMOVENODE(n);
    n->n_Priority = pri;
    InsertNodeFromTail(l, n);

    return oldpri;
}

TagArg *NextTagArg(const TagArg **tagList);

TagArg *NextTagArg(const TagArg **tagList)
{
const TagArg *t = *tagList;

    while (t)
    {
        switch (t->ta_Tag)
        {
            case TAG_END : t = NULL;
                           break;

            case TAG_NOP : t++;
                           break;

            case TAG_JUMP: t = (TagArg *)t->ta_Arg;
                           break;

            default      : *tagList = &t[1];
                           return (TagArg *)t;
        }
    }

    return NULL;
}



/* define location, size flags */
#define MEMTYPE_ANY		(uint32)0
#define MEMTYPE_FILL		(uint32)0x00000100 /* fill memory with value */

void *AllocMem(int32 size, uint32 typebits);

void *AllocMem(int32 size, uint32 typebits)
{
  void *ptr;
  if (typebits & MEMTYPE_FILL)
    {
      ptr = calloc(size,1);
      return((void *)ptr);
    }
  else
    {
     ptr = malloc(size);
      return((void *)ptr);
    }
}


#ifdef applec

static Boolean IFF_KeyBit(KeyMap keyMap, char keyCode)
{
	char* keyMapPtr = (char*)keyMap;
	char keyMapByte = keyMapPtr[keyCode / 8];
	return ((keyMapByte & (1 << (keyCode % 8))) != 0);
}

/* Spin the cursor, checking for command period. */
void IFF_SpinCursor(void)
{
	const char	kAbortKeyCode = 47;	/* US "Period" key code.	*/
	const char	kCmdKeyCode = 55;	
	KeyMap	theKeyMap;	
	GetKeys(theKeyMap);
	
	if ( IFF_KeyBit(theKeyMap, kCmdKeyCode) && IFF_KeyBit(theKeyMap, kAbortKeyCode))
	{
		exit(-9); /* conventional status value for user abort */
	}
	SpinCursor(5);
}

static Err FileOpenMac(IFFParser *iff, void *key, bool writeMode)
{
  OSErr macErr;
  short tempShort;

  if (writeMode)
    macErr = FSpOpenDF((FSSpec *)key, fsWrPerm, &tempShort);
  else
    {
      macErr = FSpOpenDF((FSSpec *)key, fsRdPerm, &tempShort);
    }
  iff->iff_IOContext = (void *)tempShort;
  if (macErr != noErr)
    return(-1);
  else
    return (0);
}

static Err FileCloseMac(IFFParser *iff)
{
  OSErr macErr;
  short tempShort;
  
  tempShort = (short)iff->iff_IOContext;
  macErr = FSClose(tempShort);
  
  if (macErr != noErr)
    return (-1);
  else
    return (0);
}

static int32 FileReadMac(IFFParser *iff, void *buffer, uint32 numBytes)
{
  long count;
  short tempShort;
  
  tempShort = (short)iff->iff_IOContext;
  count = numBytes;
  IFF_SpinCursor();
  FSRead(tempShort, &count, buffer);
  return(count);
}

static int32 FileWriteMac(IFFParser *iff, const void *buffer, uint32 numBytes)
{
  long count;
  short tempShort;
  
  count = numBytes;  
  tempShort = (short)iff->iff_IOContext;
  IFF_SpinCursor();
  FSWrite(tempShort, &count, buffer);
  return(count);
}

static int32 FileSeekMac(IFFParser *iff, int32 position)
{
  short tempShort;

  tempShort = (short)iff->iff_IOContext;
  return SetFPos(tempShort,fsFromMark, position);
}


#endif

static Err FileOpen(IFFParser *iff, void *key, bool writeMode)
{
  const char *mode;
  const char *modeRead = "rb";
  const char *modeWrite = "wb";


  if (writeMode)
    mode = modeWrite;
  else
    mode = modeRead;
  
  iff->iff_IOContext = fopen((char *)key,mode);
  if (iff->iff_IOContext == NULL)
    return(-1);
  else
    return (0);
}

static Err FileClose(IFFParser *iff)
{
  Err result;
  
  result = fclose((FILE *)(iff->iff_IOContext));
  iff->iff_IOContext = NULL;
  
  if (result != 0)
    return (-1);
  else
    return (0);
}

static int32 FileRead(IFFParser *iff, void *buffer, uint32 numBytes)
{
#ifdef applec
IFF_SpinCursor();
#endif
    return fread(buffer, 1, numBytes, iff->iff_IOContext);
}

static int32 FileWrite(IFFParser *iff, const void *buffer, uint32 numBytes)
{
#ifdef applec
IFF_SpinCursor();
#endif
    return fwrite(buffer, 1, numBytes, iff->iff_IOContext);
}

static int32 FileSeek(IFFParser *iff, int32 position)
{
    return fseek(iff->iff_IOContext, position, SEEK_CUR);
}

static Err MemOpen(IFFParser *iff, void *key, bool writeMode)
{
    iff->iff_IOContext = key;
    return 0;
}

static Err MemClose(IFFParser *iff)
{
    return 0;
}

static int32 MemRead(IFFParser *iff, void *buffer, uint32 numBytes)
{
    memcpy(buffer,iff->iff_IOContext,numBytes);
    iff->iff_IOContext = (void *)(((uint32)iff->iff_IOContext) + numBytes);
    return 0;
}

static int32 MemWrite(IFFParser *iff, const void *buffer, uint32 numBytes)
{
    memcpy(iff->iff_IOContext,buffer,numBytes);
    iff->iff_IOContext = (void *)(((uint32)iff->iff_IOContext) + numBytes);
    return 0;
}

static int32 MemSeek(IFFParser *iff, int32 position)
{
    iff->iff_IOContext = (void *)((int32)iff->iff_IOContext + position);
    return 0;
}

IFFIOFuncs fileFuncs = {FileOpen, FileClose, FileRead, FileWrite, FileSeek};

#ifdef applec
IFFIOFuncs fileFuncsMac = {FileOpenMac, FileCloseMac, FileReadMac, FileWriteMac, FileSeekMac};
#endif

IFFIOFuncs memFuncs = {MemOpen,
 MemClose,
 MemRead,
 MemWrite,
 MemSeek};


