/* @(#) troff.c 96/03/11 1.28 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "adx.h"
#include "utils.h"
#include "options.h"


/****************************************************************************/


static void OutputLine(FILE *file, const char *line)
{
uint32 i, j, k;
uint32 start;
uint32 end;
bool   italic;
bool   slash;

    if ((line[0] == '.') || (line[0] == '\''))
    {
        fprintf(file," ");
    }

    start = 0;
    i     = 0;
    while (line[i])
    {
        if (line[i] == '(')
        {
            if (line[i+1] == ')')
            {
                if (i > 0)
                {
                    end = i;
                    do
                    {
                        i--;
                        if (!isgraph(line[i]) || (line[i] == '('))
                        {
                            i++;
                            break;
                        }
                    }
                    while (i);

                    fprintf(file,"%.*s",i-start,&line[start]);
                    fprintf(file,"\\f4%.*s()\\fP",end-i,&line[i]);
                    start = end+2;
                    i     = end+2;
                }
            }
            else if (line[i+1] == '@')
            {
                if (line[i+2] == ')')
                {
                    if (i > 0)
                    {
                        end = i;
                        do
                        {
                            i--;
                            if (!isgraph(line[i]) || (line[i] == '('))
                            {
                                i++;
                                break;
                            }
                        }
                        while (i);

                        fprintf(file,"%.*s",i-start,&line[start]);
                        fprintf(file,"\\f4%.*s\\fP",end-i,&line[i]);
                        start = end+3;
                        i     = end+3;
                    }
                }
            }
        }
        else if (line[i] == '<')
        {
            j = i+1;
            while ((line[j] != '>') && line[j])
                j++;

            italic = FALSE;
            if (line[j] == '>')
            {
                if (((j >= 3) && ((strncasecmp(&line[j-2],".h>",3) == 0) || (strnicmp(&line[j-2],".i>",3) == 0)))
                 || ((j >= 5) && (strncasecmp(&line[j-4],".inl>",5) == 0)))
                {
                    italic = TRUE;
                }
            }

            if (italic)
            {
                fprintf(file,"%.*s\\fI<",i-start,&line[start]);

                slash = FALSE;
                if (outputPlatform != PLATFORM_UNIX)
                {
                    k     = i+1;
                    while (k < j)
                    {
                        if (line[k] == '/')
                        {
                            slash = TRUE;
                            break;
                        }
                        k++;
                    }
                }

                if (slash)
                {
                    if (outputPlatform == PLATFORM_MAC)
                        fprintf(file,":");

                    k = i+1;
                    while (k < j)
                    {
                        if (line[k] == '/')
                        {
                            if (outputPlatform == PLATFORM_MAC)
                                fprintf(file,":");
                            else
                                fprintf(file,"\\");
                        }
                        else
                        {
                            fprintf(file,"%c",line[k]);
                        }

                        k++;
                    }
                }
                else
                {
                    fprintf(file,"%.*s",j-i-1,&line[i+1]);
                }

                fprintf(file,">\\fP");
                i     = j;
                start = j+1;
            }
            else
            {
                fprintf(file,"%.*s<",i-start,&line[start]);
                start = i+1;
            }
        }
        else if (line[i] == '\\')
        {
            fprintf(file,"%.*s\\e",i - start,&line[start]);
            start = i+1;
        }
        i++;
    }

    fprintf(file,"%.*s\n",i - start,&line[start]);
}


/****************************************************************************/


#define PAGE_LENGTH             "11.0"
#define TOP_MARGIN              "0.5"
#define RIGHT_MARGIN            "1.0"
#define NORMAL_LINE_WIDTH       "6.5"
#define INDENTED_LINE_WIDTH     "6.25"
#define INDENT_WIDTH            ".25"
#define LARGE_INDENT_WIDTH      ".75"

#define SPACE_AFTER_DESCRIPTION "0.15"
#define SPACE_AFTER_HEADER      "0.2"
#define SPACE_BEFORE_SECTION    "10"    /* in points */
#define SPACE_AFTER_SECTION     "5"     /* in points */

#define TITLE_POINT_SIZE        14
#define HEADER_POINT_SIZE       11
#define SECTION_POINT_SIZE      12
#define BODY_POINT_SIZE         11


static void OutputAutodoc(FILE *file, Autodoc *ad, bool forPrint, char *date)
{
Section *section;
Line    *line;
Node    *n;
bool     header;
char     lineLen[20];
char     enumLineLen[20];
char    *last;

    if (forPrint)
    {
        strcpy(lineLen, NORMAL_LINE_WIDTH);
        strcpy(enumLineLen, INDENTED_LINE_WIDTH);

        fprintf(file,".in 0\n");
        fprintf(file,".nh\n");
        fprintf(file,".ll %si\n",lineLen);
        fprintf(file,".ft B\n");
        fprintf(file,".ps %d\n%s\n", TITLE_POINT_SIZE, ad->ad_Link.n_Name);
        last = ad->ad_Link.n_Name;
        ScanList(&ad->ad_VisibleAliases, n, Node)
        {
            fprintf(file,".br\n%s", n->n_Name);
            last = n->n_Name;
        }
        fprintf(file,"\\h'(3.0i-\\w'%s'u)'\n",last);
        fprintf(file,".ps %d\n.ft P\n", BODY_POINT_SIZE);
        fprintf(file,"'in 3.0i\n");
    }
    else
    {
        strcpy(lineLen,"7.5");
        strcpy(enumLineLen,"7.25");

        fprintf(file,".\\\" ADX Output File\n");
        fprintf(file,".\\\" Generated on %s\n",date);
        fprintf(file,".fp 4 C\n");
        fprintf(file,".in 0\n");
        fprintf(file,".nh\n");
        fprintf(file,".sp\n.ft B\n.ps 12\nName\n.ps 9\n.ft P\n.sp\n");
        fprintf(file,".ll %si\n",lineLen);
        fprintf(file,".in .25i\n");
        fprintf(file,"%s%s",ad->ad_Link.n_Name, (ad->ad_Private ? " (private)" : ""));
        ScanList(&ad->ad_VisibleAliases, n, Node)
        {
            fprintf(file,", %s", n->n_Name);
        }
        fprintf(file," \\- ",ad->ad_Link.n_Name, (ad->ad_Private ? " (private)" : ""));
    }

    ScanList(&ad->ad_Description,line,Line)
    {
        OutputLine(file,line->ln_Data);
    }

    if (forPrint)
    {
        fprintf(file,".sp %si\n", SPACE_AFTER_DESCRIPTION);
    }

    ScanList(&ad->ad_Sections,section,Section)
    {
        if (section->s_Link.n_Name)
        {
            fprintf(file,".in 0\n");
            fprintf(file,".sp %sp\n", SPACE_BEFORE_SECTION);
            fprintf(file,".ft B\n");
            fprintf(file,".ps %d\n%s\n.ps %d\n.ft P\n", SECTION_POINT_SIZE, section->s_Link.n_Name, BODY_POINT_SIZE);

            if (forPrint)
                fprintf(file,".sp %sp\n", SPACE_AFTER_SECTION);
            else
                fprintf(file,".sp\n");
        }
        else
        {
            fprintf(file,".sp\n");
        }

        fprintf(file,".ll %si\n",lineLen);
        fprintf(file,".in %si\n", INDENT_WIDTH);

        if (section->s_Type == FT_PREFORMATTED)
        {
            fprintf(file,".ad l\n");
            fprintf(file,".ft 4\n");

            ScanList(&section->s_Lines,line,Line)
            {
                OutputLine(file,line->ln_Data);
                fprintf(file,".br\n");
            }

            fprintf(file,".ft P\n");
            fprintf(file,".ad b\n");
        }
        else
        {
            header = TRUE;
            ScanList(&section->s_Lines,line,Line)
            {
                if (line->ln_Data[0] == 0)
                {
                    OutputLine(file,line->ln_Data);
                }
                else if ((line->ln_Data[0] == ' ')
                      && (line->ln_Data[1] == ' ')
                      && (line->ln_Data[2] == ' ')
                      && (line->ln_Data[3] == ' '))
                {
                    if (header)
                    {
                        fprintf(file,".ll %si\n",enumLineLen);
                        fprintf(file,".in %si\n", LARGE_INDENT_WIDTH);
                        header = FALSE;
                    }
                    OutputLine(file,&line->ln_Data[4]);
                }
                else
                {
                    if (!header)
                    {
                        fprintf(file,".ll %si\n",lineLen);
                        fprintf(file,".in %si\n", INDENT_WIDTH);
                        header = TRUE;
                    }
                    OutputLine(file,line->ln_Data);
                }
            }
        }
    }

    if (forPrint)
        fprintf(file,".bp\n");
}


/*****************************************************************************/


static void OutputStartPage(FILE *file, Manual *manual)
{
    fprintf(file,"'sp %si\n", TOP_MARGIN);
    fprintf(file,".ft 1\n");
    fprintf(file,".ps %d\n", HEADER_POINT_SIZE);
    fprintf(file,".if e .tl '%s'\n",manual->mn_FullName);
    fprintf(file,".if o .tl '''%s'\n",manual->mn_FullName);
    fprintf(file,".ti 0\n");
    fprintf(file,"\\l'6.60i'\n");
    fprintf(file,".br\n");
    fprintf(file,".sp %si\n", SPACE_AFTER_HEADER);
    fprintf(file,".ps\n");
    fprintf(file,".ft P\n");
}


/*****************************************************************************/


static int32 OutputTROFF(const List *manualList, const char *outputDir,
                         bool forPrint)
{
FILE      *file;
Manual    *manual;
Chapter   *chapter;
Group     *group;
Autodoc   *ad;
Line      *line;
Node      *n;
int32      result;
#ifdef macintosh
	time_t t;
#else
	long   t;
#endif
char       path[256];
char       date[128];
uint32     chapCount;

    mkdir(outputDir);

    time(&t);
    strcpy(date,asctime(localtime(&t)));
    if (date[strlen(date)-1] == '\n')
        date[strlen(date)-1] = 0;

    if (!forPrint)
    {
        sprintf(path,"%s%cwhatis",outputDir,SEP);

        file = fopen(path,"w");
        if (file)
        {
            ScanList(manualList,manual,Manual)
            {
                ScanList(&manual->mn_Chapters,chapter,Chapter)
                {
                    ScanList(&chapter->ch_Groups,group,Group)
                    {
                        ScanList(&group->gr_Autodocs,ad,Autodoc)
                        {
                            fprintf(file,"%s",ad->ad_Link.n_Name);
                            ScanList(&ad->ad_VisibleAliases, n, Node)
                            {
                                fprintf(file, ", %s", n->n_Name);
                            }
                            fprintf(file," (Portfolio) - %s\n",((Line *)FirstNode(&ad->ad_Description))->ln_Data);
                        }
                    }
                }
            }

            result = fclose(file);

            if (result < 0)
            {
                IOError("Error writing to '%s'",path);
            }
        }
        else
        {
            IOError("Unable to open '%s' for output",path);
            result = -1;
        }
    }
    else
    {
        result = 0;
    }

    if (result >= 0)
    {
        if (forPrint)
        {
            ScanList(manualList,manual,Manual)
            {
                if (result >= 0)
                {
                    sprintf(path,"%s%c%s.troff",outputDir,SEP,manual->mn_Link.n_Name);

                    file = fopen(path,"w");
                    if (file)
                    {
                        fprintf(file,".\\\" ADX Output File\n");
                        fprintf(file,".\\\" Generated on %s\n",date);
                        fprintf(file,".pl %si\n", PAGE_LENGTH);
                        fprintf(file,".po %si\n", RIGHT_MARGIN);
                        fprintf(file,".de NP\n");
                        fprintf(file,".ps 11\n");
                        fprintf(file,".sp 0.2i\n");
                        fprintf(file,".ti 0\n");
                        fprintf(file,"\\l'6.60i'\n");
                        fprintf(file,".br\n");
                        fprintf(file,".if e .tl '%s * %%'\n",manual->mn_Link.n_Name);
                        fprintf(file,".if o .tl '''%s * %%'\n",manual->mn_Link.n_Name);
                        fprintf(file,"'bp\n");
                        fprintf(file,".ps\n");
                        OutputStartPage(file,manual);
                        fprintf(file,"..\n");
                        fprintf(file,".fp 4 C\n");

                        fprintf(file,".sp 2.5i\n");
                        fprintf(file,".ps 30\n");
                        fprintf(file,".ft B\n");
                        fprintf(file,".ad c\n");
                        fprintf(file,"%s\n",manual->mn_FullName);
                        fprintf(file,".br\n.sp\n");
                        fprintf(file,".ft P\n");
                        fprintf(file,".br\n");
                        fprintf(file,".ps 12\n");
                        fprintf(file,".ad b\n");
                        fprintf(file,".sp 3\n");

                        if (manual->mn_Autodoc)
                        {
                            ScanList(&manual->mn_Autodoc->ad_Description,line,Line)
                            {
                                OutputLine(file,line->ln_Data);
                            }
                        }

                        fprintf(file,".sp 5\n");
                        fprintf(file,".ad c\n");
                        fprintf(file,"%s\n",date);
                        fprintf(file,".ad b\n");
                        fprintf(file,".bp\n");
                        fprintf(file,".bp\n");
                        fprintf(file,".nr % 1\n");
                        fprintf(file,".wh -1.0i NP\n");
                        OutputStartPage(file,manual);

                        fprintf(file,".nr %% 1\n");

                        chapCount = 1;
                        ScanList(&manual->mn_Chapters,chapter,Chapter)
                        {
                            fprintf(file,".de NP\n");
                            fprintf(file,".ps 11\n");
                            fprintf(file,".sp 0.2i\n");
                            fprintf(file,".ti 0\n");
                            fprintf(file,"\\l'6.60i'\n");
                            fprintf(file,".br\n");
                            fprintf(file,".if e .tl '%s * %%''%s'\n",manual->mn_Link.n_Name,chapter->ch_FullName);
                            fprintf(file,".if o .tl '%s''%s * %%'\n",chapter->ch_FullName,manual->mn_Link.n_Name);
                            fprintf(file,"'bp\n");
                            fprintf(file,".ps\n");
                            OutputStartPage(file,manual);
                            fprintf(file,"..\n");

                            fprintf(file,".if e .bp\n");
                            fprintf(file,".sp 2i\n");
                            fprintf(file,".ps 20\n");
                            fprintf(file,".ad c\n");
                            fprintf(file,"Chapter %d\n",chapCount);
                            fprintf(file,".br\n");
                            fprintf(file,".sp\n");
                            fprintf(file,".sp\n");
                            fprintf(file,".ft B\n");
                            fprintf(file,".ps 25\n");
                            fprintf(file,"%s\n",chapter->ch_FullName);
                            fprintf(file,".ft P\n");
                            fprintf(file,".br\n");
                            fprintf(file,".ps 12\n");
                            fprintf(file,".ad b\n");
                            fprintf(file,".sp 3\n");

                            if (chapter->ch_Autodoc)
                            {
                                ScanList(&chapter->ch_Autodoc->ad_Description,line,Line)
                                {
                                    OutputLine(file,line->ln_Data);
                                }
                            }

                            fprintf(file,".bp\n");
                            fprintf(file,".bp\n");

                            ScanList(&chapter->ch_Groups,group,Group)
                            {
                                ScanList(&group->gr_Autodocs,ad,Autodoc)
                                {
                                    OutputAutodoc(file,ad,TRUE,date);
                                }
                            }

                            chapCount++;
                        }

                        result = fclose(file);
                        if (result < 0)
                        {
                            IOError("Error writing to '%s'",path);
                        }
                    }
                    else
                    {
                        IOError("Unable to open '%s' for output",path);
                        result = -1;
                    }
                }
            }

            ScanList(manualList,manual,Manual)
            {
                ScanList(&manual->mn_Chapters,chapter,Chapter)
                {
                    if (result >= 0)
                    {
                        sprintf(path,"%s%c%s.index",outputDir,SEP,chapter->ch_Link.n_Name);

                        file = fopen(path,"w");
                        if (file)
                        {
                            ScanList(&chapter->ch_Groups,group,Group)
                            {
                                if (group->gr_Link.n_Name[0])
                                    fprintf(file,"\n\n%s\n\n",group->gr_Link.n_Name);

                                ScanList(&group->gr_Autodocs,ad,Autodoc)
                                {
                                    fprintf(file,"%s - ",ad->ad_Link.n_Name);
                                    ScanList(&ad->ad_Description,line,Line)
                                    {
                                        if (line != (Line *)FirstNode(&ad->ad_Description))
                                            fprintf(file,"%.*s",strlen(ad->ad_Link.n_Name)+3,"                                       ");

                                        OutputLine(file,line->ln_Data);
                                    }

                                    ScanList(&ad->ad_VisibleAliases, n, Node)
                                    {
                                        fprintf(file,"%s - ",n->n_Name);
                                        ScanList(&ad->ad_Description,line,Line)
                                        {
                                            if (line != (Line *)FirstNode(&ad->ad_Description))
                                                fprintf(file,"%.*s",strlen(ad->ad_Link.n_Name)+3,"                                       ");

                                            OutputLine(file,line->ln_Data);
                                        }
                                    }
                                }
                            }

                            fprintf(file,"\n");
                            result = fclose(file);
                            if (result < 0)
                            {
                                IOError("Error writing to '%s'",path);
                            }
                        }
                        else
                        {
                            IOError("Unable to open '%s' for output",path);
                            result = -1;
                        }
                    }
                }
            }
        }
        else
        {
            sprintf(path,"%s%cman1",outputDir,SEP);
	    mkdir(path);

            ScanList(manualList,manual,Manual)
            {
                ScanList(&manual->mn_Chapters,chapter,Chapter)
                {
                    ScanList(&chapter->ch_Groups,group,Group)
                    {
                        ScanList(&group->gr_Autodocs,ad,Autodoc)
                        {
                            if (result >= 0)
                            {
                                sprintf(path,"%s%cman1%c%s.1",outputDir,SEP,SEP,ad->ad_Link.n_Name);

                                file = fopen(path,"w");
                                if (file)
                                {
                                    OutputAutodoc(file,ad,FALSE,date);

                                    result = fclose(file);
                                    if (result < 0)
                                    {
                                        IOError("Error writing to '%s'",path);
                                    }
                                }
                                else
                                {
                                    IOError("Unable to open '%s' for output",path);
                                    result = -1;
                                }
                            }

                            if (result >= 0)
                            {
                                ScanList(&ad->ad_VisibleAliases, n, Node)
                                {
                                    sprintf(path,"%s%cman1%c%s.1",outputDir,SEP,SEP,n->n_Name);

                                    file = fopen(path,"w");
                                    if (file)
                                    {
                                        fprintf(file,".\\\" ADX Output File\n");
                                        fprintf(file,".\\\" Generated on %s\n",date);
                                        fprintf(file,".so man1/%s.1\n",ad->ad_Link.n_Name);

                                        result = fclose(file);
                                        if (result < 0)
                                        {
                                            IOError("Error writing to '%s'",path);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}


/*****************************************************************************/


int32 OutputMAN(const List *manualList, const char *outputDir)
{
    return OutputTROFF(manualList,outputDir,FALSE);
}


/*****************************************************************************/


int32 OutputPrint(const List *manualList, const char *outputDir)
{
    return OutputTROFF(manualList,outputDir,TRUE);
}
