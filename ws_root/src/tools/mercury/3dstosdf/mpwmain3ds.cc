/*
**	File:		mpwmain3ds.cxx
**
**	Contains:	 
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.
**
**	Change History (most recent first):
**
**		 <5>	 7/10/95	RRR		fix usage indentation problem
**
**	To Do:
*/

#include "TlA3dsfile.h"
#include "TlMaterial.h"
#include "TlTexture.h"
#include <stdio.h>
#include <ctype.h>
#include <fstream.h>
#include <string.h>

/*
** Extract last word of the file name on Mac and UNIX
*/
const char
*SDF_ExtractFileName( const char *in )
{
    static char out_file[ 80 ];
    int i = 0, i1 = 0, i2 = 0;

    // locate last unix slash character
    while ( in[i] != '\0' )
    {
        if ( ( in[i] == '/' ) || ( in[i] == ':' ) ) i1 = i + 1;
        i++;
    }
    i = i2 = i1;
    // locate last dot character
    while ( in[i] != '\0' )
    {
        if ( in[i] == '.' ) i2 = i;
        i++;
    }

    if (( i1 == i2 ) && (in[i-1] == '.'))
    	i2 = i - 1;
    else if (( i1 == i2 ) && (in[i-1] != '.')) 
    	i2 = i;

    i = i1;
    while ( i < i2 )
    {
        out_file[ i - i1 ] = in[ i ];
        i++;
    }
    out_file[ i - i1 ] = '\0';
    // fprintf( stderr, "%d, %d, %d = %s - %s\n", i1, i2, i, in, out_file );

    return out_file;
}

void
Usage( char *nme )
{
    fprintf( stderr, "Usage: %s [-a -t -m -sn/sa <script_file> -p prefix -o <sdf_outfile>] <3ds_infile>\n", nme );
	fprintf( stderr, "Description:\n" );
	fprintf( stderr, "  Version 3.1 \n" );
	fprintf( stderr, "  This program creates a SDF data out of a 3DS file\n" );

    fprintf( stderr, "  -a                  write out keyframe animation file\n");
    fprintf( stderr, "  -t                  do not write texture references\n");
    fprintf( stderr, "  -l                  do not write light references\n");
    fprintf( stderr, "  -c                  disable texture seam stitching\n");
    fprintf( stderr, "  -g                  enable geometry splitting based on smoothing groups\n");
    fprintf( stderr, "  -m                  write texture and material data into separate files\n");
    fprintf( stderr, "                      writes \"sdf_outfile.tex\" and \"sdf_outfile.mat\" files\n");
    fprintf( stderr, "  -sn <script_file>   write texture references into a new script file\n");
    fprintf( stderr, "  -sa <script_file>   append texture references into an existing script file\n");
    fprintf( stderr, "  -p prefix           add a prefix string to all the object names\n");
    fprintf( stderr, "  -o <sdf_outfile>    write out SDF file with given name\n");
    fprintf( stderr, "                      default SDF file name is 3ds_infile.sdf\n");
    fprintf( stderr, "  <3ds_infile>        3DS file for input\n");
    exit(1);
}

int
main( int argc, char **argv )
{
	const char *tmp;
	char temp[80], sdffile[ 80 ], animfile[ 80 ];
	char top_node_name[80];
	ErrCode readErr;
	int animData = 0;
	char *cp, opt;
	int sdfON = 0, texOFF = 0, lgtOFF = 0;
	int seamOFF = 0, sgroupsON = 0;
	const char *prefixStr = NULL;
	char *scrFile = NULL;
	int scrType = 0; /* 0 - no script file, 1 - new, 2 - append */
	int ac = argc;
	char **av = argv;
	char *progName = argv[0];
	Boolean attFile = FALSE;
	
	if ( argc < 2 )		Usage( progName );
	cp= *++av; /* point to first argument */
    while(ac >= 2 && *cp++ == '-')
    {
        opt= *cp++;
        switch(tolower(opt))
        {
                case 'a': { /* write Animation file */
                        animData = 1;
						};
                        break;
                case 't': { /* do not write textures */
                        texOFF = 1;
						};
                case 'l': { /* do not write lighting info */
                        lgtOFF = 1;
						};
                        break;
                 case 'c': { /* do not stitch texture seams info */
                        seamOFF = 1;
						};
                        break;
                 case 'g': { /* split geometry based on smoothing groups */
                        sgroupsON = 1;
						};
                        break;
                case 'p': { /* add prefix to the object names */
                        prefixStr = *++av; ac--;
						};
                        break;
                case 's': { /* flag to write script file */
                        scrFile = *++av; ac--;
                        opt= tolower(*cp++);
                        if ( opt == 'n' ) scrType = 1;
                        else if( opt == 'a' ) scrType = 2;
						};
                        break;
                case 'm': { /* write separate texture and material files */
                        attFile = TRUE;
						};
                        break;
                case 'o':  { /* output SDF file name */
						sdfON = 1;
						ac--;
						if ( ac <= 2 ) Usage( progName );
						tmp = ChopString( *++av, '.' );
						sprintf( temp, "%s", tmp );
						sprintf( sdffile, "%s", *av );
						};
                        break;
                default: Usage( progName);
        }
        ac--; /* force one less argument */
        // make sure that the atleast one argument is left for input file name
        if ( ac <= 1 ) Usage( progName );

        cp= *++av; /* next argument */
    }
	--cp;
	fprintf( stderr, "Converting 3DS file: %s\n", cp );
	
	//  Read 3DS file 
	A3dsfile file3DS( cp );

	/* create the output file name if it is not specified */
	if ( sdfON == 0 )
	{
		tmp = ChopString( cp, '.' );
		sprintf( temp, "%s", tmp );
		sprintf( sdffile, "%s.sdf", temp );
	}
	
	// Set the current filename to use for material, texture group name
	SetFileName3DS( SDF_ExtractFileName( sdffile ) );
	
	// Set the prefix for objects
	if ( prefixStr ) 
	{
		file3DS.SetPrefix( prefixStr );
		sprintf( top_node_name, "%sworld", prefixStr );
	} else sprintf( top_node_name, "%s_world", GetFileName3DS() );
	
	// turn off texture references
	if ( texOFF ) file3DS.SetTexRefOutput( FALSE );

	// turn off light references
	if ( lgtOFF ) file3DS.SetLightsOutput( FALSE );

	// turn on smoothing groups
	if ( sgroupsON ) file3DS.SetSmootGroupsOutput( TRUE );

	// turn off texture seam stitching
	if ( seamOFF ) file3DS.SetTexStitchOutput( FALSE );

	TlGroup grp( top_node_name );
	
	readErr = file3DS.ReadModel( &grp );
	
	if ( readErr == noError ) 
	{
		// Write the SDF file
		ofstream of( sdffile );

		PRINT_ERROR( of.fail(), "Unable to create SDF file for write\n" );
	
		of << "SDFVersion 0.1" << '\n';
		of << "Units INCHES" << '\n' << '\n';
		
		grp.SetRefID(-1);					// Set unique reference indices
		file3DS.PrintDefineAttributes( of,
		                   sdffile, attFile,
		                   scrFile, scrType ); // Write texture and material defines
		grp.WriteSDF1( of );				// Write all unique objects
		grp.WriteSDF( of );					// Write hierarchy information

		if ( animData )
		{	
			sprintf( animfile, "%s.ani", temp );
			fprintf( stderr, "Writing animation file: %s\n", animfile );
			// Write an animation file
			ofstream ofa( animfile );
			PRINT_ERROR( ofa.fail(), "Unable to create Animation file for write\n" );

			ofa << "# KeyFrame animation file" << endl;
			//file3DS.PrintKFObjects( ofa );
			file3DS.SDF_WriteKfData( ofa );
		}
	} else if ( readErr == ioError ) 
		cout << cp << " is not a 3D Studio file" << endl;
	
	return 0;	
}
