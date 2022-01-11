#include "TlA3dsfile.h"
#include "TlMaterial.h"
#include "TlTexture.h"
#include <stdio.h>

void main()
{
	char infile[ 80 ], outfile[ 80 ], prefix[80];
	char texON = 'y';
			
	cout << "Enter 3DS filename for input" << endl;
	cin >> infile;
	cout << "Enter SDF filename for output" << endl;
	cin >> outfile;
	cout << "Do you want texture references ( y/n )" << endl;
	cin >> texON;

	if ( ( strlen( infile ) > 0) && (strlen( outfile ) > 0) )
	{	
		// Read 3DS file		
		A3dsfile file3DS( infile );
		if ( texON == 'n' ) 
		{
			cout << "Texture references OFF" << endl;
			file3DS.SetTexRefOutput( FALSE );
		}	
	
		TlGroup grp("World");
		file3DS.ReadModel( &grp );
							
		// Write the SDF file
		ofstream of( outfile );
		
		of << "SDFVersion 0.1" << endl;
		of << "Units INCHES" << endl << endl;
		
		grp.SetRefID(-1);					// Set unique reference indices
		file3DS.PrintDefineAttributes( of, outfile, FALSE, NULL, 0 ); // Write texture and material defines
		grp.WriteSDF1( of );				// Write all unique objects
		grp.WriteSDF( of );					// Write hierarchy information
		//file3DS.PrintKFObjects( cout );	// Write an .nasl file
		file3DS.SDF_WriteKfData( cout );    // Write an .anim file
	}
}

