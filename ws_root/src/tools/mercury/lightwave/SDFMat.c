/*
	File:		SDFMat.c

	Contains:	Handles SDF Materials

	Written by:	Todd Allendorf

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		<2+>	12/15/95	TMA		Added support for TexBlend parsing.

	To Do:
*/


#include <math.h>
#include "ifflib.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "M2TXTypes.h"
#include "M2TXattr.h"
#include "LWSURF.h"
#include "SDFMat.h"
#include "qmem.h"

#include "LWToken.h"
#include <assert.h>
#include "os.h"
#include "lws.i"
#include "lws.h"

extern LWS  *cur_lws;
extern int  tokenType;		/* type of input token */
extern char token[256];			/* input token buffer */


extern int CommentLevel;
extern bool NoAmbient;
extern double AmbientScale;

#ifdef applec
int strcasecmp(char *s, char *t);
#endif

#define FP_CLOSE(s, t, delta) (fabs((s) - (t)) <= (delta))
/*
bool FP_CLOSE(double s, double t, double delta)
{
  if ((fabs(s-t)) < delta)
    return TRUE;
  else
    return FALSE;
}
*/
void SDFMat_Init(SDFMat *mat)
{
  mat->ValSet = 0;
  mat->OrigName = NULL;
}


M2Err LWSURF_ToSDFMat(LWSURF surf, SDFMat *mat, char *namePtr)
{
  uint8 red, green, blue;
  uint32 tempColor;
  uint16 diff, lumi, glos, spec, tran;
  bool   flag;

 mat->ValSet = 0;
 
 if (!LWSURF_GetRed(surf,&red))
   red = 128;
 if (!LWSURF_GetGreen(surf,&green))
   green = 128;
 if (!LWSURF_GetBlue(surf,&blue))
   blue = 128;
 if (!LWSURF_GetTRAN(surf,&tran))
   tran = 0;

 if (LWSURF_GetDIFF(surf,&diff))
   {
     mat->diffuse[0] = (red/255.0)*(diff/256.0);
     mat->diffuse[1] = (green/255.0)*(diff/256.0);
     mat->diffuse[2] = (blue/255.0)*(diff/256.0);
     mat->diffuse[3] = 1.0 - (tran/256.0);
     mat->ValSet |= SDFMAT_DIFFUSE_MASK;
   }      

 if (LWSURF_GetCOLR(surf,&tempColor))
   {
     if (NoAmbient==FALSE)
       {
	 mat->ambient[0] =  AmbientScale*(red/255.0);
	 mat->ambient[1] =  AmbientScale*(green/255.0);
	 mat->ambient[2] =  AmbientScale*(blue/255.0);
	 mat->ambient[3] = 1.0 - (tran/256.0);
	 mat->ValSet |= SDFMAT_AMBIENT_MASK;
       }
   }

 if (LWSURF_GetSPEC(surf,&spec))
   {
     if (!LWSURF_GetFColHilite(surf,&flag))
       {
	 mat->specular[0] =  (spec/256.0)*(red/255.0);
	 mat->specular[1] =  (spec/256.0)*(green/255.0);
	 mat->specular[2] =  (spec/256.0)*(blue/255.0);
       }
     else
       {
	 mat->specular[0] =  (spec/256.0);
	 mat->specular[1] =  (spec/256.0);
	 mat->specular[2] =  (spec/256.0);
       }
     mat->specular[3] = 1.0 - (tran/256.0);
     mat->ValSet |= SDFMAT_SPECULAR_MASK;
   }


 if (LWSURF_GetLUMI(surf,&lumi))
   {
     mat->emission[0] =  (lumi/256.0)*(red/255.0);
     mat->emission[1] =  (lumi/256.0)*(green/255.0);
     mat->emission[2] =  (lumi/256.0)*(blue/255.0);
     mat->emission[3] =  1.0 - (tran/256.0);
     mat->ValSet |= SDFMAT_EMISSION_MASK;
   }
 else if (!LWSURF_GetFLuminous(surf,&flag))
   {
     mat->emission[0] =  (red/255.0);
     mat->emission[1] =  (green/255.0);
     mat->emission[2] =  (blue/255.0);
     mat->emission[3] =  1.0 - (tran/256.0);
     mat->ValSet |= SDFMAT_EMISSION_MASK;
   }

 if (LWSURF_GetGLOS(surf,&glos))
   {
     /*
       if (glos==0)
       glos=1;
       mat->shine = 1.0/(double)glos;
       */
     mat->shine = (double)glos/256.0;
     mat->ValSet |= SDFMAT_SHINE_MASK;
   }

 if (LWSURF_GetF2Sided(surf,&flag))
   {
     if (flag)
       mat->ValSet |= SDFMAT_F2SIDED_MASK;
   }
 mat->OrigName = namePtr;

return(M2E_NoErr);

}


M2Err LWSURF_MakeWhiteMat(LWSURF surf, float intensity, SDFMat *mat)
{

  uint8 red, green, blue;
  uint32 tempColor;
  uint16 diff, lumi, glos, spec, tran;
  bool   flag;

 mat->ValSet = 0;
 
 
 if (!LWSURF_GetRed(surf,&red))
   red = 128;
 if (!LWSURF_GetGreen(surf,&green))
   green = 128;
 if (!LWSURF_GetBlue(surf,&blue))
   blue = 128;
 if (!LWSURF_GetTRAN(surf,&tran))
   tran = 0;

 if (LWSURF_GetDIFF(surf,&diff))
   {
     mat->diffuse[0] = intensity*(diff/256.0);
     mat->diffuse[1] = intensity*(diff/256.0);
     mat->diffuse[2] = intensity*(diff/256.0);
     mat->diffuse[3] = 1.0 - (tran/256.0);
     mat->ValSet |= SDFMAT_DIFFUSE_MASK;
   }      

 if (LWSURF_GetCOLR(surf,&tempColor))
   {
     if (!NoAmbient)
       {
	 
	 mat->ambient[0] =  AmbientScale*intensity;
	 mat->ambient[1] =  AmbientScale*intensity;
	 mat->ambient[2] =  AmbientScale*intensity;
	 mat->ambient[3] = 1.0 - (tran/256.0);
	 mat->ValSet |= SDFMAT_AMBIENT_MASK;
       }
   }

 if (LWSURF_GetSPEC(surf,&spec))
   {
     if (!LWSURF_GetFColHilite(surf,&flag))
       {
	 mat->specular[0] =  (spec/256.0)*(red/255.0);
	 mat->specular[1] =  (spec/256.0)*(green/255.0);
	 mat->specular[2] =  (spec/256.0)*(blue/255.0);
       }
     else
       {
	 mat->specular[0] =  (spec/256.0);
	 mat->specular[1] =  (spec/256.0);
	 mat->specular[2] =  (spec/256.0);
       }
     mat->specular[3] = 1.0 - (tran/256.0);
     mat->ValSet |= SDFMAT_SPECULAR_MASK;
   }


 if (LWSURF_GetLUMI(surf,&lumi))
   {
     mat->emission[0] =  (lumi/256.0)*intensity;
     mat->emission[1] =  (lumi/256.0)*intensity;
     mat->emission[2] =  (lumi/256.0)*intensity;
     mat->emission[3] =  1.0 - (tran/256.0);
     mat->ValSet |= SDFMAT_EMISSION_MASK;
   }
 else if (!LWSURF_GetFLuminous(surf,&flag))
   {
     mat->emission[0] =  intensity;
     mat->emission[1] =  intensity;
     mat->emission[2] =  intensity;
     mat->emission[3] =  1.0 - (tran/256.0);
     mat->ValSet |= SDFMAT_EMISSION_MASK;
   }

 if (LWSURF_GetGLOS(surf,&glos))
   {
     if (glos==0)
       glos=1;
     mat->shine = 1.0/(double)glos;
     mat->ValSet |= SDFMAT_SHINE_MASK;
   }

 if (LWSURF_GetF2Sided(surf,&flag))
   {
     if (flag)
       mat->ValSet |= SDFMAT_F2SIDED_MASK;
   }
 mat->OrigName = NULL;   /* Best for now, it's a hack */

return(M2E_NoErr);


} 

M2Err SDFMat_Add(SDFMat *mat, SDFMat **Materials, int *NMats, int *CurMat, 
		  bool collapse, int *MatIndex)
{
  int j, curMat, nMats, matIndex;
  SDFMat *mats, *temp;

  mats = *Materials;
  curMat = *CurMat;
  nMats = *NMats;
  matIndex = -1;
  
  if (collapse ==TRUE)
    for (j=0; j<curMat; j++)
      {
	if (SDFMat_Compare(mat, &(mats[j])))
	  {
	    matIndex = j;
	    break;
	  }
      }

  if (matIndex == -1)
    {
      matIndex = curMat;
      /* Check if the material size is exceeded*/
      
      curMat++;
      if (curMat >= nMats)
	{
	  nMats += MAT_BUF_SIZE;
	  temp = qMemResizePtr(mats, nMats*sizeof(SDFMat));
	  if (temp == NULL)
	    {
	      fprintf(stderr,"ERROR:Out of memory in SDFMat_Add!\n");
	      return(M2E_NoMem);
	    }
	  else 
	    mats = temp;
	}
      SDFMat_Copy(&(mats[matIndex]), mat);
    }

  *Materials = mats;
  *CurMat = curMat;
  *NMats = nMats;
  *MatIndex = matIndex;

  return(M2E_NoErr);
}

bool SDFMat_Compare(SDFMat *mat1, SDFMat *mat2)
{
  int i;
  
  if ( (mat1->ValSet)!= (mat2->ValSet))
    return(FALSE);
  
  if (mat1->ValSet & SDFMAT_DIFFUSE_MASK)
    {
      for (i=0; i<4; i++)
	if ( !(FP_CLOSE(mat1->diffuse[i], mat2->diffuse[i], 0.6/256.0)))
	  return(FALSE);
    }
  
  if (mat1->ValSet & SDFMAT_AMBIENT_MASK)
    {
      for (i=0; i<4; i++)
	if ( !(FP_CLOSE(mat1->ambient[i], mat2->ambient[i], 0.6/256.0)))
	  return(FALSE);
    }
  
  if (mat1->ValSet & SDFMAT_SPECULAR_MASK)
    {
      for (i=0; i<4; i++)
	if ( !(FP_CLOSE(mat1->specular[i], mat2->specular[i], 0.6/256.0)))
	  return(FALSE);
    }
  
  if (mat1->ValSet & SDFMAT_EMISSION_MASK)
    {
      for (i=0; i<4; i++)
	if ( !(FP_CLOSE(mat1->emission[i], mat2->emission[i], 0.6/256.0)))
	  return(FALSE);
    }  

  if (mat1->ValSet & SDFMAT_SHINE_MASK)
    {
      if ( !(FP_CLOSE(mat1->shine, mat2->shine, 0.6/256.0)))
	return(FALSE);
    }  
  
  return (TRUE);
}

void SDFMat_Copy(SDFMat *mat1, SDFMat *mat2)
{
  int i;
  
  for(i=0; i<4; i++)
    {
      mat1->diffuse[i] = mat2->diffuse[i];
      mat1->ambient[i] = mat2->ambient[i];
      mat1->specular[i] = mat2->specular[i];
      mat1->emission[i] = mat2->emission[i];
    }
  mat1->shine = mat2->shine;
  mat1->ValSet = mat2->ValSet;
  mat1->OrigName = mat2->OrigName;
}


static void Indent(FILE *fPtr, int tab)
{
  int i;
  for(i=0; i<tab; i++)
    fprintf(fPtr,"\t");
}

void SDFMat_Print(SDFMat *mat, FILE *fPtr, int tab)
{
  bool set;
  
  Indent(fPtr, tab);
  fprintf(fPtr,"{\n");

  if (mat->OrigName!=NULL)
    {
      Indent(fPtr, tab+1);
      fprintf(fPtr,"#Define Material \"%s\" {\n",mat->OrigName);
    }
  if ((mat->ValSet) & SDFMAT_DIFFUSE_MASK)
    {
      Indent(fPtr, tab+1);
      fprintf(fPtr,"diffuse {%.5f %.5f %.5f %.5f}\n",
	      mat->diffuse[0], mat->diffuse[1], mat->diffuse[2],
	      mat->diffuse[3]);
    }
  if ((mat->ValSet) & SDFMAT_AMBIENT_MASK)
    {
      Indent(fPtr, tab+1);
      fprintf(fPtr,"ambient {%.5f %.5f %.5f %.5f}\n",
	      mat->ambient[0], mat->ambient[1], mat->ambient[2],
	      mat->ambient[3]);
    }

  if ((mat->ValSet) & SDFMAT_SPECULAR_MASK)
    {
      Indent(fPtr, tab+1);
      fprintf(fPtr,"specular {%.5f %.5f %.5f %.5f}\n",
	      mat->specular[0], mat->specular[1], mat->specular[2],
	      mat->specular[3]);
    }

  if ((mat->ValSet) & SDFMAT_EMISSION_MASK)
    {
      Indent(fPtr, tab+1);
      fprintf(fPtr,"emission {%.5f %.5f %.5f %.5f}\n",
	      mat->emission[0], mat->emission[1], mat->emission[2],
	      mat->emission[3]);
    }

  if ((mat->ValSet) & SDFMAT_SHINE_MASK)
    {
      Indent(fPtr, tab+1);
      fprintf(fPtr,"shine %.5f\n", mat->shine);
      if (!((mat->ValSet) & SDFMAT_SPECULAR_MASK))
	{
	  Indent(fPtr, tab+1);
	  fprintf(fPtr,"specular {1.0 1.0 1.0 1.0}\n");
	}
    }

  Indent(fPtr, tab+1);
  fprintf(fPtr,"shadeenable(");
  set = FALSE;
  if ((mat->ValSet) & SDFMAT_DIFFUSE_MASK)
    {
      set = TRUE;
      fprintf(fPtr," diffuse");
    }
  if ((mat->ValSet) & SDFMAT_AMBIENT_MASK)
    {
      if (set)
	fprintf(fPtr," |");
      else
	set = TRUE;
      fprintf(fPtr," ambient");	  
    }
  if (((mat->ValSet) & SDFMAT_SPECULAR_MASK) ||
      ((mat->ValSet) & SDFMAT_SHINE_MASK))
    {
      if (set)
	fprintf(fPtr," |");
      else
	set = TRUE;
      fprintf(fPtr," specular");	  
    }
  if ((mat->ValSet) & SDFMAT_EMISSION_MASK)
    {
      if (set)
	fprintf(fPtr," |");
      else
	set = TRUE;
      fprintf(fPtr," emissive");	  
    }
  if ((mat->ValSet) & SDFMAT_F2SIDED_MASK)
    {
      if (set)
	fprintf(fPtr," |");
      else
	set = TRUE;
      fprintf(fPtr," twosided");	  
    }
  fprintf(fPtr," )\n");
      
  Indent(fPtr, tab);
  fprintf(fPtr,"}\n");

}

M2Err Material_Keyword(int32 *key)
{

  *key = -1;

  if(!strcasecmp(token, "diffuse"))
    *key = SDFMAT_DIFFUSE_MASK;
  else if(!strcasecmp(token, "ambient"))
    *key = SDFMAT_AMBIENT_MASK;
  else if(!strcasecmp(token, "specular"))
    *key = SDFMAT_SPECULAR_MASK;
  else if(!strcasecmp(token, "emission"))
    *key = SDFMAT_EMISSION_MASK;
  else if(!strcasecmp(token, "shine"))
    *key = SDFMAT_SHINE_MASK;
  else if(!strcasecmp(token, "shadeenable"))
    *key = SDFMAT_ENABLE_MASK;
  else
    {
      BadToken("Unrecognized Material Keyword:",token);
      return(M2E_Range);
    }
  return(M2_NoErr);
}

M2Err Material_ShadeEnable(uint16 *value)
{
  
  
  lws_get_token();
  if (tokenType != T_LPAREN)
    {
      BadToken("Parsing color value, needed a left brace.",token);
      return(M2E_Range);
    }
  
  do 
    {
      lws_get_token();
      
      if(!strcmp(token, "|"))
	;
      else if(tokenType==T_RPAREN)
	{
	  return(M2E_NoErr);
	}
      else if(!strcasecmp(token, "diffuse"))
	*value |= SDFMAT_DIFFUSE_MASK;
      else if(!strcasecmp(token, "ambient"))
	*value |= SDFMAT_AMBIENT_MASK;
      else if(!strcasecmp(token, "specular"))
	*value |= SDFMAT_SPECULAR_MASK;
      else if(!strcasecmp(token, "emissive"))
	*value |= SDFMAT_EMISSION_MASK;
      else if(!strcasecmp(token, "twosided"))
	*value |= SDFMAT_F2SIDED_MASK;
      else
	{
	  BadToken("Unrecognized ShadeEnable Flag:",token);
	  return(M2E_Range);
	}
    } while (tokenType != T_RPAREN);

  return(M2E_NoErr);
}

M2Err Material_Float(gfloat *value)
{
  if(lws_read_float(value))	
    return(M2E_NoErr);
  else
    return(M2E_Range);
}

M2Err Material_Color4(gfloat value[4])
{
  int i, lastRead;

  lws_get_token();
  if (tokenType != T_LBRACE)
    {
      BadToken("Parsing color value, needed a left brace.",token);
      return(M2E_Range);
    }
  lws_get_token();
  if (tokenType != T_NUMBER)
    {
      BadToken("Parsing color value, needed a number.",token);
      return(M2E_Range);
    }
  else
    sscanf(token, "%f", &(value[0]));
  lastRead = 0;
  while (lastRead<3)
    {
      lws_get_token();
      if (tokenType == T_RBRACE)
	{
	  for (i=lastRead+1; i<3; i++)
	    value[i]=value[lastRead];
	  value[3] = 1.0;
	  return(M2E_NoErr);
	}
      else if (tokenType != T_NUMBER)
	{
	  BadToken("Parsing color value, needed a number or brace.",token);
	  return(M2E_Range);
	}
      else
	sscanf(token, "%f", &(value[lastRead+1]));
      lastRead++;
    }
  lws_get_token();
  if (tokenType == T_RBRACE)
    {
      return(M2E_NoErr);
    }
  else 
    {
      BadToken("Parsing color value, needed a right brace.",token);
      return(M2E_Range);
    }
  return(M2E_NoErr);
}

M2Err Material_Eval(SDFMat *mat, int32 command)
{
  M2Err err;

  err = M2E_NoErr;
  switch(command)
    {
    case SDFMAT_DIFFUSE_MASK:
      err = Material_Color4(mat->diffuse);
      break;
    case SDFMAT_AMBIENT_MASK:
      err = Material_Color4(mat->ambient);
      break;
    case SDFMAT_SPECULAR_MASK:
      err = Material_Color4(mat->specular);
      break;
    case SDFMAT_EMISSION_MASK:
      err = Material_Color4(mat->emission);
      break;
    case SDFMAT_SHINE_MASK:
      err = Material_Float(&(mat->shine));
      mat->ValSet |= SDFMAT_SHINE_MASK;
      break;
    case SDFMAT_ENABLE_MASK:
      err = Material_ShadeEnable(&(mat->ValSet));
    }
  
  return(err);
}

M2Err Material_Entry(SDFMat *mat)
{
  M2Err err;
  int32 value;
  int curMat = 0;
  int newToken;
  
  if (tokenType == T_RBRACE)
    return (M2E_NoErr);
  do
    {
      do
	{
	  newToken = lws_get_token();
	  if (newToken == T_RBRACE)
	    return (M2E_NoErr);
	} while (newToken != T_KEYWORD);
      err = Material_Keyword(&value);
      if (err != M2E_NoErr)
	return(err);
      if (value != -1)
	err = Material_Eval(mat, value);
      if (err != M2E_NoErr)
	return(err);
    } while (1);
  return(M2E_NoErr);
}

M2Err Material_ReadFile(char *fileIn, SDFMat **myMat, int *numMat, int *curMat)
{
  LWS lws;
  ByteStream*		stream = NULL;
  int nMaterials = MAT_BUF_SIZE;
  SDFMat *materials, *temp;
  int cMat = 0;
  M2Err err;
  
  
  *myMat = NULL;
  materials = (SDFMat *)qMemClearPtr(nMaterials, sizeof(SDFMat));
  if (materials == NULL)
    {
      fprintf(stderr,"ERROR:Out of memory in Material_ReadFile!\n");
      return(M2E_NoMem);
    }
  strcpy(lws.FileName, fileIn);
  stream = K9_OpenByteStream(lws.FileName, Stream_Read, 0);
  if (stream == NULL)
    {
      return (M2E_BadFile);
    }
  lws.Stream = stream;
  lws.Parent = cur_lws;
  cur_lws = &lws;

  while (lws_get_token()!= T_END)
    {
      switch (tokenType)
	{
	case T_KEYWORD:
	  if (!strcasecmp(token, "MatArray"))
	    {
	      if (lws_get_token() != T_KEYWORD)
		{
		  fprintf(stderr,"ERROR:Name expected. Got \"%s\" at line %d\n", token,
			  cur_lws->Lines);
		  return(M2E_BadFile);
		}
	      
	      if (!lws_check_token(T_LBRACE))
		return(M2E_BadFile);
	      while ((tokenType != T_RBRACE) && (tokenType != T_END))
		{
		  err = Material_Entry(&(materials[cMat]));
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"ERROR:Bad reading in Material_Entry\n");
		      return(err);
		    }
		  cMat++;
		  if (cMat >= nMaterials)
		    {
		      nMaterials += MAT_BUF_SIZE;
		      temp = qMemResizePtr(materials, nMaterials*sizeof(SDFMat));
		      if (temp == NULL)
			{
			  fprintf(stderr,"ERROR:Out of memory in Material_ReadFile!\n");
			  return(M2E_NoMem);
			}
		      else
			materials = temp;
		      SDFMat_Init(&(materials[cMat]));
		    }
		  if (lws_get_token()== T_END)
		    break;
		}
	    }
	  break;
	default:
	  break;
	}
    }
  *myMat = materials;
  *numMat = nMaterials;
  *curMat = cMat;
  return(M2E_NoErr);
}

