/* @(#) fsinfo.c 96/07/02 1.8 */

/**
|||	AUTODOC -class Shell_Commands -name FSInfo
|||	Displays information on mounted file systems.
|||
|||	  Format
|||
|||	    FSInfo
|||
|||	  Description
|||
|||	    This command displays information about mounted file
|||	    system. The information includes the raw device the file
|||	    system is running on, the FS creation time, the FS size,
|||	    and its status. The status specified whether the FS is
|||	    read-write or read-only, whether it is online, and whether it
|||	    is intended for end-user storage.
|||
|||	  Implementation
|||
|||	    Command implemented in V27.
|||
|||	  Location
|||
|||	    System.m2/Programs/FSInfo
|||
**/

#include <kernel/types.h>
#include <kernel/operror.h>
#include <file/filefunctions.h>
#include <file/filesystem.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <international/intl.h>
#include <misc/date.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/****************************************************************************/


#define Error(x,err) {printf x; PrintfSysErr(err);}


int32 main(void)
{
Directory      *dir;
DirectoryEntry  de;
Item            file;
Item            ioreq;
IOInfo          ioInfo;
FileSystemStat  fsStat;
Err             err;
char            state[30];
unichar         unidate[30];
char            date[30];
bool            intlOpened;
char            fsName[FILESYSTEM_MAX_NAME_LEN+2];
Item            locale;
GregorianDate   gd;

    intlOpened = FALSE;
    locale     = -1;

    if (intlOpenFolio() >= 0)
    {
        locale = intlOpenLocale(NULL);
        if (locale >= 0)
            intlOpened = TRUE;
        else
            intlCloseFolio();
    }

    dir = OpenDirectoryPath("/");
    if (dir)
    {
        printf("Device            Creation    Bytes     Total      Free  Used  State       Mount\n");
        printf("& Item            Date        /Blck    Blocks    Blocks                    Name\n");
        printf("-----------------------------------------------------------------------------------\n");

        while (ReadDirectory(dir,&de) >= 0)
        {
            sprintf(fsName,"/%s",de.de_FileName);
            file = OpenFile(fsName);
            if (file >= 0)
            {
                ioreq = CreateIOReq(NULL,0,file,0);
                if (ioreq >= 0)
                {
                    /* set all fields to 0, for safety */
                    memset(&fsStat,0,sizeof(fsStat));

                    memset(&ioInfo,0,sizeof(IOInfo));
                    ioInfo.ioi_Command         = FILECMD_FSSTAT;
                    ioInfo.ioi_Recv.iob_Buffer = &fsStat;
                    ioInfo.ioi_Recv.iob_Len    = sizeof(fsStat);
                    err = DoIO(ioreq,&ioInfo);
                    if (err >= 0)
                    {
                        if (de.de_Flags & FILE_IS_READONLY)
                            strcpy(state,"RO");
                        else
                            strcpy(state,"RW");

                        if (de.de_Flags & FILE_USER_STORAGE_PLACE)
                            strcat(state,"/User");

                        err = -1;
                        if (fsStat.fst_BitMap & FSSTAT_CREATETIME)
                        {
                            err = OpenDateFolio();
                            if (err >= 0)
                            {
                                ConvertTimeValToGregorian((TimeVal *)&fsStat.fst_CreateTime, &gd);
                                CloseDateFolio();

                                err = intlFormatDate(locale, intlLookupLocale(locale)->loc_ShortDate,
                                                     &gd, unidate, sizeof(unidate));
                                if (err >= 0)
                                    err = intlTransliterateString(unidate, INTL_CS_UNICODE,
                                                                  date, INTL_CS_ASCII,
                                                                  sizeof(date), ' ');
                            }
                        }

                        if (err < 0)
                        {
                            strcpy(date,"----------");
                        }

                        printf("%-7.7s #%-08X %10s %6u %9u %9u %4u%%  %-10s  %s\n",
                               fsStat.fst_RawDeviceName,
                               fsStat.fst_RawDeviceItem,
                               date,
                               fsStat.fst_BlockSize,
                               fsStat.fst_Size,
                               fsStat.fst_Free,
                               (fsStat.fst_Used*100) / fsStat.fst_Size,
                               state,
                               fsStat.fst_MountName);
                    }
                    else
                    {
                        Error(("Unable to interrogate %s: ",fsName),err);
                    }
                    DeleteIOReq(ioreq);
                }
                else
                {
                    Error(("Unable to interrogate %s: ",fsName),ioreq);
                }
                CloseFile(file);
            }
            else
            {
                Error(("Unable to access %s: ",fsName),file);
            }
        }
        CloseDirectory(dir);
    }
    else
    {
        Error(("Unable to scan mounted file systems: "), NOMEM);
    }

    if (intlOpened)
    {
        intlCloseLocale(locale);
        intlCloseFolio();
    }

    return 0;
}
