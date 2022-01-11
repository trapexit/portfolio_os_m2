/*
 *	@(#) geointerface.c 3/21/96
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
 

#include "bsdf_iff.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "texpage.h"

int TexPage_Parse(char *tpfile);
static int gTexpage = 0;
static int has_texpage = 0;

static void makelower(char *src)
{
  int i;
  int n = strlen(src);
	
  for (i = 0; i < n; i++)
    *(src+i) = tolower(*(src+i));
}

int HasTexPage(void)
{
  return(has_texpage);
}

void SetTexPage(int flag)
{
  has_texpage = flag;
}

#ifdef __Sun__
#define SEEK_END 2
#endif

int TexPage_Parse(char *tpfile)
{
  FILE *fp;
  int fsize, itok = 0, i;
  char *tmp, *utf[1000], *str;
  int texpage = 0, subpage = 0;
	
  fp = fopen(tpfile, "r");
  if (fp == NULL)
    return 0;
  fseek(fp, 0, SEEK_END);
  fsize = ftell(fp);
  rewind(fp);
  str = (char *)malloc(fsize+1);
  fread(str, 1, (unsigned int)fsize, fp);
  *(str+fsize) = 0;
	
  for (i = 0; i < fsize; i++)
    {
      if (*(str+i) == '#')
	while ((*(str+i) != '\r') && (*(str+i) != '\n'))
	  {
	    *(str+i) = ' ';
	    i++;
	    if (i >= fsize)
	      break;
	  }
    }

  utf[itok] = strtok(str, " \t\n\"\r");
  while (utf[itok] != NULL)
    {
      makelower(utf[itok]);
      if (!strcmp(utf[itok], "texpagearray"))
	break;
      utf[itok] = strtok(NULL, " \t\n\"\r");
    }
  if (utf[itok] == NULL)
    goto Error;

  itok++;
  while (utf[itok-1] != NULL)
    {
      utf[itok] = strtok(NULL, " \t\n\"\r");
      itok++;
    }
  /*
    itok--;
    while (i < itok)
    {
    makelower(utf[i]);
    if (strcmp(utf[i], "texpagearray"))
    i++;
    else
    break;
    }
    */
  {
    i = 0;
    makelower(utf[i]);
    if (!strcmp(utf[i++], "texpagearray"))
      {
	if (!strcmp(utf[i++], "{"))
	  {
	    while (!strcmp(utf[i], "{"))
	      {
		i++;
		subpage = 0;
		while (strcmp(utf[i], "}"))
		  {
		    makelower(utf[i]);
		    if (strcmp(utf[i++], "filename"))
		      goto Error;
		    makelower(utf[i]);
		    SetTexEntry(utf[i], texpage, subpage);
		    subpage++;
		    i++;
		  }
		texpage++;
		if (strcmp(utf[i++], "}"))
		  goto Error;
	      }
	  }
	else
	  goto Error;
	if (strcmp(utf[i++], "}"))
	  goto Error;
      }
    else
      goto Error;
  }
  gTexpage = texpage;
  free(str);
  fclose(fp);
  return 1;
Error:
  printf("syntax error\n");
  return 0;
}

typedef struct TPIndex
{
  int32   texpage;
  int32   subpage;
  int32	used;
} TPIndex;

static char *textable = NULL;
static TPIndex *texindextable = NULL;
static int lastcount = 0;
#define TEXCHARNUM 		32
#define TEXTABLESIZE	1000
static int tp[TEXTABLESIZE];

void NewTexTable(void)
{
  textable = (char *)malloc(TEXTABLESIZE * TEXCHARNUM);
  texindextable = (TPIndex *)malloc(TEXTABLESIZE * sizeof(TPIndex));
  memset(textable, 0, TEXTABLESIZE * TEXCHARNUM);
  memset(texindextable, 0, TEXTABLESIZE * sizeof(TPIndex));
}

void DeleteTexTable(void)
{
  free(textable);
  free(texindextable);
}

void SetTexName
(
 int texindex,
 char *name
 )
{
  char *ptr;
  ptr = textable + texindex * TEXCHARNUM;
  strcpy(ptr, name);
  makelower(ptr);
  lastcount++;
}

char *GetTexName
(
 int texindex
 )
{
  char *ptr;
  ptr = textable + texindex * TEXCHARNUM;
  return ptr;
}

int SetTexEntry
(
 char *name,
 int texpage,
 int subpage
 )
{
  char *ptr;
  int i;
  int flag = 0;
	
  for (i = 0; i < lastcount; i++)
    {
      ptr = textable + i * TEXCHARNUM;
      if (!strcmp(name, ptr))
	{
	  texindextable[i].texpage = texpage;
	  texindextable[i].subpage = subpage;
#if 0
	  printf("name = %s, texpage = %d, subpage = %d\n"
		 , ptr, texindextable[i].texpage, texindextable[i].subpage);
#endif
	  flag = 1;
	}
    }
  return flag;
}

int GetUsedTexCount(void)
{
  int i;
  int count = 0;
	
  for (i = 0; i < gTexpage; i++)
    tp[i] = 0;
  /* memset(tp, 0, gTexpage * sizeof(int)); */
	
  for (i = 0; i < lastcount; i++)
    {
      if (texindextable[i].used)
	tp[texindextable[i].texpage] = 1;
    }
  for (i = 0; i < gTexpage; i++)
    {
      if (tp[i])
	count++;
    }
  return count;
}

int GetTexEntry
(
 int texindex,
 char *name,
 int *texpage,
 int *subpage
 )
{
  char *ptr;
	
  if (texindex < 0)
    {
      *texpage = -1;
      *subpage = -1;
      return 0;
    }
  if (texindex >= lastcount)
    return 0;
	
  ptr = textable + texindex * TEXCHARNUM;
  if (name)
    strcpy(name, ptr);
  *texpage = texindextable[texindex].texpage;
  *subpage = texindextable[texindex].subpage;
  texindextable[texindex].used = 1;
#if 0
  if (name)
    printf("texindex = %d, name = %s, texpage = %d, subpage = %d\n"
	   , texindex , name, *texpage, *subpage);
#endif
  return 1;
}


/*
** Header file stuff
*/

static FILE* hdrfp = NULL;
static char hfile[64];

void OpenHeaderFile
(
 char *filename
 )
{
  int i, len;

  hdrfp = fopen(filename, "w");
  if (hdrfp == NULL)
    {
      printf("cannot open %s\n", filename);
      exit(0);
    }
  strcpy(hfile, filename);
  len = strlen(hfile);
  for (i = 0; i < len; i++)
    if (hfile[i] == '.')
      hfile[i] = 0;
  fprintf(hdrfp, "#ifndef _H_%s\n", hfile);
  fprintf(hdrfp, "#define _H_%s\n\n", hfile);
}

void CloseHeaderFile(void)
{
  if (hdrfp)
    {
      fprintf(hdrfp, "\n#endif\n");
      fclose(hdrfp);
    }
}

void AddDefine
(
 char *name,
 int	 num
 )
{
  char nname[64];
  int	 i, len;
  if(!hdrfp)
    return;
  strcpy(nname, name);
  len = strlen(nname);
  if (len > 64)
    {
      printf("ERROR: name length exceed 64, make it smaller\n");
      exit(0);
    }
  for (i = 0; i < len; i++)
    if (nname[i] == '.')
      nname[i] = '_';
  fprintf(hdrfp, "#define %s_%s", hfile, name);
  len = 64 - len;
  for (i = 0; i < len; i++)
    fprintf(hdrfp, " ");
  fprintf(hdrfp, "%d\n", num);
}

int HasHeaderFile(void)
{
  if (hdrfp)
    return 1;
  return 0;
}

