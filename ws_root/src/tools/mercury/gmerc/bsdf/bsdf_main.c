/*
 *	@(#) bsdf_main.c 96/08/01 1.1
 *	Copyright 1996, The 3DO Company
 */

/**
|||	AUTODOC -public -class tools -group m2tx -name gmerc
|||	Create binary SDF geometry data for mercury engine
|||	
|||	  Synopsis
|||	
|||	    gmerc [options] <SDF_file_1 SDF_file_2 ... SDF_file_n>
|||	
|||	  Description
|||	
|||	    Gmerc is a geometry data processing tool for M2 Mercury engine.
|||	    It compiles ASCII SDF files and create binary SDF files. The binary SDF
|||	    file contains
|||	    1. Geometry data
|||	    2. Material data
|||	    3. Hierarchy data
|||	    4. Keyframe animation data
|||	
|||	  Arguments
|||	
|||	    <SDF_file_1 SDF_file_2 ... SDF_file_n>
|||	        ASCII SDF files.
|||	
|||	  Options
|||	
|||	    -s
|||	        The Triangle primitive will be interpreted as shared vertices
|||	    -f
|||	        Write out object name chunk
|||	    -h
|||	        Create a header file for object name
|||	    -c
|||	        Turn off hierarchy collapsing
|||	    -t <texpage.tex>
|||	        Texture page file name
|||	    -a <anim.bsf>
|||	        Animation file name
|||	    -b <geom.bsf>
|||	        Mercury geometry file name
|||	
|||	  Caveats
|||	
|||	    1. Provide one TexArray and one MatArray for each compilation.
|||	    2. Light, Camera, and Scene data will not be processed.
|||	    
|||	  See Also
|||	
|||	    utfpage, utfsdfpage
**/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fw.h>
#include "bsdf_proto.h"
#include "texpage.h"
#include "geoitpr.h"

/* int TexPage_Parse(char *texpagefile); */
void SetAnimFlag(uint32 flag);
void ResetMercid(void);
Err WriteBlockSize(void);

/* new parser */
#define NEW_PARSER	1
#ifdef NEW_PARSER
#include "parsertypes.h"
#include "parsererrors.h"
#include "parser.h"
extern Syntax File;
#endif

static void 
Usage(void)
{
  fprintf(stderr, "Usage: gmerc [options] <SDF_file_1 SDF_file_2 ... SDF_file_n>\n" );
  fprintf(stderr, "Description:\n" );
  fprintf(stderr, "   Version 3.1 b1\n" );
  fprintf(stderr, "   Geometry Compiler for Mercury\n" );
  fprintf(stderr, "   -s               The Triangle primitive will be interpreted as shared vertices\n");
  fprintf(stderr, "   -f               Write out object name chunk\n");
  fprintf(stderr, "   -h               Create a header file for object name\n");
  fprintf(stderr, "   -c               Turn off hierarchy collapsing\n");
  fprintf(stderr, "   -t <texpage.tex> Texture page file name\n");
  fprintf(stderr, "   -a <anim.bsf>    Animation file name.\n");
  fprintf(stderr, "   -b <geom.bsf>    Mercury geometry file name.\n");
  fprintf(stderr, "Example: gmerc -t rush.tex -a rush.anim.bsf -b rush.geom.bsf rush.sdf rush.anim\n" );

  exit( 0 );
}

/****
 *
 * This program inputs an ASCII SDF file and generates a binary one
 *
 * Usage:
 *  bsdf [ -c -x -G ] <file1> <file2> ...
 *
 *  <file>  Name of an ASCII SDF file (not including .sdf)
 *
 * -c		Produce compiled surfaces
 * -x		Load Framework extensions
 * -G		Save TriMesh for GL rendering
 *
 * This program requires <file>.sdf to be an ASCII SDF file somewhere
 * on the search path. It outputs either <file>.bsf or <file>.csf
 * depending on whether or not compiled surfaces are requested
 *
 ****/
int main(int argc, char **argv)
{
  /* SDF*		sdf = NULL; */
  uint32		compile = SDF_CompileSurfaces | SDF_SnakePrimitives;
  uint32		animflag = 0;
  char		infile[100];
  char		animfile[100];
  char		tpfile[100];
  char		outfile[100];
  char		outanimfile[100];
  char		compchar = 'n';
  char		mpw = 0, opt;
  char		*cp;
  int 		ac = argc, i;
  char 		**av = argv;
  char 		*bin_file = NULL;
  char		headerfile[100];
  char 		*tokens;
  Err 		err = NO_ERROR;

  if (argc == 0)
    {
      printf( "Enter input file : " );
      scanf( "%s", infile );
      scanf( "%c", &compchar );
	
      printf( "Enter texture page file : " );
      scanf( "%s", tpfile );
      scanf( "%c", &compchar );

      printf( "Compile anim file y/n : " );
      scanf( "%c", &compchar );
      animfile[0] = 0;
      if(  compchar == 'y' )
	{
	  printf( "Enter anim file : " );
	  scanf( "%s", animfile );
	  scanf( "%c", &compchar );
	}
#if 0
      printf( "Compile anim file y/n : " );
      scanf( "%c", &compchar );
      if(  compchar == 'y' )
	{
	  printf( "Enter anim file : " );
	  scanf( "%s", animfile );
	  scanf( "%c", &compchar );
	  animflag = 1;
	}
	
      printf( "Enter texture page file : " );
      scanf( "%s", tpfile );
      scanf( "%c", &compchar );

      printf( "Enter anim output file name : " );
      scanf( "%s", outanimfile );
      scanf( "%c", &compchar );
#endif
		
#if 0
      printf( "Compile surfaces y/n : " );
      scanf( "%c", &compchar );
      if(  compchar == 'y' ) compile = SDF_CompileSurfaces | SDF_SnakePrimitives;
#endif
      compile = SDF_CompileSurfaces | SDF_SnakePrimitives;
		
      printf( "Enter output file : " );
      scanf( "%s", outfile );
    }
  else
    {
      mpw = 1;
      if ( argc < 2 ) Usage();
	
      /* default now is to optimize and snakify */	
	
      cp = *++av; /* point to first argument */
      while( ac >= 2 && *cp++ == '-' )
	{
	  opt = *cp++;
	  switch( opt )
	    {
	    case 's':   /* write out framework chunk */
	      SetFacetSurfaceFlag(1);
	      break;
	    case 'f':   /* write out framework chunk */
	      SetFrameworkFlag(1);
	      break;
	    case 'h':   /* write the anim file */
	      strcpy(headerfile, *++av);
	      OpenHeaderFile(headerfile);
	      ac--;
	      break;
	    case 'a':   /* write the anim file */
	      strcpy(outanimfile, *++av);
	      animflag = 1;
	      ac--;
	      break;
	    case 'b':   /* write a binary file */
	      strcpy(outfile, *++av);
	      ac--;
	      break;
	    case 't':   /* reading texture page name */
	      strcpy(tpfile, *++av);
	      ac--;
	      SetTexPage(1);
	      break;
	    case 'm': 
	      SetMessageFlag(1);
	      break;
	    case 'c':   /* collapse hierarchy */
	      SetCollapseFlag(0);
	      break;
	    default: Usage();
	    }
	  ac--; /* force one less argument */
	  cp = *++av; /* next argument */
	}
	
      /* make sure that the atleast one argument is left for input file name */
      if ( ac <= 1 ) Usage();
    }

  /* Gfx_Init(); */

	/* sdf = SDF_Open(NULL); */
	/* assert(sdf); */
	
  if (mpw)
    {
#ifdef NEW_PARSER
      Glib_Init();
      Symbol_Init();
      err = InitParser(&File);
      if (err < 0)
	{
	  DeleteParser();
	  exit(err);
	}
      for( i = ( argc - ( ac - 1 ) ); i < argc; i++ ) /* keep merging */
	{
	  err = StartParser(argv[ i ], &File, &tokens);
	  if (err < 0)
	    {
	      DeleteParser();
	      printf("bsdf: cannot parse %s\n", argv[ i ]);
	      exit(err);
	    }
	  /*
	    if (SDF_Parse(sdf, argv[ i ]) != GFX_OK)
	    {
	    printf("bsdf: cannot parse %s\n", infile);
	    exit(0);
	    }
	    */
	}
      WriteBlockSize();
      if (err == NO_MORE_DATA)
	{
	  InterpretSDFTokens(tokens);
	}
      /* DeleteParser(); */
      Symbol_Delete();
      if ((HasTexPage()) && (!TexPage_Parse(tpfile)))
	{
	  printf("bsdf: cannot parse %s\n", tpfile);
	  exit(0);
	}
#endif
    }
  else
    {
#ifdef NEW_PARSER

      Glib_Init();
      Symbol_Init();

      err = InitParser(&File);
      if (err < 0)
	{
	  DeleteParser();
	  exit(err);
	}
      err = StartParser(infile, &File, &tokens);

      if( animfile[0] )
	err = StartParser(animfile, &File, &tokens);

      WriteBlockSize();
      if (err == NO_MORE_DATA)
	{
	  InterpretSDFTokens(tokens);
	}
			
      DeleteParser();
		
      Symbol_Delete();

      if (!TexPage_Parse(tpfile))
	{
	  printf("bsdf: cannot parse %s\n", tpfile);
	  exit(0);
	}

      /* SetMessageFlag(1); */
      /* Process_Geometry(); */

      SetTexPage(1);

      if (animflag)
	{
	  SetAnimFlag(1);
	  ResetMercid();
	  SDFB_WriteIFF( outfile, compile );
	  SetAnimFlag(2);
	  ResetMercid();
	  SDFB_WriteIFF( outanimfile, compile );
	}
      else
	SDFB_WriteIFF( outfile, compile );
		
      Glib_Delete();
      if (HasTexPage())
	DeleteTexTable();
      CloseHeaderFile();
      printf("Done\n");
      exit(0);
#endif
#ifndef NEW_PARSER
      if (SDF_Parse(sdf, infile) != GFX_OK)
	{
	  printf("bsdf: cannot parse %s\n", infile);
	  exit(0);
	}
      if (animflag)
	{
	  if (SDF_Parse(sdf, animfile) != GFX_OK)
	    {
	      printf("bsdf: cannot parse %s\n", animfile);
	      exit(0);
	    }
	}
	
      if (!TexPage_Parse(tpfile))
	{
	  printf("bsdf: cannot parse %s\n", tpfile);
	  exit(0);
	}
#endif
    }
	
  /*
 * Write binary SDF file 
 */
  if (animflag)
    {
      printf("Writing geometry file to : %s\n", outfile);
      printf("Writing animation file to : %s\n", outanimfile);
    }
  else
    printf("Writing binary SDF file to : %s\n", outfile);
 	
#if 0
  SDF_WriteBinary( sdf, outfile, compile );
#else
  if (animflag)
    {
      SetAnimFlag(1);
      ResetMercid();
      SDFB_WriteIFF( outfile, compile );
      SetAnimFlag(2);
      ResetMercid();
      SDFB_WriteIFF( outanimfile, compile );
    }
  else
    SDFB_WriteIFF( outfile, compile );
#endif

  /* clean up the texture page table */
  Glib_Delete();
  if (HasTexPage())
    DeleteTexTable();
  CloseHeaderFile();
  printf("Done\n");
	
  return(0);
}
