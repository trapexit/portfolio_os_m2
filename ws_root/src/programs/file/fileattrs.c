/* @(#) fileattrs.c 96/09/10 1.8 */

/**
|||	AUTODOC -class Shell_Commands -name FileAttrs
|||	Gets or sets attributes of a file.
|||
|||	  Format
|||
|||	    FileAttrs [-version <8 bit version>]
|||	              [-revision <8 bit revision>]
|||	              [-type <32 bit file type number>]
|||	              [-date <dd-mmm-yyyy hh:mm:ss>]
|||	              {file}
|||
|||	  Description
|||
|||	    This command lets you set attributes of a file. The supported
|||	    attributes include its version and revision codes, as well as
|||	    its 4 byte file type value.
|||
|||	    If you run the command without specifying any attributes, it will
|||	    display the current attributes of the files.
|||
|||	  Arguments
|||
|||	    -version <8 bit version>
|||	        Specifies the 8 bit version code for the files.
|||
|||	    -revision <8 bit revision>
|||	        Specifies the 8 bit revision code for the files.
|||
|||	    -type
|||	        Specifies the 32 bit file type value to set.
|||
|||	    -date <dd-mmm-yyyy>
|||	        Lets you specify a new modification date for the file.
|||	        The day of the month is specified in numerals, the month is
|||	        specified as a three letter string, and the year is a four
|||	        character numeral. For example: "29-Feb-1996 15:32:47"
|||
|||	    -time <hh:mm:ss>
|||	        Lets you specify a new modification time for the file.
|||
|||	    {files}
|||	        Specifies the names of the files to get or set the attributes
|||	        of. Any number of files or directories can be specified.
|||
|||	  Implementation
|||
|||	    Command implemented in V27.
|||
|||	  Location
|||
|||	    System.m2/Programs/FileAttrs
|||
**/

#include <kernel/types.h>
#include <kernel/operror.h>
#include <file/filefunctions.h>
#include <file/filesystem.h>
#include <international/intl.h>
#include <misc/date.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*****************************************************************************/


static const char *months[] =
{
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec",
    NULL
};

#define SECONDS_PER_DAY (60*60*24)


/****************************************************************************/


#define Error(x,err) {printf x; PrintfSysErr(err);}


int32 main(int32 argc, char *argv[])
{
Err           err;
int           parm;
bool          setVersion;
bool          setRevision;
bool          setFileType;
bool          setDate;
bool          setTime;
int32         version;
int32         revision;
uint32        fileType;
FileStatus    fileStatus;
IOInfo        ioInfo;
Item          file;
Item          ioreq;
bool          first;
char         *dateStr;
char         *timeStr;
Item          locale;
unichar       unidate[30];
unichar       unitime[30];
char          date[30];
char          time[30];
uint32        rem;
TimeVal       dateTV;
TimeVal       timeTV;
GregorianDate gd;
uint32        day, month, year;
char          monthName[10];
uint32        hours, minutes, seconds;

    locale      = intlOpenLocale(NULL);

    setVersion  = FALSE;
    setRevision = FALSE;
    setFileType = FALSE;
    setDate     = FALSE;
    setTime     = FALSE;

    version           = 0;
    revision          = 0;
    fileType          = 0;
    dateStr           = NULL;
    timeStr           = NULL;
    dateTV.tv_Seconds = 0;
    timeTV.tv_Seconds = 0;

    for (parm = 1; parm < argc; parm++)
    {
        if ((strcasecmp("-help",argv[parm]) == 0)
         || (strcasecmp("-?",argv[parm]) == 0))
        {
	    printf("fileattrs - get/set the attributes of a file\n");
            printf("    [-version <8 bit version>]\n");
	    printf("    [-revision <8 bit revision>]\n");
	    printf("    [-type <32 bit file type number>]\n");
            printf("    [-date <dd-mmm-yyyy>]\n");
            printf("    [-time <hh:mm:ss>]\n");
	    printf("    {file}\n");
            return (0);
        }

        if (strcasecmp("-version",argv[parm]) == 0)
        {
            parm++;
            if (parm == argc)
            {
                printf("No version number specified for '-version' option\n");
                return 1;
            }
            version = strtol(argv[parm],NULL,0);
            if ((version < 0) || (version > 255))
            {
                printf("%d is not a valid version number, must be 0..255\n",version);
                return 1;
            }
            setVersion = TRUE;
            argv[parm] = NULL;
        }
        else if (strcasecmp("-revision",argv[parm]) == 0)
        {
            parm++;
            if (parm == argc)
            {
                printf("No revision number specified for '-revision' option\n");
                return 1;
            }
            revision = strtol(argv[parm],NULL,0);
            if ((revision < 0) || (revision > 255))
            {
                printf("%d is not a valid revision number, must be 0..255\n",revision);
                return 1;
            }
            setRevision = TRUE;
            argv[parm] = NULL;
        }
        else if (strcasecmp("-type",argv[parm]) == 0)
        {
            parm++;
            if (parm == argc)
            {
                printf("No file type specified for '-type' option\n");
                return 1;
            }
            fileType = strtol(argv[parm],NULL,0);
            setFileType = TRUE;
            argv[parm] = NULL;
        }
        else if (strcasecmp(argv[parm],"-date") == 0)
        {
            parm++;
            if (parm == argc)
            {
                printf("No date string given for '-date' option\n");
                return 1;
            }
            dateStr = argv[parm];
            setDate = TRUE;
            argv[parm] = NULL;
        }
        else if (strcasecmp(argv[parm],"-time") == 0)
        {
            parm++;
            if (parm == argc)
            {
                printf("No time string given for '-time' option\n");
                return 1;
            }
            timeStr = argv[parm];
            setTime = TRUE;
            argv[parm] = NULL;
        }
    }

    if (setDate || setTime)
    {
        if (timeStr)
        {
            if (sscanf(timeStr, "%u:%u:%u", &hours, &minutes, &seconds) != 3)
            {
                printf("'%s' is not a valid time string\n", timeStr);
                return 1;
            }

            gd.gd_Year   = 1993;
            gd.gd_Month  = 1;
            gd.gd_Day    = 1;
            gd.gd_Hour   = hours;
            gd.gd_Minute = minutes;
            gd.gd_Second = seconds;
            err = ConvertGregorianToTimeVal(&gd, &timeTV);
            if (err < 0)
            {
                printf("'%s' is not a valid time\n", timeStr);
                return 1;
            }
        }

        if (dateStr)
        {
            if (sscanf(dateStr, "%u-%3[a-zA-Z]-%u", &day, monthName, &year) != 3)
            {
                printf("'%s' is not a valid date string\n", dateStr);
                return 1;
            }
            monthName[3] = 0;

            month = 0;
            while (months[month])
            {
                if (strcasecmp(monthName, months[month]) == 0)
                    break;

                month++;
            }

            if (!months[month])
            {
                printf("'%s' is not a valid month name\n", monthName);
                return 1;
            }
            month++;

            gd.gd_Year   = year;
            gd.gd_Month  = month;
            gd.gd_Day    = day;
            gd.gd_Hour   = 0;
            gd.gd_Minute = 0;
            gd.gd_Second = 0;
            err = ConvertGregorianToTimeVal(&gd, &dateTV);
            if (err < 0)
            {
                printf("'%s' is not a valid date\n", dateStr);
                return 1;
            }
        }
    }

    first = TRUE;
    for (parm = 1; parm < argc; parm++)
    {
        if (argv[parm] && (argv[parm][0] != '-'))
        {
            file = err = OpenFile(argv[parm]);
            if (file >= 0)
            {
                ioreq = err = CreateIOReq(NULL,0,file,0);
                if (ioreq >= 0)
                {
                    memset(&ioInfo,0,sizeof(IOInfo));
                    ioInfo.ioi_Command         = CMD_STATUS;
                    ioInfo.ioi_Recv.iob_Buffer = &fileStatus;
                    ioInfo.ioi_Recv.iob_Len    = sizeof(fileStatus);
                    err = DoIO(ioreq,&ioInfo);
                    if (err >= 0)
                    {
                        if (!setVersion
                         && !setRevision
                         && !setFileType
                         && !setDate
                         && !setTime)
                        {
                            ConvertTimeValToGregorian((TimeVal *)&fileStatus.fs_Date, &gd);

                            err = intlFormatDate(locale, intlLookupLocale(locale)->loc_ShortDate,
                                                 &gd, unidate, sizeof(unidate));
                            if (err >= 0)
                                err = intlTransliterateString(unidate, INTL_CS_UNICODE,
                                                              date, INTL_CS_ASCII,
                                                              sizeof(date), ' ');

                            if (err < 0)
                            {
                                strcpy(date,"----------");
                            }

                            err = intlFormatDate(locale, intlLookupLocale(locale)->loc_Time,
                                                 &gd, unitime, sizeof(unitime));
                            if (err >= 0)
                                err = intlTransliterateString(unitime, INTL_CS_UNICODE,
                                                              time, INTL_CS_ASCII,
                                                              sizeof(time), ' ');

                            if (err < 0)
                            {
                                strcpy(time,"--------");
                            }

                            if (first)
                            {
                                printf("Type  Version     Date        Time      Name\n");
                                first = FALSE;
                            }

                            printf("%c%c%c%c  %3d.%-3d  %10s  %11.11s  %s\n",(fileStatus.fs_Type >> 24) & 0xff,
                                                                (fileStatus.fs_Type >> 16) & 0xff,
                                                                (fileStatus.fs_Type >> 8) & 0xff,
                                                                fileStatus.fs_Type & 0xff,
                                                                fileStatus.fs_Version,
                                                                fileStatus.fs_Revision,
                                                                date,
                                                                time,
                                                                argv[parm]);
                        }
                        else
                        {
                            if (setVersion)
                                fileStatus.fs_Version = version;

                            if (setRevision)
                                fileStatus.fs_Revision = revision;

                            if (setVersion || setRevision)
                            {
                                memset(&ioInfo,0,sizeof(IOInfo));
                                ioInfo.ioi_Command = FILECMD_SETVERSION;
                                ioInfo.ioi_Offset  = (fileStatus.fs_Version << 8)
                                                    | (fileStatus.fs_Revision);
                                err = DoIO(ioreq,&ioInfo);
                            }

                            if (err >= 0)
                            {
                                if (setFileType)
                                {
                                    memset(&ioInfo,0,sizeof(IOInfo));
                                    ioInfo.ioi_Command = FILECMD_SETTYPE;
                                    ioInfo.ioi_Offset  = fileType;
                                    err = DoIO(ioreq,&ioInfo);
                                }
                            }

                            if (err >= 0)
                            {
                                if (setDate)
                                {
                                    rem = fileStatus.fs_Date.tv_Seconds & SECONDS_PER_DAY;
                                    fileStatus.fs_Date.tv_Seconds      = dateTV.tv_Seconds + rem;
                                    fileStatus.fs_Date.tv_Microseconds = 0;
                                }

                                if (setTime)
                                {
                                    rem = fileStatus.fs_Date.tv_Seconds / SECONDS_PER_DAY;
                                    fileStatus.fs_Date.tv_Seconds      = timeTV.tv_Seconds + rem;
                                    fileStatus.fs_Date.tv_Microseconds = 0;
                                }

                                if (setDate || setTime)
                                {
                                    memset(&ioInfo,0,sizeof(IOInfo));
                                    ioInfo.ioi_Command         = FILECMD_SETDATE;
                                    ioInfo.ioi_Send.iob_Buffer = &fileStatus.fs_Date;
                                    ioInfo.ioi_Send.iob_Len    = sizeof(TimeVal);
                                    err = DoIO(ioreq,&ioInfo);
                                }
                            }

                            if (err < 0)
                            {
                                Error(("Unable to set attributes for %s: ",argv[parm]),err);
                            }
                        }
                    }
                    else
                    {
                        Error(("Unable to access %s: ",argv[parm]),err);
                    }
                    DeleteIOReq(ioreq);
                }
                else
                {
                    Error(("Unable to access %s: ",argv[parm]),err);
                }
                CloseFile(file);
            }
            else
            {
                Error(("Unable to open %s: ",argv[parm]),err);
            }

            if (err < 0)
                break;
        }
    }

    intlCloseLocale(locale);

    return 0;
}
