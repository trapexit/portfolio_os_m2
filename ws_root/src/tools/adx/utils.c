/* @(#) utils.c 95/09/22 1.1 */

#include <ctype.h>

int strcasecmp(const char *a,const char *b)
{
    for (;;)
    {	char c1 = *a++, c2 = *b++;
	int d = toupper((int)c1) - toupper((int)c2);
	if (d != 0) return d;
	if (c1 == 0) return 0;	   /* no need to check c2 */
    }
}


int strncasecmp(const char *a,const char *b, int n)
{
    while (n-- > 0)
    {	char c1 = *a++, c2 = *b++;
	int d = toupper((int)c1) - toupper((int)c2);
	if (d != 0) return d;
	if (c1 == 0) return 0;	   /* no need to check c2 */
    }
    return 0;
}


#ifdef macintosh


#define OLDROUTINELOCATIONS 0
#define OLDROUTINENAMES 0

#include <types.h>
#include <Errors.h>
#include <Files.h>
#include <TextUtils.h>
#include <ErrMgr.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

extern int errno;

int mkdir(const char* path)
{
	OSErr result = noErr;
	long newDirID = 0;

	unsigned char localPath[256];
	strcpy((char*)localPath, path);
	c2pstr((char*)localPath);
    result = DirCreate(0, 0, localPath, &newDirID);
	if (result == dupFNErr)
		result = noErr;
	if (result != noErr)
	{
		errno = result;
		printf("Error creating '%s'\n", path);
		printf("%s\n", GetSysErrText(result, (char*)localPath));
	}
	return (int)result;
}

#else

#include <stdio.h>

int mkdir(const char *dirName)
{
char temp[300];

   sprintf(temp,"mkdir -p %s",dirName);
   system(temp);
   return 0;
}

#endif /* macintosh */
