
/* @(#) decomp3do.c 96/12/11 1.5 */

/**
|||	AUTODOC -class Dev_Commands -name decomp3do
|||	Decompresses a file created by the comp3do(@) tool and creates
|||	an output file which is the decompressed version of the input.
|||
|||	  Synopsis
|||
|||	    decomp3do <input file> <output file>
|||
|||	  Description
|||
|||	    This tool (MPW tool and Portfolio program are available) decompresses
|||	    <input file> (created by the comp3do(@) tool) and writes the result
|||	    to <output file>.
|||
|||	  Arguments
|||
|||	    <inputFile>
|||	        Specifies the file to be decompressed.
|||
|||	    <outputFile>
|||	        Specifies the output file to be created or overwritten. No
|||	        warning is given if the file exists.
|||
|||	  Implementation
|||
|||	    MPW tool and Portfolio program.
|||
|||	  Associated Files
|||
|||	    <:misc:compression.h>, System.m2/Modules/compression
|||
**/

/* This is a simple program that decompresses an input file, and produces an
 * output file which is the decompressed version of the input. The
 * file must have been compressed with comp3do (ie. the first word in
 * the file is the uncompressed data length, the rest of the file is the
 * compressed data)
 *
 * You run the program with two command-line arguments:
 *
 *	decomp3do <input file> <output file>
 *
 * The input file is the file you wish to decompress, and the output file is
 * the resulting decompressed file.
 */

#include <stdio.h>

#ifdef macintosh
  #include <:misc:compression.h>
  #include <cursorctl.h>

  /* don't include files with these declarations, they need still other includes which
     cause conflicts with Portfolio defines... */
  extern int  unlink(char*);
  extern int strcmp(const char *str1, const char *str2);
#else
  #include <misc/compression.h>
#endif	/* macintosh */


/*****************************************************************************/


#define USAGE       "Usage: decomp3do <input file> <output file>\n"
#define BUFFER_SIZE 4096

static uint8 buf[BUFFER_SIZE];
static bool  writeError;
static int   totalBytes;


/*****************************************************************************/


static void PutDecompressedWord(FILE *file, uint32 word)
{
    if (!writeError)
    {
        if (totalBytes < sizeof(uint32))
        {
            if (fwrite(&word,1,totalBytes,file) != totalBytes)
                writeError = TRUE;

            totalBytes = 0;
        }
        else
        {
            if (fwrite(&word,1,sizeof(uint32),file) != sizeof(uint32))
                writeError = TRUE;

            totalBytes -= sizeof(uint32);
        }
    }
}


/*****************************************************************************/


int main(int argc, char **argv)
{
Decompressor *decomp;
FILE         *in;
FILE         *out;
int           numBytes;
int           result;

    if (argc != 3)
    {
        printf("%s",USAGE);
        return (0);
    }

    if ((strcmp("-help",argv[1]) == 0)
     || (strcmp("-?",argv[1]) == 0))
    {
        printf("%s",USAGE);
        return (0);
    }

    result     = 0;
    writeError = FALSE;

    in = fopen(argv[1],"r");
    if (in)
    {
        out = fopen(argv[2],"w");
        if (out)
        {
            fread(&totalBytes,1,sizeof(uint32),in);

#ifdef macintosh
			// spin the cursor, let folks know we are working on it
			InitCursorCtl(NULL);
			SpinCursor(32);
#endif	/* macintosh */

            if (CreateDecompressor(&decomp,(CompFunc)PutDecompressedWord,(TagArg *)out) >= 0)
            {
                while (TRUE)
                {
#ifdef macintosh
					SpinCursor(32);
#endif	/* macintosh */

                    numBytes = fread(buf,1,BUFFER_SIZE,in);
                    if (numBytes < 0)
                    {
                        printf("decomp3do: error reading input file '%s'\n",argv[1]);
                        result = 1;
                        break;
                    }

                    if (numBytes == 0)
                        break;

                    if (numBytes % 4)
                    {
                        printf("decomp3do: corrupted input file '%s'\n",argv[1]);
                        result = 1;
                        break;
                    }

                    FeedDecompressor(decomp,buf,numBytes / 4);

                    if (totalBytes == 0)
                        break;
                }

                DeleteDecompressor(decomp);

                if (!result && totalBytes)
                {
                    printf("decomp3do: corrupted input file '%s'\n",argv[1]);
                    result = 1;
                }

                if (writeError)
                {
                    printf("decomp3do: error writing to output file '%s'\n",argv[2]);
                    result = 1;
                }
            }
            else
            {
                printf("decomp3do: could not create compression engine\n");
                result = 1;
            }

            fclose(out);

            if (result != 0)
                unlink(argv[2]);
        }
        else
        {
            printf("decomp3do: could not open output file '%s'\n",argv[2]);
            result = 1;
        }

        fclose(in);
    }
    else
    {
        printf("decomp3do: could not open input file '%s'\n",argv[1]);
        result = 1;
    }

    return (result);
}
