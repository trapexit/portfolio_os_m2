/*
	File:		qmem.h

	Contains:	This file contains definition qmem 

	Written by:	Anthont Tai, 3DO 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	 7/11/95	TMA		Removing <stdio.h> include.
		<1+>	 1/16/95	TMA		Update headers

	To Do:
*/


#ifndef _H_qmem
#define _H_qmem


void *qMemNewPtr
	(
	long size
	);

void *qMemClearPtr
	(
	long size,
	long elemSize
	);

void qMemReleasePtr
	(
	void *ptr
	);

void *qMemResizePtr
	(
	void *ptr,
	long size
	);

BYTE qGetByte
	(
	FILE *theFile
	);

WORD qGetWord
	(
	FILE *theFile
	);

WORD qGetWord
	(
	FILE *theFile
	);

DWORD qGetDWord
	(
	FILE *theFile
	);

void qGetString
	(
	BYTE *thePtr,
	long theSize, 
	FILE *theFile
	);

void BeginWrite(void);
long GetCurrPos(void);

int qPutByte
	(
	BYTE theByte,
	FILE *theFile
	);

short qPutWord
	(
	WORD theWord,
	FILE *theFile
	);

short qPutDWord
	(
	DWORD theDWord,
	FILE *theFile
	);

void qPutString
	(
	BYTE *thePtr,
	long theSize, 
	FILE *theFile
	);

void qPutBits
	(
	DWORD theValue,
	BYTE bitnum,
	FILE *theFile
	);

void BeginPutBits(void);

void EndPutBits(FILE *theFile);

void qMemPutBits
	(
	DWORD theValue,
	BYTE bitnum
	);

void BeginMemPutBits
	(
	BYTE *thePtr,
	long bytecount
	);

void EndMemPutBits(void);

DWORD qMemGetBits
	(
	BYTE bitnum
	);

void BeginMemGetBits
	(
	BYTE *theBuff
	);

void EndMemGetBits(void);

void qMemPutString
	(
	BYTE *ptr,
	long bytecount
	);

void qMemPutBytes
	(
	BYTE *ptr,
	long bytecount
	);

void BeginMemPutBytes
	(
	BYTE *thePtr,
	long bytecount
	);
	
void EndMemPutBytes(void);

long GetCurrMemPutPos(void);

void qMemGetBytes
	(
	BYTE *ptr, 
	long bytecount
	);

void BeginMemGetBytes
	(
	BYTE *theBuff
	);
	
void EndMemGetBytes(void);

long GetCurrMemPutBitPos(void);

void qMemSkipBits
	(
	long bitnum
	);
  
#define MemQB_Vars \
register uint32 *QBWordPtr; \
register uint32 QBBitsLeft; \
register uint32 QBCurWord; \
register uint32 QBResult;  \
register uint32 QBBitNum

#ifdef INTEL

#define BeginMemQGetBits(B)  QBWordPtr = (B); QBBitsLeft=32; \
QBCurWord = *QBWordPtr; \
QBBitNum = (QBCurWord<<24) | ((QBCurWord & 0x0000FF00)<<8) | ((QBCurWord & 0x00FF0000)>>8) |  (QBCurWord>>24); \
QBCurWord = QBBitNum

#else

#define BeginMemQGetBits(B)  QBWordPtr = (B); QBBitsLeft=32; \
QBCurWord = *QBWordPtr

#endif

#ifdef INTEL

#define MemQGetBits(A) \
QBBitNum= (A); \
if (QBBitsLeft>QBBitNum) \
{ \
  QBResult = QBCurWord >> (32-QBBitNum); QBCurWord = QBCurWord << QBBitNum; \
  QBBitsLeft -= QBBitNum; \
} \
else if (QBBitsLeft<QBBitNum) \
{ \
  QBResult = QBCurWord >> (32-QBBitsLeft); QBBitsLeft = QBBitNum-QBBitsLeft; \
  QBResult = QBResult << QBBitsLeft; QBWordPtr++; QBCurWord = *QBWordPtr; \
  QBBitNum = (QBCurWord<<24) | ((QBCurWord & 0x0000FF00)<<8) | ((QBCurWord & 0x00FF0000)>>8) |  (QBCurWord>>24); \
  QBCurWord = QBBitNum; \
  QBResult += QBCurWord >> (32 - QBBitsLeft); \
  QBCurWord = QBCurWord << QBBitsLeft; QBBitsLeft = 32 - QBBitsLeft; \
} \
else \
{ \
   QBResult = QBCurWord >> (32-QBBitNum); QBBitsLeft = 0;\
}

#else

#define MemQGetBits(A) \
QBBitNum= (A); \
if (QBBitsLeft>QBBitNum) \
{ \
  QBResult = QBCurWord >> (32-QBBitNum); QBCurWord = QBCurWord << QBBitNum; \
  QBBitsLeft -= QBBitNum; \
} \
else if (QBBitsLeft<QBBitNum) \
{ \
  QBResult = QBCurWord >> (32-QBBitsLeft); QBBitsLeft = QBBitNum-QBBitsLeft; \
  QBResult = QBResult << QBBitsLeft; QBWordPtr++; QBCurWord = *QBWordPtr; \
  QBResult += QBCurWord >> (32 - QBBitsLeft);  QBCurWord = QBCurWord << QBBitsLeft;\
  QBBitsLeft = 32 - QBBitsLeft; \
} \
else \
{ \
   QBResult = QBCurWord >> (32-QBBitNum); QBBitsLeft = 0;\
}

#endif

#define EndMemQGetBits()  QBWordPtr=NULL;


#define BeginMemQPutBits(B)  QBWordPtr = (B); QBBitsLeft=32; \
QBCurWord = 0


#ifdef INTEL
 
#define MemQPutBits(B,A) \
QBResult =(B); QBBitNum =(A); \
if (QBBitsLeft > QBBitNum) \
{ \
  QBCurWord += QBResult << (QBBitsLeft-QBBitNum); QBBitsLeft -= QBBitNum; \
} \
else if (QBBitsLeft<QBBitNum) \
{ \
  QBBitsLeft = QBBitNum-QBBitsLeft;  QBCurWord += QBResult >> (QBBitsLeft); \
  QBBitNum = (QBCurWord<<24) | ((QBCurWord & 0x0000FF00)<<8) | ((QBCurWord & 0x00FF0000)>>8) |  (QBCurWord>>24); \
  QBCurWord = QBBitNum; \
  *QBWordPtr = QBCurWord; QBWordPtr++; QBCurWord = (QBResult) << (32 - QBBitsLeft);  \
  QBBitsLeft = 32 - QBBitsLeft; \
} \
else \
{ \
  QBCurWord += QBResult; \
  QBBitNum = (QBCurWord<<24) | ((QBCurWord & 0x0000FF00)<<8) | ((QBCurWord & 0x00FF0000)>>8) |  (QBCurWord>>24); \
  QBCurWord = QBBitNum; \
  *QBWordPtr = QBCurWord; QBCurWord=0; QBBitsLeft = 32; QBWordPtr++; \
}

#else
 
#define MemQPutBits(B,A) \
QBResult =(B); QBBitNum =(A); \
if (QBBitsLeft > QBBitNum) \
{ \
  QBCurWord += QBResult << (QBBitsLeft-QBBitNum); QBBitsLeft -= QBBitNum; \
} \
else if (QBBitsLeft<QBBitNum) \
{ \
  QBBitsLeft = QBBitNum-QBBitsLeft; QBCurWord += QBResult >> (QBBitsLeft); \
  *QBWordPtr = QBCurWord; QBWordPtr++;  QBCurWord = (QBResult) << (32 - QBBitsLeft);  \
  QBBitsLeft = 32 - QBBitsLeft; \
} \
else \
{ \
  QBCurWord += QBResult;  \
  *QBWordPtr = QBCurWord; QBCurWord=0; QBBitsLeft = 32; QBWordPtr++; \
}

#endif


#ifdef INTEL

#define EndMemQPutBits()  \
if (QBBitsLeft<32) \
{ \
    QBBitNum = (QBCurWord<<24) | ((QBCurWord & 0x0000FF00)<<8) | ((QBCurWord & 0x00FF0000)>>8) |  (QBCurWord>>24); \
    QBCurWord = QBBitNum; \
   *QBWordPtr = QBCurWord;\
}\
QBBitsLeft=0; QBWordPtr=NULL

#else

#define EndMemQPutBits()  \
if (QBBitsLeft<32) \
   *QBWordPtr = QBCurWord;\
QBBitsLeft=0; QBWordPtr=NULL

#endif


#define oneExtend(A,B) ((0x01) & (B)) ? (((B)<<(8-(A))) + ((0xFF>>(A)))) : ((B)<<(8-(A)))

#define QuickDecode(color, ssb, alpha, red, green, blue) \
blue = color & 0xFF; green = (color >> 8) &0xFF; \
red = (color >> 16) &0xFF; alpha = ((color >> 24) &0x7F) << 1;	\
if (color & 0x01000000) alpha++; ssb = (color >> 31)

#define QuickCreate(ssb, alpha, red, green, blue) \
(((uint32)(ssb))<<31) + (((uint32)((alpha) & 0xFE))<<23) + (((uint32)(red)) << 16) +  (((uint32)(green)) << 8) + (blue)



#endif
