/*
 *	@(#) futil.c 95/04/11 1.13
 *	Copyright 1994, The 3DO Company
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "kerneltypes.h"
#include "os.h"
#include "fw.h"

#ifdef __Sun__
#define SEEK_END 2
#define SEEK_CUR 1
#define SEEK_SET 0
#endif

static 
int32 goodfile(char *name)
{
	FILE *inf;

	inf = fopen(name, "r");
	if (!inf)
		return 0;
	fclose(inf);
	return 1;
}

int32
glib_file_locate(char *name, char *expname, char *domain)
{
	register char *path;
	register char *start, *finish;
	register char save;
	char template[256];

	/**
	if (name[0] == '/' || name[0] == '.') {
		strcpy(expname, name);
		return;
	}
	**/

	path = (char *) getenv(domain);

	if (goodfile(name)) {
		strcpy(expname, name);
		return 1;
	}

	if (!path) {
		if (goodfile(name)) {
			strcpy(expname, name);
			return 1;
		}
	}
	start = path;
	finish = path;
	while (1) {
		if (*path == 0 || *path == ':') {
			if (start != finish) {
				save = *finish;
				*finish = 0;
				strcpy(template, start);
				*finish = save;
				strcat(template, "/");
				strcat(template, name);
				if (goodfile(template)) {
					strcpy(expname, template);
					return 1;
				}
			}
			if (*path == 0) {
				if (goodfile(name)) {
					strcpy(expname, name);
					return 1;
				}
				else {
					return 0;
				}
			}
			path++;
			start = path;
			finish = path;
		} else {
			path++;
			finish = path;
		}
	}
}


FILE *
K9_OpenFile(char *fileName, int32 mode)
{
	FILE    *file = NULL;
  /*	char expname[256]; */

    /* check argument */

    if (fileName == NULL || *fileName == '\0')
    	return NULL;

	if ( mode == Stream_Read ) {
    /*		glib_file_locate(fileName, expname, GLIB_FILE_PATH); */
		file = fopen(fileName, "r");
	} else {
		file = fopen(fileName, "w+");
	}

    if (file == NULL) {
    	fprintf(stderr, "openFile: Could not open file \"%s\"\n", fileName);
    	return NULL;
    }

	return file;
}

ByteStream *
K9_OpenByteStream(char *fileName, int32 mode, int32 bSize)
{
	int32 size = bSize;
    FILE    *file = NULL;
	ByteStream *theStream;


	if ( file = K9_OpenFile(fileName, mode) ) {
		theStream = (ByteStream *)malloc(sizeof(ByteStream));
		if (!theStream) {
			fclose(file);
			return NULL;
		}
		theStream->stream = file;
		theStream->gotChar = FALSE;
    	return theStream;
	} else return NULL;
}

void
K9_ExpandFileName(char *name, char *expname)
{
  /*	glib_file_locate(name, expname, GLIB_FILE_PATH); */
  strcpy(expname, name);
}

static seek_types[] = {
						SEEK_SET,
						SEEK_CUR,
						SEEK_END
};

int32 
K9_SeekByteStream(ByteStream *theStream, int32 offset, int32 whence)
{
	return fseek((FILE *)theStream->stream, offset, seek_types[whence]);
}

void 
K9_CloseByteStream(ByteStream *theStream)
{
	fclose((FILE *)theStream->stream);
}

int32
K9_ReadByteStream(ByteStream *theStream, char *buffer, int32 nBytes)
{
	return fread(buffer, sizeof(char), nBytes, (FILE *)theStream->stream);
} 

int32
K9_WriteByteStream(ByteStream *theStream, char *buffer, int32 nBytes)
{
    return fwrite(buffer, sizeof(char), nBytes, (FILE *)theStream->stream);
}

int32
K9_GetChar(ByteStream *theStream)
{
    char c ;
    int32 result;

    if ( theStream->gotChar ) {
        theStream->gotChar = FALSE;
        return theStream->savedChar;
    }

    result = K9_ReadByteStream(theStream, &c, 1);

    if ( result != 1 )
        return EOF;

    return c;

}

void
K9_UngetChar(ByteStream *theStream, int32 c)
{
    theStream->gotChar = TRUE;
    theStream->savedChar = c;
}

bool 
K9_EndOfStream(ByteStream *theStream)
{
	return feof((FILE *)(theStream->stream));
}

int32 K9_GetByteStreamLength(ByteStream *theStream)
{
  /*  struct stat stbuf; */
  long  cur, end, result;
  FILE *fPtr;
  
  fPtr = (FILE *)theStream->stream;
  /*  fstat(fileno((FILE *)(theStream->stream)), &stbuf); */
  
  cur = ftell(fPtr);
  result = fseek(fPtr, 0, SEEK_END);
  end = ftell(fPtr);
  result = fseek(fPtr, cur, SEEK_SET);
  return(end);
}


bool
K9_ErrorOnStream(ByteStream *theStream)
{
    return ferror((FILE *)(theStream->stream));
}
