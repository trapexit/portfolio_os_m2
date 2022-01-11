/*
	File:		utfmerge.c

	Contains:	Takes one file and merges a given component of another onto it

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <5>	 7/15/95	TMA		Autodocs updated.
		 <4>	 5/31/95	TMA		Standalone version missing a variable.
		 <3>	 5/31/95	TMA		Removed unused variables.
		 <2>	 5/16/95	TMA		Autodocs added. Now actively tries to coerse a legal output
									format if possible.
	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name utfmerge
|||	Merges a component of one file into the input file.
|||	
|||	  Synopsis
|||	
|||	    utfmerge <input file> <merge file> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool takes an input UTF file and a "merge" file and overrides the
|||	    specified component of the input file with that of the merge file.
|||	
|||	  Caveats
|||	
|||	    You may sometimes come up with an illegal format when trying to a 
|||	    component (rather than overriding) to an existing texture.  For instance,
|||	    an indexed 7-bit color image with an ssb component cannot be merged with
|||	    a 7-bit alpha channel (a fifteen bit texture would result, which is 
|||	    illegal) so the tool will output an 8-bit color image with a 7-bit alpha
|||	    component and an SSB component.  The program will do it's best to make a
|||	    legal format and will warn you if the resulting output is not legal.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <merge file>
|||	        The UTF texture that will override a specific component of the input.
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  Options
|||	
|||	    -color
|||	        Override the color component of the input with that of the merge file.
|||	    -alpha
|||	        Override the alpha component of the input with that of the merge file.
|||	    -ssb
|||	        Override the ssb component of the input with that of the merge file.
|||	
|||	  Examples
|||	
|||	    utfmerge check.rgb.utf check.rgba.utf -alpha checkout.rgba.utf
|||	
|||	    This takes the alpha channel from check.rgba.utf and combines it with 
|||	    check.rgb.utf and writes the resulting file out into checkout.rgba. 
|||	    If check.rgb.utf has an alpha channel already, it will be overridden. 
|||	
|||	  See Also
|||	
|||	    utfmakelod, utffit
**/

#include "M2TXlib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF Merge Channels\n");
  printf("   -color \tUse the color channel of the merge file\n");
  printf("   -alpha \tUse the alpha channel of the merge file\n");
  printf("   -ssb \tUse the alpha channel of the merge file\n");
}

int main( int argc, char *argv[] )
{
	M2TX tex, newTex, colorTex, alphaTex, ssbTex;
	M2TXPIP *oldPIP, *newPIP, *colorPIP, *alphaPIP, *ssbPIP;
	M2TXRaw raw, colorRaw, alphaRaw, ssbRaw;
	M2TXHeader *header, *newHeader, *colorHeader, *alphaHeader, *ssbHeader;
	char fileIn[256];
	char mergeField[256];
	char fileMerge[256];
	char fileOut[256];
	bool mAlpha = FALSE;
	bool mColor = FALSE;
	bool mSSB = FALSE;
	int done;
  bool flag, isCompressed, isLiteral;
  bool hasColor, hasAlpha, hasSSB;
  bool needSSB = FALSE;
	uint16 xSize, ySize, mXSize, mYSize;
	FILE *fPtr;
	uint8 numLOD, nMLOD, i;
  uint8 cDepth, aDepth, ssbDepth;
	M2Err err;
	int argn;
		
#ifdef M2STANDALONE
	printf("Enter: <FileIn> <MergeFile> <FileOut>\n");
	printf("Example: dumb.rgb.utf dumb.alpha.utf dumb.rgba.utf\n");
	fscanf(stdin,"%s %s %s",fileIn, fileMerge, fileOut);
	printf("Enter chunk to merge onto input file \"alpha\", \"color\", or \"ssb\".\n");
	done = FALSE;
	do
	{
		fscanf(stdin,"%s",mergeField);
		if (strcmp(mergeField,"color")==0)
		{
			done = mColor = TRUE;
		}
		else if (strcmp(mergeField,"alpha")==0)
		{
			done = mAlpha = TRUE;
		}
		else if (strcmp(mergeField,"ssb")==0)
		{
			done = mSSB = TRUE;
		}
		else
			printf("Error.  Enter \"alpha\", \"color\", or \"ssb\".\n");
	}
	while (!done);

#else
    /* Check for command line options. */
    if (argc != 5)
      {
	fprintf(stderr,"Usage: %s <Input File> <Merge File> [<-color>|<-alpha>|<-ssb>] <Output File>\n",argv[0]);
	print_description();
	return(-1);    
      }
	strcpy(fileIn, argv[1]);
	strcpy(fileMerge, argv[2]);
	
	argn = 3;
	while ((argn<argc) && (argv[argn][0] == '-') && (argv[argn][1] != '\0') )
	  {
	    if ( strcmp( argv[argn], "-color")==0 )
	      {
     		++argn;
		mColor = TRUE;
	      }
	    else if ( strcmp( argv[argn], "-alpha")==0 )
	      {
     		++argn;
		mAlpha = TRUE;
	      }
	    else if ( strcmp( argv[argn], "-ssb")==0 )
	      {
     		++argn;
		mSSB = TRUE;
	      }
	    else
	      {
		fprintf(stderr,"Usage: %s <Input File> <Merge File> [<-color>|<-alpha>|<-ssb>] <Output File>\n",argv[0]);
		return(-1);
	      }
	  }
	
    if ( argn != argc )
    {
        /* Open the input file. */
     	 strcpy( fileOut, argv[argn] );
    }
    else
    {
        /* No output file specified. */
      fprintf(stderr,"Usage: %s <Input File> <Merge File> [<-color>|<-alpha>|<-ssb>] <Output File>\n",argv[0]);
    	return(-1);   
    }
#endif
	fPtr = fopen(fileIn, "r");
	if (fPtr == NULL)
	{
	  fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileIn);
		return(-1);
	}
	else 
		fclose(fPtr);

	M2TX_Init(&tex);						/* Initialize a texture */
	M2TX_Init(&newTex);						/* Initialize a texture */
	err = M2TX_ReadFile(fileIn,&tex);
	if (err != M2E_NoErr)
	  {
	    fprintf(stderr,"ERROR:Bad file \"%s\".\n",fileIn);
	      return(-1);
	  }
	M2TX_GetHeader(&tex,&header);
	
	M2TX_Copy(&newTex, &tex);
	/* M2TX_SetHeader(&newTex, header); */

	M2TX_GetHeader(&newTex,&newHeader);
	M2TXHeader_GetFIsCompressed(header, &isCompressed);
	M2TXHeader_SetFIsCompressed(newHeader, FALSE);
	M2TXHeader_GetNumLOD(header, &numLOD);
	err = M2TXHeader_GetMinXSize(header, &xSize);
	err = M2TXHeader_GetMinYSize(header, &ySize);
	M2TX_GetPIP(&tex,&oldPIP);
	M2TX_SetPIP(&newTex, oldPIP);

	if (mColor)
	{
		M2TX_Init(&colorTex);						/* Initialize a texture */
		err = M2TX_ReadFile(fileMerge,&colorTex);
		if (err != M2E_NoErr)
		  {
		    fprintf(stderr,"ERROR:Bad file \"%s\".\n",fileMerge);
		    return(-1);
		  }
		M2TX_GetHeader(&colorTex,&colorHeader);
		M2TXHeader_GetFIsLiteral(colorHeader, &flag);
		M2TXHeader_SetFIsLiteral(newHeader, flag);
		M2TXHeader_GetFHasColor(colorHeader, &flag);
		M2TXHeader_SetFHasColor(newHeader, flag);
		M2TXHeader_GetCDepth(colorHeader, &cDepth);
		M2TXHeader_SetCDepth(newHeader, cDepth);
		M2TXHeader_GetNumLOD(colorHeader, &nMLOD);
		if (numLOD != nMLOD)
		{
		  fprintf(stderr,"ERROR:Number of LODs don't match. Abortting.\n");
			return(-1);
		}
		err = M2TXHeader_GetMinXSize(colorHeader, &mXSize);
		err = M2TXHeader_GetMinYSize(colorHeader, &mYSize);
		M2TX_GetPIP(&colorTex, &colorPIP);
		M2TX_SetPIP(&newTex, colorPIP);
		if ((xSize != mXSize) || (ySize != mYSize))
		{
		  fprintf(stderr,"ERROR:Image sizes don't match. Abortting.\n");
			return(-1);
		}
	}
	
	if (mAlpha)
	{
		M2TX_Init(&alphaTex);						/* Initialize a texture */
		err = M2TX_ReadFile(fileMerge,&alphaTex);
		if (err != M2E_NoErr)
		  {
		    fprintf(stderr,"ERROR:Bad file \"%s\".\n",fileMerge);
		    return(-1);
		  }
		M2TX_GetHeader(&alphaTex,&alphaHeader);
		M2TXHeader_GetFHasAlpha(alphaHeader, &flag);
		M2TXHeader_SetFHasAlpha(newHeader, flag);
		M2TXHeader_GetADepth(alphaHeader, &aDepth);
		M2TXHeader_SetADepth(newHeader, aDepth);
		M2TXHeader_GetNumLOD(alphaHeader, &nMLOD);
		if (numLOD != nMLOD)
		{
		  fprintf(stderr,"ERROR:Number of LODs don't match. Abortting.\n");
			return(-1);
		}
		err = M2TXHeader_GetMinXSize(alphaHeader, &mXSize);
		err = M2TXHeader_GetMinYSize(alphaHeader, &mYSize);
		if ((xSize != mXSize) || (ySize != mYSize))
		{
		  fprintf(stderr,"ERROR:Image sizes don't match. Abortting.\n");
			return(-1);
		}	
		M2TX_GetPIP(&alphaTex, &alphaPIP);
	}
	if (mSSB)
	{
		M2TX_Init(&ssbTex);						/* Initialize a texture */
		err = M2TX_ReadFile(fileMerge,&ssbTex);
		if (err != M2E_NoErr)
		  {
		    fprintf(stderr,"ERROR:Bad file \"%s\".\n",fileMerge);
		    return(-1);
		  }
		M2TX_GetHeader(&ssbTex,&ssbHeader);
		M2TXHeader_GetFHasSSB(ssbHeader, &flag);
		M2TXHeader_SetFHasSSB(newHeader, flag);
		M2TXHeader_GetNumLOD(ssbHeader, &nMLOD);
		if (numLOD != nMLOD)
		{
		  fprintf(stderr,"ERROR:Number of LODs don't match. Abortting.\n");
		  return(-1);
		}
		err = M2TXHeader_GetMinXSize(ssbHeader, &mXSize);
		err = M2TXHeader_GetMinYSize(ssbHeader, &mYSize);
		if ((xSize != mXSize) || (ySize != mYSize))
		{
		  fprintf(stderr,"ERROR:Image sizes don't match. Abortting.\n");
			return(-1);
		}	
		M2TX_GetPIP(&ssbTex, &ssbPIP);
	}

	M2TX_GetPIP(&newTex, &newPIP);

  M2TXHeader_GetFHasColor(newHeader, &hasColor);
  if (hasColor)
    M2TXHeader_GetCDepth(newHeader, &cDepth);	
  else
    cDepth = 0;
  M2TXHeader_GetFHasAlpha(newHeader, &hasAlpha);
  if (hasAlpha)
    M2TXHeader_GetADepth(newHeader, &aDepth);	
  else
    aDepth = 0;
  M2TXHeader_GetFHasSSB(newHeader, &hasSSB);	
  if (hasSSB)
    ssbDepth = 1;
  else
    ssbDepth = 0;
  M2TXHeader_GetFIsLiteral(newHeader, &isLiteral);

  if (!isLiteral)
    {
      while ((!(M2TX_IsLegal(cDepth, aDepth, ssbDepth, FALSE))) && (cDepth<9))
	cDepth++;
      if (cDepth>8)
	{
	  cDepth = 8;
	  if (!hasSSB)
	    {
	      if (M2TX_IsLegal(cDepth,aDepth, 1, FALSE))
		needSSB = TRUE;
	      else
		{
		  fprintf(stderr,"WARNING:Can't find the right color, alpha, ssb combo\n");
		  fprintf(stderr,"The output type will not be a legal type\n");
		  fprintf(stderr,"Try utfmakepip to get it to a legal type\n");
		}
	    }	
	  else
	    {
	      fprintf(stderr,"WARNING:Can't find the right color, alpha, ssb combo\n");
	      fprintf(stderr,"The output type will not be a legal type\n");
	      fprintf(stderr,"Try utfmakepip to get it to a legal type\n");
	    }
	}
      M2TXHeader_SetCDepth(newHeader,cDepth);
      if (needSSB)
	M2TXHeader_SetFHasSSB(newHeader, TRUE);
    }

  for (i=0; i<numLOD; i++)
    {
      M2TXHeader_GetFIsCompressed(header, &isCompressed);
		if (isCompressed)
		{
			err = M2TX_ComprToM2TXRaw(&tex, oldPIP, i, &raw);
		}
		else
		{
			err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, i, &raw);
		}

		if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Error color during decompression, must abort.\n");
			return(-1);
		}				
		
		if (mColor)
		{
	  M2TXHeader_GetFIsCompressed(colorHeader, &isCompressed);
			if (isCompressed)
			{
				err = M2TX_ComprToM2TXRaw(&colorTex, colorPIP, i, &colorRaw);
			}
			else
			{
				err = M2TX_UncomprToM2TXRaw(&colorTex, colorPIP, i, &colorRaw);
			}

			if (err != M2E_NoErr)
			{
	      fprintf(stderr,"ERROR: Error during color decompression, must abort.\n");
				return(-1);
			}
			if (colorRaw.Alpha != NULL)
				free (colorRaw.Alpha);
			if (colorRaw.SSB != NULL)
				free(colorRaw.SSB);
			if (raw.Red != NULL)
				free(raw.Red);
			if (raw.Green != NULL)
				free(raw.Green);
			if (raw.Blue != NULL)
				free(raw.Blue);
	  if (colorRaw.Red == NULL)
	    raw.HasColor = FALSE;
	  else
	    raw.HasColor = TRUE;
			raw.Red = colorRaw.Red;
			raw.Green = colorRaw.Green;
			raw.Blue = colorRaw.Blue;
		}
		if (mAlpha)
		{
	  M2TXHeader_GetFIsCompressed(alphaHeader, &isCompressed);
			if (isCompressed)
			{
				err = M2TX_ComprToM2TXRaw(&alphaTex, alphaPIP, i, &alphaRaw);
			}
			else
			{
				err = M2TX_UncomprToM2TXRaw(&alphaTex, alphaPIP, i, &alphaRaw);
			}

			if (err != M2E_NoErr)
			{
	      fprintf(stderr,"ERROR:Error during alpha decompression, must abort.\n");
				return(-1);
			}
			if (alphaRaw.Red != NULL)
				free(alphaRaw.Red);
			if (alphaRaw.Green != NULL)
				free(alphaRaw.Green);
			if (alphaRaw.Blue != NULL)
				free(alphaRaw.Blue);
			if (alphaRaw.SSB != NULL)
				free(alphaRaw.SSB);
			if (raw.Alpha != NULL)
				free(raw.Alpha);
	  if (alphaRaw.Alpha == NULL)
	    raw.HasAlpha = FALSE;
	  else
	    raw.HasAlpha = TRUE;
			raw.Alpha = alphaRaw.Alpha;
		}
		if (mSSB)
		{
			  M2TXHeader_GetFIsCompressed(ssbHeader, &isCompressed);
			if (isCompressed)
			{
				err = M2TX_ComprToM2TXRaw(&ssbTex, ssbPIP, i, &ssbRaw);
			}
			else
			{
				err = M2TX_UncomprToM2TXRaw(&ssbTex, ssbPIP, i, &ssbRaw);
			}

			if (err != M2E_NoErr)
			{
	      fprintf(stderr,"ERROR:Error during ssb decompression, must abort.\n");
				return(-1);
			}
			if (ssbRaw.Red != NULL)
				free(ssbRaw.Red);
			if (ssbRaw.Green != NULL)
				free(ssbRaw.Green);
			if (ssbRaw.Blue != NULL)
				free(ssbRaw.Blue);
			if (ssbRaw.Alpha != NULL)
				free(ssbRaw.Alpha);
			if (raw.SSB != NULL)
				free(raw.SSB);

	  if (ssbRaw.SSB == NULL)
	    raw.HasSSB = FALSE;
	  else
	    raw.HasSSB = TRUE;
			raw.SSB = ssbRaw.SSB;
		}

		err = M2TXRaw_ToUncompr(&newTex, newPIP, i, &raw);
		if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
			return(-1);
		}				
		M2TXRaw_Free(&raw);
		M2TXHeader_FreeLODPtr(header,i);
	}
		
	M2TX_WriteFile(fileOut,&newTex);    			/* Write it to disk */
	M2TXHeader_FreeLODPtrs(newHeader);

	return(0);
}
