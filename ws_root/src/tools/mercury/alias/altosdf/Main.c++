
/*
**	File:		Main.c++	
**
**	Contains:	Main function Alias v6.0 wire to M2 SDF file converter
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1994 by The 3DO Company. All rights reserved.
**	          	This material constitutes confidential and proprietary
**	          	information of the 3DO Company and shall not be used by
**	          	any Person or for any purpose except as expressly
**	          	authorized in writing by the 3DO Company.
**
**	Change History (most recent first):
**
**	To Do:
*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <AlUniverse.h>
#include <AlGroupNode.h>
#include <PrintSDF.h>
#include <AlTesselate.h>

void
PrintUsage( char *nme )
{
	fprintf( stderr, "Usage: %s "
		"[-r] [-l] [-m] [-t polytype] [-p prefix] [-q tessquality] [-sa scriptfile] [-sn scriptfile] [-o outfile] <infile>\n", 
		nme );

	fprintf( stderr, "Description:\n"
		"  Version 1.0 a5\n"
		"  This program creates a SDF data out of a wire file\n"
		"  -c               write out vertex color[default  = OFF]\n"
		"  -r               do not write texture references[ default write textures ]\n"
		"  -l               do not write lighting information [default = ON]\n"
		"  -m               separate materials and textures in different files [default = OFF]\n"
		"  -t polytype      quads for quadrilateral polygons [ default triangles ]\n"
		"  -p prefix        concatenate the 'prefix' to object names[ default none ]\n"
		"  -q tessquality   0.0 to 1.0 for coarse to fine quality [ default 0.5 ]\n"
		"  -sa scriptfile   append to a texture conversion script file [ default none ]\n"
		"  -sn scriptfile   output to a texture conversion script file [ default none ]\n"
		"  -o outfile       output SDF file name[ default stdout ]\n"
		"  infile           input Alias wire file name\n"
		);
	exit( 0 );
}

main( int argc, char** argv )
{
	char temp[80], sdffile[ 80 ];
	char *cp, opt;
	int sdfON = 0, texOFF = 0;
	const char *prefixStr = NULL;
	char		*infile;
	FILE		*outfp = stdout;
	char		*str;
	double		qual;
	int ac = argc;
	char **av = argv;
	char *progName = argv[0];
	char has_anim = 0;
	
	if ( argc < 2 )		PrintUsage( progName );
	cp= *++av; /* point to first argument */
    while(argc >= 2 && *cp++ == '-')
    {
        opt= *cp++;
        switch(tolower(opt))
        {
                case 't':  {
						str = *++av; ac--;
           				if ( !strcmp( str, "quads" ) )
                			setTessPolyType( kTESSELATE_QUADRILATERAL );
						}
                        break;
                case 'q': {
						str = *++av; ac--;
                        qual = atof( str );
						setTessQuality( qual );
						}
                        break;
                case 'p': {
						str = *++av; ac--;
                        SDF_SetPrefix( str ); 
						}
                        break;
                case 'c': {
                        SDF_SetColorFlag( 1 ); 
						}
                        break;
                case 'l': {
                        SDF_SetLightInfo( 0 ); 
						}
                        break;
                case 'm': {
                        SDF_SeparateLightMaterialInfo( 1 ); 
						}
                        break;
                case 'a': {
                        has_anim = 1; 
                        SDF_HasAnim( 1 ); 
						}
                        break;
                case 'r': {
                        SDF_SetTexRefOutput( 0 ); 
						}
                        break;
                case 'sa': { 
						str = *++av; ac--;
						outfp = fopen( str, "a" );
						if ( outfp == NULL )
							fprintf( stderr, "ERROR: Error opening script file '%s'.\n", str );
						else setScriptFile( outfp );
						}
                        break;
                case 'sn': { 
						str = *++av; ac--;
						outfp = fopen( str, "w" );
						if ( outfp == NULL )
							fprintf( stderr, "ERROR: Error opening script file '%s'.\n", str );
						else setScriptFile( outfp );
						}
                        break;
                case 'o': { 
						str = *++av; ac--;
						outfp = fopen( str, "w" );
						if ( outfp == NULL )
							fprintf( stderr, "ERROR: Error opening output file '%s'.\n", str );
						else setPrintFile( outfp );
						}
                        break;
                default: PrintUsage( progName );
        }
        ac--; /* force one less argument */
		// make sure that the atleast one argument is left for input file name
		if ( ac <= 1 ) PrintUsage( progName );

        cp= *++av; /* next argument */
    }

	infile = --cp;
	fprintf( stderr, "Converting  Alias file: %s\n", infile );

	AlUniverse::initialize( kYUp );

	if( sSuccess != AlUniverse::retrieve( infile ) ) {
		fprintf( stderr, "ERROR: Error retrieving file '%s'.\n", infile );
		exit( -1 );
	}

	// Start printing from top-level node "WORLD"
	printAlUniverse( infile );
	fclose(outfp);

	if (has_anim)
		printAlUniverseAnim( infile );

	exit( 0 );
}
