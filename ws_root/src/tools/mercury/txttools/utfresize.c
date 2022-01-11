/*
	File:		utfresize.c

	Contains:	Resize a utf file (first LOD) using the specified filters

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	 5/16/95	TMA		Autodocs added.
	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name utfresize
|||	Resizes the first LOD to the specified size and writes out a new file.
|||	
|||	  Synopsis
|||	
|||	    utfresize <input file> <x size> <y size> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file and resizes the finest level of detail to the
|||	    specified size with an optional filter.  Only a single level of detail is
|||	    written out.
|||	
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <x size>
|||	        The x dimension of the output UTF file
|||	    <y size>
|||	        The x dimension of the output UTF file
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  Options
|||	
|||	    -fc
|||	        Color channel filter complexity.
|||	    -fa
|||	        Alpha channel filter complexity.
|||	    -fs
|||	        SSB channel filter complexity.
|||	
|||	    The filter complexity means the following: 1=point sampling, 2=averaged,
|||	    3=weighted sampling, 4=sinc filter, 5=Lancz s3 filter, 6=Michell filter
|||	
|||	  Caveats
|||	
|||	    Indexed UTF files cannot be resampled with any other filter than point
|||	    sampling (complexity = 0).  Using other filters would introduce new colors
|||	    may not be in the PIP.  If you want to resample with other filters, use a
|||	    literal image, then color reduce it to the desired bitdepth.
|||	    If multiple levels of detail are needed, use utfmakelod on the output.
|||	
|||	  See Also
|||	
|||	    utfmakelod, utffit
**/


#include "M2TXlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF Resize\n");
  printf("   -fc 1-6\tColor Filter Complexity. 1=Point Sampling (default = 5 (sinc))\n");
  printf("   -fa 1-6\tAlpha Filter Complexity. 1=Point Sampling (default = 1)\n");
  printf("   -fs 1-6\tSSB Filter Complexity. 1=Point Sampling (default = 1)\n");
}

int main( int argc, char *argv[] )
{
	M2TX tex, newTex;
	M2TXRaw raw, rawLOD;
	M2TXIndex index, indexLOD;
	M2TXHeader *header, *newHeader;
	M2TXPIP *oldPIP;
	char fileIn[256];
	char fileOut[256];
	bool hasPIP, isCompressed, isLiteral, hasColor, hasAlpha, hasSSB;
	int xDim, yDim, filterValue;
	uint32 xSize, ySize;
	FILE *fPtr;
	M2Err err;
	long *curFilter;
	long colorFilter = M2SAMPLE_LANCZS3;
	long indexFilter = M2SAMPLE_POINT;
	long alphaFilter = M2SAMPLE_POINT;
	long ssbFilter = M2SAMPLE_POINT;
	int argn;
	
#ifdef M2STANDALONE	
	printf("Enter: <FileIn> <xSize> <ySize> <FileOut>\n");
	printf("Example: dumb.utf 256 256 dumb.cmp.utf\n");
	fscanf(stdin,"%s %d %d %s",fileIn, &xDim, &yDim, fileOut);

#else
    /* Check for command line options. */
    if (argc < 5)
    {
      fprintf(stderr,"Usage: %s <Input File> <xSize> <ySize> [-fc 0-6] [-fa 0-6] [-fs 0-6] <Output File>\n",argv[0]);
      print_description();
     return(-1);    
    }
    strcpy(fileIn, argv[1]);
    xDim = strtol(argv[2], NULL, 10);
    yDim = strtol(argv[3], NULL, 10);
    if (xDim == 0 || yDim == 0)
      {
	fprintf(stderr, "Cannot have zero dimension! Quitting...\n");
	return(-1);
      }

    argn = 4;
    while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
        if ( strcmp( argv[argn], "-fc")==0 )
        {
     		++argn;
			curFilter = &colorFilter;
        }
        else if ( strcmp( argv[argn], "-fa")==0 )
        {
     		++argn;
			curFilter = &alphaFilter;
        }
        else if ( strcmp( argv[argn], "-fs")==0 )
        {
     		++argn;
			curFilter = &ssbFilter;
        }

		else
		{
		  fprintf(stderr,"Usage: %s <Input File> <xSize> <ySize> [-fc 0-6] [-fa 0-6] [-fs 0-6] <Output File>\n",argv[0]);
			return(-1);
		}

		filterValue = strtol(argv[argn], NULL, 10);
		argn++;
		
		if ((filterValue > 6)||(filterValue<1))
		{
		  fprintf(stderr,"Usage: %s <Input File> <xSize> <ySize> [-fc 1-6] [-fa 1-6] [-fs 1-6] <Output File>\n",argv[0]);
			return(-1);
		}
		else
		{
			*curFilter = (1<<(filterValue));
		}	
	}
	
    if ( argn != argc )
    {
        /* Open the output file. */
     	 strcpy( fileOut, argv[argn] );
    }
    else
    {
        /* No output file specified. */
      fprintf(stderr,"Usage: %s <Input File> [-pip] [-LODSs] <Output File>\n",argv[0]);
    	return(-1);   
    }
#endif

	xSize = xDim; ySize = yDim;

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
	    fprintf(stderr,"ERROR:Bad file \"%s\" \n",fileIn);
	    return(-1);
	  }
	M2TX_GetHeader(&tex,&header);
	M2TX_GetHeader(&newTex,&newHeader);

	M2TX_Copy(&newTex, &tex);
	/*	M2TX_SetHeader(&newTex, header); */

	M2TXHeader_GetFIsCompressed(header, &isCompressed);
	M2TXHeader_GetFIsLiteral(header, &isLiteral);
	M2TXHeader_GetFHasColor(header, &hasColor);
	M2TXHeader_GetFHasAlpha(header, &hasAlpha);
	M2TXHeader_GetFHasSSB(header, &hasSSB);
	M2TXHeader_GetFHasPIP(header, &hasPIP);
	if (hasPIP)
	{
		M2TX_GetPIP(&tex,&oldPIP);
		M2TX_SetPIP(&newTex, oldPIP);
	}
		
	M2TXHeader_SetNumLOD(newHeader,1);
	err = M2TXHeader_SetMinXSize(newHeader, xSize);
	if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Bad xSize, aborting!\n");
		return(-1);
	}
	err = M2TXHeader_SetMinYSize(newHeader, ySize);
	if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Bad ySize, aborting!\n");
		return(-1);
	}
	
	if (isLiteral)
	{
		err = M2TXRaw_Init(&rawLOD, xSize, ySize, hasColor, hasAlpha, hasSSB);
		if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Allcation failed!\n");
			return(-1);
		}
	}
	else
	{
		err = M2TXIndex_Init(&indexLOD, xSize, ySize, hasColor, hasAlpha, hasSSB);
		if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Allcation failed!\n");
			return(-1);
		}
	}
	
	if (isCompressed)
	{
		if (isLiteral)
			err = M2TX_ComprToM2TXRaw(&tex, oldPIP, 0, &raw);
		else
			err = M2TX_ComprToM2TXIndex(&tex, oldPIP,0, &index);
	}
	else
	{
		if (isLiteral)
			err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, 0, &raw);
		else
			err = M2TX_UncomprToM2TXIndex(&tex, oldPIP, 0 , &index);
	}


	if (isLiteral)
	{
		if (hasColor)
		{
			err = M2TXRaw_LODCreate(&raw, colorFilter, M2Channel_Colors, &rawLOD);
			if (err != M2E_NoErr)
			{
			  fprintf(stderr,"ERROR:Color resize failed!\n");
				return(-1);
			}
		}
		if (hasAlpha)
		{
			err = M2TXRaw_LODCreate(&raw, alphaFilter, M2Channel_Alpha, &rawLOD);
			if (err != M2E_NoErr)
			{
			  fprintf(stderr,"ERROR:Alpha resize failed!\n");
				return(-1);
			}
		}
		if (hasSSB)
		{
			err = M2TXRaw_LODCreate(&raw, ssbFilter, M2Channel_SSB, &rawLOD);
			if (err != M2E_NoErr)
			{
			  fprintf(stderr,"ERROR:SSB resize failed!\n");
				return(-1);
			}
		}
		M2TXRaw_Free(&raw);
	}
	else
	{
		if (hasColor)
		{
			err = M2TXIndex_LODCreate(&index, indexFilter, M2Channel_Index, &indexLOD);
			if (err != M2E_NoErr)
			{
			  fprintf(stderr,"ERROR: Index resize failed!\n");
				return(-1);
			}
		}
		if (hasAlpha)
		{
			err = M2TXIndex_LODCreate(&index, alphaFilter, M2Channel_Alpha, &indexLOD);
			if (err != M2E_NoErr)
			{
			  fprintf(stderr,"ERROR: Alpha resize failed!\n");
				return(-1);
			}
		}
		if (hasSSB)
		{
			err = M2TXIndex_LODCreate(&index, ssbFilter, M2Channel_SSB, &indexLOD);
			if (err != M2E_NoErr)
			{
			  fprintf(stderr,"ERROR:Index resize failed!\n");
				return(-1);
			}
		}
		M2TXIndex_Free(&index);
	}

	if (isLiteral)
	{
		err = M2TXRaw_ToUncompr(&newTex, oldPIP,0, &rawLOD);
		if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
			return(-1);
		}				
		M2TXRaw_Free(&rawLOD);
	}
	else
	{
		err = M2TXIndex_ToUncompr(&newTex, oldPIP,0, &indexLOD);
		if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
			return(-1);
		}				
		M2TXIndex_Free(&indexLOD);
	}
	M2TXHeader_FreeLODPtrs(header);
	M2TX_WriteFile(fileOut,&newTex);    			/* Write it to disk */
	M2TXHeader_FreeLODPtrs(newHeader);

	return(0);

}
