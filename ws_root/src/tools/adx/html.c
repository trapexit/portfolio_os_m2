/* @(#) html.c 96/04/15 1.46 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "adx.h"
#include "utils.h"
#include "options.h"


/****************************************************************************/


#define MAX_FILENAME_LENGTH 25


/****************************************************************************/


typedef struct
{
    Node     sn_Link;
    Autodoc *sn_Autodoc;
} SortNode;

typedef struct
{
    Node   in;
    Node  *in_Directory;
} IncludeNode;


/****************************************************************************/


static const char HTMLHeader[] = "<!DOCTYPE HTML PUBLIC \"-//TETF//DTD HTML 2.0//EN\">\n";


/****************************************************************************/


static IncludeNode *FindInclude(const char *includeName, int32 len, List *knownIncludes,
                                const char *autodocName)
{
IncludeNode *in;
Node        *node;
char         path[256];
FILE        *file;

    ScanList(knownIncludes, in, IncludeNode)
    {
        if ((strncmp(in->in.n_Name, includeName, len) == 0) && (in->in.n_Name[len] == 0))
        {
            return in;
        }
    }

    ScanList(&includeDirs, node, Node)
    {
        sprintf(path, "%s%c%.*s", node->n_Name, SEP, len, includeName);

        file = fopen(path, "r");
        if (file)
        {
            fclose(file);

            in = malloc(sizeof(IncludeNode) + len + 1);
            if (in)
            {
                in->in.n_Name = (char *)&in[1];
                strncpy(in->in.n_Name, includeName, len);
                in->in.n_Name[len] = 0;
                in->in_Directory = node;
                AddHead(knownIncludes, (Node *)in);
            }

            return in;
        }
    }

    if (verbose)
        printf("In autodoc '%s', no link created for include <%.*s>\n",autodocName,len,includeName);

    return NULL;
}


/****************************************************************************/


static void OutputRef(FILE *file, const List *manualList,
                      const char *sym, int32 len, const char *autodocName,
                      bool parens)
{
Manual  *manual;
Chapter *chapter;
Group   *group;
Autodoc *ad;
Node    *n;
int32    fileNameLen;
char    *found;
uint32   state;

    if (len < 0)
        len = strlen(sym);

    for (state = 0; state < 3; state++)
    {
        ScanList(manualList,manual,Manual)
        {
            ScanList(&manual->mn_Chapters,chapter,Chapter)
            {
                ScanList(&chapter->ch_Groups,group,Group)
                {
                    ScanList(&group->gr_Autodocs,ad,Autodoc)
                    {
                        found = NULL;
                        if (state == 0)
                        {
                            if (strncasecmp(ad->ad_Link.n_Name,sym,len) == 0)
                            {
                                if (ad->ad_Link.n_Name[len] == 0)
                                    found = ad->ad_Link.n_Name;
                            }
                        }
                        else if (state == 1)
                        {
                            ScanList(&ad->ad_VisibleAliases, n, Node)
                            {
                                if (strncasecmp(n->n_Name, sym, len) == 0)
                                {
                                    if (n->n_Name[len] == 0)
                                    {
                                        found = ad->ad_Link.n_Name;
                                        break;
                                    }
                                }
                            }
                        }
                        else if (state == 2)
                        {
                            ScanList(&ad->ad_HiddenAliases, n, Node)
                            {
                                if (strncasecmp(n->n_Name, sym, len) == 0)
                                {
                                    if (n->n_Name[len] == 0)
                                    {
                                        found = ad->ad_Link.n_Name;
                                        break;
                                    }
                                }
                            }
                        }

                        if (found)
                        {
                            fileNameLen = strlen(found);
                            if (fileNameLen > MAX_FILENAME_LENGTH)
                                fileNameLen = MAX_FILENAME_LENGTH;

                            if (parens)
                                fprintf(file,"<CODE><A HREF=\"./%.*s.html\">%.*s()</A></CODE>",fileNameLen,found,len,sym);
                            else
                                fprintf(file,"<CODE><A HREF=\"./%.*s.html\">%.*s</A></CODE>",fileNameLen,found,len,sym);

                            return;
                        }
                    }
                }
            }
        }
    }

    if (parens)
        fprintf(file,"<CODE>%.*s()</CODE>",len,sym);
    else
        fprintf(file,"<CODE>%.*s</CODE>",len,sym);

    if (verbose)
    {
        printf("In autodoc '%s', no link created for %.*s()\n",autodocName,len,sym);
    }
}


/****************************************************************************/


static void OutputLine(FILE *file, const List *manualList,  const char *line,
                       const char *autodocName, List *knownIncludes)
{
uint32       i, j, k;
uint32       start, end;
bool         italic;
bool         slash;
IncludeNode *foundInclude;

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
                    OutputRef(file,manualList,&line[i],end-i,autodocName,TRUE);
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
                        OutputRef(file,manualList,&line[i],end-i,autodocName,FALSE);
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
                if (((j >= 3) && ((strncasecmp(&line[j-2],".h>",3) == 0) || (strncasecmp(&line[j-2],".i>",3) == 0)))
                 || ((j >= 5) && (strncasecmp(&line[j-4],".inl>",5) == 0)))
                {
                    italic = TRUE;
                }
            }

            if (italic)
            {
                foundInclude = FindInclude(&line[i+1], j - i - 1, knownIncludes, autodocName);

                if (foundInclude)
                    fprintf(file,"%.*s<A HREF=\"./includes/%s\"><CITE>&lt;", i-start, &line[start], foundInclude->in.n_Name);
                else
                    fprintf(file,"%.*s<CITE>&lt;",i-start,&line[start]);

                slash = FALSE;

                if (outputPlatform != PLATFORM_UNIX)
                {
                    k = i+1;
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

                if (foundInclude)
                    fprintf(file,"&gt;</CITE></A>");
                else
                    fprintf(file,"&gt;</CITE>");

                i     = j;
                start = j+1;
            }
            else
            {
                fprintf(file,"%.*s&lt;",i-start,&line[start]);
                start = i+1;
            }
        }
        else if (line[i] == '>')
        {
            fprintf(file,"%.*s&gt;",i-start,&line[start]);
            start = i+1;
        }
        else if (line[i] == '&')
        {
            fprintf(file,"%.*s&amp;",i-start,&line[start]);
            start = i+1;
        }
        i++;
    }

    fprintf(file,"%.*s\n",i - start,&line[start]);
}


/****************************************************************************/


static void OutputHead(FILE *file, const char *title)
{
    fprintf(file,"%s",HTMLHeader);
    fprintf(file,"<HTML>\n");
    fprintf(file,"<HEAD>\n");
    fprintf(file,"<!-- Hey, what the heck do you think you're doing? -->\n");
    fprintf(file,"<!-- Do you think this is some sort of game? -->\n");
    fprintf(file,"<TITLE>%s</TITLE>\n",title);
    fprintf(file,"</HEAD>\n");
    fprintf(file,"<BODY>\n");
}


/****************************************************************************/


static void OutputTail(FILE *file)
{
    fprintf(file,"</BODY>\n");
    fprintf(file,"</HTML>\n");
}

/****************************************************************************/

int32 OutputHTML(const List *manualList, const char *outputDir)
{
FILE        *file;
FILE        *output;
Manual      *manual;
Chapter     *chapter;
Group       *group;
Autodoc     *ad;
Section     *section;
Line        *line;
Node        *n;
int32        result;
char         path[256];
bool         enumeration;
bool         indented;
bool         para;
List         knownIncludes;
IncludeNode *in;
#ifdef macintosh
	time_t t;
#else
	long   t;
#endif
char       date[128];
Line      *next;
SortNode  *sn;
List       sorted;
char       temp[256];
int        len;

    result = 0;

    PrepList(&knownIncludes);

    sprintf(path, "%s%cPortfolio_HTML", outputDir, SEP);
    mkdir(path);

    time(&t);
    strcpy(date,asctime(localtime(&t)));
    if (date[strlen(date)-1] == '\n')
        date[strlen(date)-1] = 0;

    sprintf(path,"%s%cPortfolio_TOC.html",outputDir,SEP);

    /* output global TOC */
    file = fopen(path,"w");
    if (file)
    {
        OutputHead(file,"Portfolio Reference");
        fprintf(file,"<H1><IMG SRC=\"./Portfolio_HTML/3DOLogo.gif\" ALIGN=\"middle\">\n");
        fprintf(file,"Portfolio Reference</H1>\n");
        fprintf(file,"<P>%s</P>\n",date);

        if (extractPrivate)
        {
            fprintf(file,"<I><BR>This reference material contains private system documentation\n");
            fprintf(file,"and must not be distributed outside of 3DO. You can generate a\n");
            fprintf(file,"sanitized version of this material, suitable for public\n");
            fprintf(file,"release by extracting the autodocs without the -private option.</I>\n\n");
        }

        ScanList(manualList,manual,Manual)
        {
            fprintf(file,"<H3>%s</H3>\n",manual->mn_FullName);

            if (manual->mn_Autodoc)
            {
                ScanList(&manual->mn_Autodoc->ad_Description,line,Line)
                {
                    OutputLine(file,manualList,line->ln_Data,NULL, &knownIncludes);
                }
            }

            fprintf(file,"<UL>\n");
            ScanList(&manual->mn_Chapters,chapter,Chapter)
            {
                fprintf(file,"<LI><CODE><A HREF=\"./Portfolio_HTML/%s_TOC.html\">%s</A></CODE>\n",chapter->ch_Link.n_Name,chapter->ch_FullName);
            }
            fprintf(file,"</UL>\n");
        }
        fprintf(file,"<H3><A HREF=\"./Portfolio_HTML/GlobalIndex.html\">Global Index</A></H3>\n");

        OutputTail(file);

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

    /* output local TOCs */
    ScanList(manualList,manual,Manual)
    {
        ScanList(&manual->mn_Chapters,chapter,Chapter)
        {
            if (result >= 0)
            {
                sprintf(path,"%s%cPortfolio_HTML%c%s_TOC.html",outputDir,SEP,SEP,chapter->ch_Link.n_Name);

                file = fopen(path,"w");
                if (file)
                {
                    OutputHead(file,chapter->ch_FullName);

                    if (chapter->ch_Autodoc)
                    {
                        ScanList(&chapter->ch_Autodoc->ad_Description,line,Line)
                        {
                            OutputLine(file,manualList,line->ln_Data,NULL,&knownIncludes);
                        }
                    }

                    fprintf(file,"<UL>\n");
                    ScanList(&chapter->ch_Groups,group,Group)
                    {
                        if (group->gr_Link.n_Name[0])
                            fprintf(file,"</UL>\n<H3>%s</H3>\n<UL>\n",group->gr_Link.n_Name);

                        ScanList(&group->gr_Autodocs,ad,Autodoc)
                        {
                            PrepList(&sorted);
                            sn = malloc(sizeof(SortNode));
                            if (sn)
                            {
                                sn->sn_Link.n_Name = ad->ad_Link.n_Name;
                                sn->sn_Autodoc     = ad;
                                AddHead(&sorted,(Node *)sn);
                            }
                            else
                            {
                                fclose(file);
                                NoMemory();
                                return -1;
                            }

                            ScanList(&ad->ad_VisibleAliases, n, Node)
                            {
                                sn = malloc(sizeof(SortNode));
                                if (sn)
                                {
                                    sn->sn_Link.n_Name = n->n_Name;
                                    sn->sn_Autodoc     = ad;
                                    InsertNodeAlpha(&sorted,(Node *)sn);
                                }
                                else
                                {
                                    fclose(file);
                                    NoMemory();
                                    return -1;
                                }
                            }

                            ScanList(&sorted,sn,SortNode)
                            {
                                ad = sn->sn_Autodoc;
                                fprintf(file,"<LI><CODE><A HREF=\"./%.*s.html\">%s</A></CODE>%s - ",MAX_FILENAME_LENGTH,ad->ad_Link.n_Name,sn->sn_Link.n_Name, (ad->ad_Private ? " (private)" : ""));
                                ScanList(&ad->ad_Description,line,Line)
                                {
                                    OutputLine(file,manualList,line->ln_Data,NULL,&knownIncludes);
                                }
                            }

                            while (sn = (SortNode *)RemHead(&sorted))
                                free(sn);
                        }
                    }

                    fprintf(file,"</UL>\n");
                    OutputTail(file);

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

    /* output global index */
    if (result >= 0)
    {
        sprintf(path,"%s%cPortfolio_HTML%cGlobalIndex.html",outputDir,SEP,SEP);

        file = fopen(path,"w");
        if (file)
        {
            OutputHead(file,"Portfolio Global Index");
            fprintf(file,"<UL>\n");

	    /* perform a painfully slow insertion sort... */

            PrepList(&sorted);
            ScanList(manualList,manual,Manual)
            {
                ScanList(&manual->mn_Chapters,chapter,Chapter)
                {
                    ScanList(&chapter->ch_Groups,group,Group)
                    {
                        ScanList(&group->gr_Autodocs,ad,Autodoc)
                        {
                            sn = malloc(sizeof(SortNode));
                            if (sn)
                            {
                                sn->sn_Link.n_Name = ad->ad_Link.n_Name;
                                sn->sn_Autodoc     = ad;
                                InsertNodeAlpha(&sorted,(Node *)sn);
                            }
                            else
                            {
                                fclose(file);
                                NoMemory();
                                return -1;
                            }

                            ScanList(&ad->ad_VisibleAliases, n, Node)
                            {
                                sn = malloc(sizeof(SortNode));
                                if (sn)
                                {
                                    sn->sn_Link.n_Name = n->n_Name;
                                    sn->sn_Autodoc     = ad;
                                    InsertNodeAlpha(&sorted,(Node *)sn);
                                }
                                else
                                {
                                    fclose(file);
                                    NoMemory();
                                    return -1;
                                }
                            }
                        }
                    }
                }
            }

            fprintf(file,"<CENTER><H3>\n");
            ScanList(&sorted,sn,SortNode)
            {
                if ((sn == (SortNode *)FirstNode(&sorted))
                 || (toupper(sn->sn_Link.n_Name[0]) != toupper(((SortNode *)PrevNode((Node *)sn))->sn_Link.n_Name[0])))
                {
                    fprintf(file,"<A HREF=\"#%c\">%c</A>\n",toupper(sn->sn_Link.n_Name[0]), toupper(sn->sn_Link.n_Name[0]));
                }
            }
            fprintf(file,"</CENTER></H3><BR><HR>\n");

            ScanList(&sorted,sn,SortNode)
            {
                if ((sn == (SortNode *)FirstNode(&sorted))
                 || (toupper(sn->sn_Link.n_Name[0]) != toupper(((SortNode *)PrevNode((Node *)sn))->sn_Link.n_Name[0])))
                {
                    fprintf(file,"</UL>\n<A NAME=\"%c\"><H3>%c</H3></A>\n<UL>\n",toupper(sn->sn_Link.n_Name[0]), toupper(sn->sn_Link.n_Name[0]));
                }

                ad = sn->sn_Autodoc;
                fprintf(file,"<LI><CODE><A HREF=\"./%.*s.html\">%s</A></CODE> - ",MAX_FILENAME_LENGTH,ad->ad_Link.n_Name,sn->sn_Link.n_Name);
                ScanList(&ad->ad_Description,line,Line)
                {
                    OutputLine(file,manualList,line->ln_Data,NULL,&knownIncludes);
                }
            }

            while (sn = (SortNode *)RemHead(&sorted))
                free(sn);

            fprintf(file,"</UL>\n");
            OutputTail(file);

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
                            sprintf(path,"%s%cPortfolio_HTML%c%.*s.html",outputDir,SEP,SEP,MAX_FILENAME_LENGTH,ad->ad_Link.n_Name);

                            file = fopen(path,"w");
                            if (file)
                            {
                                if (group->gr_Link.n_Name[0])
                                {
                                    sprintf(temp,"%s - %s",chapter->ch_FullName,group->gr_Link.n_Name);
                                    OutputHead(file,temp);
                                }
                                else
                                {
                                    OutputHead(file,chapter->ch_FullName);
                                }
                                fprintf(file,"<H1><A NAME=\"%s\">%s</A></H1>%s\n",ad->ad_Link.n_Name,ad->ad_Link.n_Name, (ad->ad_Private ? " (private)" : ""));

                                ScanList(&ad->ad_VisibleAliases, n, Node)
                                {
                                    fprintf(file,"<BR><H1>%s</H1>\n",n->n_Name);
                                }

                                fprintf(file,"<P>\n");
                                ScanList(&ad->ad_Description,line,Line)
                                {
                                    OutputLine(file,manualList,line->ln_Data,ad->ad_Link.n_Name, &knownIncludes);
                                }
                                fprintf(file,"</P>\n");

                                ScanList(&ad->ad_Sections,section,Section)
                                {
                                    if (section->s_Link.n_Name)
                                    {
                                        fprintf(file,"<H3>");
                                        if ((strcasecmp(section->s_Link.n_Name,"Warning") == 0)
                                         || (strcasecmp(section->s_Link.n_Name,"Warnings") == 0)
                                         || (strcasecmp(section->s_Link.n_Name,"Caveat") == 0)
                                         || (strcasecmp(section->s_Link.n_Name,"Caveats") == 0))
                                        {
                                            fprintf(file,"<IMG SRC=\"./Warning.gif\" ALIGN=\"middle\">  ");
                                        }

                                        fprintf(file,"%s</H3>\n",section->s_Link.n_Name);
                                    }
                                    else
                                    {
                                        fprintf(file,"</P><P>\n");
                                    }

                                    if (section->s_Type == FT_PREFORMATTED)
                                    {
                                        fprintf(file,"<PRE>\n");
                                        ScanList(&section->s_Lines,line,Line)
                                        {
                                            OutputLine(file,manualList,line->ln_Data,ad->ad_Link.n_Name, &knownIncludes);
                                        }
                                        fprintf(file,"</PRE>\n");
                                    }
                                    else
                                    {
                                        enumeration = FALSE;
                                        indented    = FALSE;

                                        ScanList(&section->s_Lines,line,Line)
                                        {
                                            if (line->ln_Data[0] == 0)
                                            {
                                                fprintf(file,"<P>\n");
                                            }
                                            else if ((line->ln_Data[0] != ' ')
                                                  || (line->ln_Data[1] != ' ')
                                                  || (line->ln_Data[2] != ' ')
                                                  || (line->ln_Data[3] != ' '))
                                            {
                                                indented = FALSE;
                                                if (line != (Line *)LastNode(&section->s_Lines))
                                                {
                                                    next = (Line *)NextNode(line);
                                                    if ((next->ln_Data[0] == ' ')
                                                     && (next->ln_Data[1] == ' ')
                                                     && (next->ln_Data[2] == ' ')
                                                     && (next->ln_Data[3] == ' '))
                                                    {
                                                        if (!enumeration)
                                                        {
                                                            enumeration = TRUE;
                                                            fprintf(file,"<DL>\n");
                                                        }
                                                        fprintf(file,"<DT>");
                                                    }
                                                    else if (enumeration)
                                                    {
                                                        enumeration = FALSE;
                                                        fprintf(file,"</DL>");
                                                    }
                                                }
                                                else if (enumeration)
                                                {
                                                    enumeration = FALSE;
                                                    fprintf(file,"</DL>");
                                                }
                                                OutputLine(file,manualList,line->ln_Data,ad->ad_Link.n_Name, &knownIncludes);
                                            }
                                            else
                                            {
                                                if (!indented)
                                                {
                                                    indented = TRUE;
                                                    fprintf(file,"<DD>");
                                                }
                                                OutputLine(file,manualList,&line->ln_Data[4],ad->ad_Link.n_Name, &knownIncludes);
                                            }
                                        }

                                        if (enumeration)
                                            fprintf(file,"</DL>\n");
                                    }
                                }

                                fprintf(file,"<BR><BR><HR><BR>\n");

                                if (IsNodeB(&group->gr_Autodocs,PrevNode((Node *)ad)))
                                {
                                    fprintf(file,"<IMG SRC=\"./Previous.gif\" ALT=\"Previous\" ALIGN=\"middle\">  ");
                                    OutputRef(file,manualList,((Autodoc *)PrevNode((Node *)ad))->ad_Link.n_Name,-1,ad->ad_Link.n_Name,FALSE);
                                    fprintf(file,"  \n");
                                }

                                if (IsNode(&group->gr_Autodocs,NextNode((Node *)ad)))
                                {
                                    fprintf(file,"<IMG SRC=\"./Next.gif\" ALT=\"Next\" ALIGN=\"middle\">  ");
                                    OutputRef(file,manualList,((Autodoc *)NextNode((Node *)ad))->ad_Link.n_Name,-1,ad->ad_Link.n_Name,FALSE);
                                    fprintf(file,"\n");
                                }

                                OutputTail(file);

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
        }
    }

    if (result >= 0)
    {
        if (!IsEmptyList(&knownIncludes))
        {
            sprintf(path, "%s%cPortfolio_HTML%cincludes", outputDir, SEP, SEP);
            mkdir(path);

            /* now install needed include files */
            ScanList(&knownIncludes, in, IncludeNode)
            {
                sprintf(path, "%s%c%s",in->in_Directory->n_Name, SEP, in->in.n_Name);
                file = fopen(path, "r");
                if (file)
                {
                    sprintf(path, "%s%cPortfolio_HTML%cincludes%c%s", outputDir, SEP, SEP, SEP, in->in.n_Name);

                    len = strlen(path);
                    while (TRUE)
                    {
                        len--;
                        if (path[len] == SEP)
                        {
                            path[len] = 0;
                            break;
                        }
                    }
                    mkdir(path);

                    sprintf(path, "%s%cPortfolio_HTML%cincludes%c%s", outputDir, SEP, SEP, SEP, in->in.n_Name);
                    output = fopen(path, "w");
                    if (output)
                    {
                    int ch;

                       while ((ch = getc(file)) != EOF)
                           putc(ch, output);

                       fclose(output);
                    }
                    else
                    {
                        IOError("Unable to open '%s' for output",path);
                        result = -1;
                    }

                    fclose(file);
                }
                else
                {
                    IOError("Unable to open '%s' for input",path);
                    result = -1;
                }

                if (result < 0)
                    break;
            }
        }
    }

    while (in = (IncludeNode *)RemHead(&knownIncludes))
        free(in);

    return result;
}
