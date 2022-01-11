/* @(#) comp3do.c 96/12/11 1.5 */

/**
|||	AUTODOC -class Dev_Commands -name comp3do
|||	Compresses an input file and creates a file which can be decompressed with the
|||	decomp3do(@) tool or with Portfolio's compression folio.
|||
|||	  Synopsis
|||
|||	    comp3do <input file> <output file>
|||
|||	  Description
|||
|||	    This tool (MPW tool and Portfolio program are available) compresses
|||	    <input file> and writes the result to <output file>.  The first longword
|||	    of the compressed file <output file> is the length of the UNCOMPRESSED
|||	    data.  The file can be decompressed with the decomp3do(@) tool by simply calling
|||
|||	        decomp3do(@) <compressed file> <output file>
|||
|||	    The file can also be decompressed by Portfolio's compression folio, but DO NOT
|||	    feed the decompressor the length word as part of the stream of compressed data.
|||	    For example (in pseudo code, error checking omitted)
|||
|||	        uint32 *compData = openandloadfile("compressed file");
|||	        void   *decompBuff = AllocMem(@)(compData[0], MEMTYPE_NORMAL);
|||	        err = SimpleDecompress(@)(&compData[1], compData[0], decompBuff, compData[0]);
|||
|||	  Arguments
|||
|||	    <inputFile>
|||	        Specifies the file to be compressed.
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

/* This is a simple program that compresses an input file, and produces an
 * output file which is the compressed version of the input. The file can
 * be decompressed using the decomp3do program.
 *
 *  NOTE: the first longword of the compressed file will be the length of the
 *        uncompressed data.  If this file is to be loaded and uncompressed
 *        by the compression folio on the 3DO, DO NOT feed the decompressor
 *        the length word.  For example (in pseudo code)
 *
 *        uint32 *compData = openandloadfile();
 *        uncompData = decompressdata(&compData[1]);
 *
 * You run the program with two command-line arguments:
 *
 *	comp3do <input file> <output file>
 *
 * The input file is the file you wish to compress, and the output file is
 * the resulting compressed file.
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


#define USAGE       "Usage: comp3do <input file> <output file>\n"
#define BUFFER_SIZE 4096

static bool  writeError;
static char  buf[BUFFER_SIZE];


/*****************************************************************************/


static void PutCompressedWord(FILE *file, uint32 word)
{
    if (!writeError)
        if (fwrite(&word,1,sizeof(uint32),file) != sizeof(uint32))
            writeError = TRUE;
}


/*****************************************************************************/


int main(int argc, char **argv)
{
uint32        i;
Compressor   *compr;
FILE         *in;
FILE         *out;
int           skip;
int           numBytes;
int           totalBytes;
int           result;

    if (argc != 3)
    {
        printf("%s",USAGE);
        return (-1);
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
            totalBytes = 0;
            fwrite(&totalBytes,1,sizeof(uint32),out);  /* place holder */

#ifdef macintosh
		// spin the cursor, let folks know we are working on it
		InitCursorCtl(NULL);
		SpinCursor(32);
#endif	/* macintosh */

            if (CreateCompressor(&compr,(CompFunc)PutCompressedWord, (TagArg *)out) >= 0)
            {
                skip = 0;
                while (TRUE)
                {

#ifdef macintosh
			SpinCursor(32);
#endif	/* macintosh */

                    numBytes = fread(&buf[skip],1,BUFFER_SIZE-skip,in);

                    if (numBytes < 0)
                    {
                        printf("comp3do: error reading from input file '%s'\n",argv[1]);
                        result = 1;
                        break;
                    }

                    if (numBytes == 0)
                    {
                        if (skip)
                        {
                            for (i = sizeof(uint32) - 1; i >= skip; i--)
                                buf[i] = 0;

                            FeedCompressor(compr,buf,1);
                        }
                        break;
                    }

                    totalBytes += numBytes;
                    numBytes   += skip;
                    skip        = numBytes % sizeof(uint32);

                    FeedCompressor(compr,buf,numBytes / sizeof(uint32));

                    for (i = 0; i < skip; i++)
                        buf[i] = buf[numBytes - skip + i];
                }

                DeleteCompressor(compr);

                if (!writeError)
                {
                    rewind(out);
                    fwrite(&totalBytes,1,sizeof(uint32),out);
                }
                else
                {
                    printf("comp3do: error writing to output file '%s'\n",argv[2]);
                    result = 1;
                }
            }
            else
            {
                printf("comp3do: could not create compression engine\n");
                result = 1;
            }

            fclose(out);

            if (result != 0)
                unlink(argv[2]);
        }
        else
        {
            printf("comp3do: could not open output file '%s'\n",argv[2]);
            result = 1;
        }

        fclose(in);
    }
    else
    {
        printf("comp3do: could not open input file '%s'\n",argv[1]);
        result = 1;
    }

    return (result);
}
