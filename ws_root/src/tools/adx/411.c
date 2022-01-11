/* @(#) 411.c 96/03/11 1.15 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "adx.h"
#include "utils.h"


/*****************************************************************************/


static void OutputLine(FILE *file, const char *line)
{
uint32 i, j, k;
uint32 start;
uint32 end;
bool   italic;
bool   slash;

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
                    fprintf(file,"%.*s()",end-i,&line[i]);
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
                        fprintf(file,"%.*s",end-i,&line[i]);
                        start = end+3;
                        i     = end+3;
                    }
                }
            }
        }
        i++;
    }

    fprintf(file,"%.*s\n",i - start,&line[start]);
}


/****************************************************************************/


int32 Output411(const List *manualList, const char *outputDir)
{
FILE      *file;
Manual    *manual;
Chapter   *chapter;
Autodoc   *ad;
Section   *section;
Line      *line;
int32      result;
#ifdef macintosh
	time_t t;
#else
	long   t;
#endif
char       path[256];
Group     *group;
Node      *n;

   result = -1;
   mkdir(outputDir);

   sprintf(path,"%s%cPortfolioHelp",outputDir,SEP);
   file = fopen(path,"w");
   if (file)
   {
        time(&t);
        fprintf(file,"\nADX Output File\n");
        fprintf(file,"Generated on %s\n",asctime(localtime(&t)));

        ScanList(manualList,manual,Manual)
        {
            ScanList(&manual->mn_Chapters,chapter,Chapter)
            {
                ScanList(&chapter->ch_Groups,group,Group)
                {
                    ScanList(&group->gr_Autodocs,ad,Autodoc)
                    {
			fprintf(file,"\n%cKY %s\n",0xBE,ad->ad_Link.n_Name);
			ScanList(&ad->ad_VisibleAliases, n, Node)
			{
                            fprintf(file,"\n%cKY %s\n",0xBE,n->n_Name);
			}

			fprintf(file,"%cC\n",0xBE);
                        ScanList(&ad->ad_Description,line,Line)
                        {
                            OutputLine(file, line->ln_Data);
                        }

                        ScanList(&ad->ad_Sections,section,Section)
                        {
                            if (section->s_Link.n_Name)
                                fprintf(file,"\n  %s\n\n",section->s_Link.n_Name);
                            else
                                fprintf(file,"\n",section->s_Link.n_Name);

                            ScanList(&section->s_Lines,line,Line)
                            {
                                fprintf(file, "    ");
                                OutputLine(file, line->ln_Data);
                            }
                        }
                    }
                }
            }
        }
        result = fclose(file);
     }
     else
     {
        IOError("Unable to open '%s' for output",path);
     }

     return result;
}
