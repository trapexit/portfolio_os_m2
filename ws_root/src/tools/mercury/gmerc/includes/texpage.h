/*
 *	@(#) texpage.h 3/21/96
 *	Copyright 1996, The 3DO Company
 */

		
/****
 *
 * This is a interface between the reader/parser module and the geometry library
 *
 * The first step is to saperate the 3do parser (sdf parser with the rest of the
 * gcomp code. As an immediate step, a temporary mechanism will be used to gap
 * the framework parser and geometry processing code. We will take the data out of
 * the framework dictionary and move it into geometry library internal data structure.
 * The next step will be swapping all of the framework reader/parser out of the 
 * project and repalce it with the new ASCII SDF parser.
 *
 ****/
 

#ifndef _TEXPAGE_
#define _TEXPAGE_

#define GEOM_FLAG   0x0001
#define ANIM_FLAG   0x0002


uint32 GetFacetSurfaceFlag(void);
void SetFacetSurfaceFlag(uint32 flag);
uint32 GetFrameworkFlag(void);
void SetFrameworkFlag(uint32 flag);
uint32 GetMessageFlag(void);
void SetMessageFlag(uint32 flag);
uint32 GetCollapseFlag(void);
void SetCollapseFlag(uint32 flag);
uint32 GetAnimFlag(void);
void SetAnimFlag(uint32 flag);
void ResetMercid(void);


int HasTexPage(void);
void SetTexPage(int flag);
int TexPage_Parse(char *tpfile);
void NewTexTable(void);
void DeleteTexTable(void);
void SetTexName(int texindex,char *name);
char *GetTexName(int texindex);
int SetTexEntry(char *name,int texpage,int subpage);
int GetUsedTexCount(void);
int GetTexEntry(int texindex,char *name,int *texpage,int *subpage);

/*
** Header file stuff
*/
void OpenHeaderFile(char *filename);
void CloseHeaderFile(void);
void AddDefine(char *name, int num);
int HasHeaderFile(void);

#endif
