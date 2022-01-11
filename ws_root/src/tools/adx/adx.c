/* @(#) adx.c 96/04/29 1.31 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include "adx.h"
#include "utils.h"
#include "options.h"


/****************************************************************************/


OutputPlatforms outputPlatform;
bool            verbose;
bool            extractPrivate;
bool            extractInternal;
List            includeDirs;

#define VERBOSE(x) if (verbose) printf x


/****************************************************************************/


typedef int32 (* OutputFunc)(const List *classList, const char *fileName);

typedef struct
{
    char       *ot_TypeName;
    OutputFunc  ot_OutputFunc;
    char       *ot_OutputDir;
} OutputType;

static OutputType outputTypes[] =
{
    {"ascii", OutputASCII, NULL},
    {"html",  OutputHTML,  NULL},
    {"411",   Output411,   NULL},
    {"man",   OutputMAN,   NULL},
    {"print", OutputPrint, NULL},
    {NULL}
};


/****************************************************************************/


typedef struct
{
    Node  def_Link;
} Define;

#define def_Name def_Link.n_Name

typedef struct
{
    Node ib_Link;
    bool ib_Skip;
    bool ib_Else;
} IfBlock;


/****************************************************************************/


/* parsing state */
static FILE       *file;
static char        lineBuffer[2048];
static char       *inputLine;
static uint32      lineNum;
static uint32      lineLen;
static const char *fileName;
static bool        validAutodocLine;
static bool        parsingAutodoc;


/****************************************************************************/


void ParseWarning(const char *msg, ...)
{
va_list a;

    va_start(a,msg);

    fprintf(stderr,"ADX Warning: ");
    vfprintf(stderr,msg,a);
    fprintf(stderr,"\n             file '%s', line %d\n",fileName,lineNum);

    va_end(a);
}


/****************************************************************************/


void ParseError(const char *msg, ...)
{
va_list a;

    va_start(a,msg);

    fprintf(stderr,"ADX Error: ");
    vfprintf(stderr,msg,a);
    fprintf(stderr,"\n           file '%s', line %d\n",fileName,lineNum);

    va_end(a);
}


/****************************************************************************/


void IOError(const char *msg, ...)
{
va_list a;

    va_start(a,msg);

    fprintf(stderr,"ADX IO Error: ");
    vfprintf(stderr,msg,a);
    fprintf(stderr,"\n              ");
    perror(NULL);

    va_end(a);
}


/****************************************************************************/


void NoMemory(void)
{
    fprintf(stderr,"ADX Error: Unable to allocate memory\n");
}


/****************************************************************************/


static int32 GetLine(void)
{
int32 len;
int32 numTabs;

    errno = 0;
    lineNum++;

    inputLine = lineBuffer;
    len       = 0;

    if (fgets(lineBuffer,sizeof(lineBuffer),file))
    {
        validAutodocLine = FALSE;
        if ((strncmp(lineBuffer,"|||",3) == 0)
         || (strncmp(lineBuffer,"\\\\\\",3) == 0)
         || (strncmp(lineBuffer," |||",4) == 0))
        {
            numTabs = 0;
            len     = 3;

            if (lineBuffer[0] == ' ')
                len++;

            while (lineBuffer[len])
            {
                if (lineBuffer[len] == '\t')
                    numTabs++;

                len++;
            }

            do
            {
                len--;
                if (len < 0)
                    break;
            }
            while (isspace(lineBuffer[len]));

            len++;
            lineBuffer[len] = 0;

            if (parsingAutodoc)
            {
                if (lineBuffer[0] == ' ')
                {
                    if (lineBuffer[4] == '\t')
                    {
                        inputLine  = &lineBuffer[5];
                        len       -= 5;
                    }
                    else if (lineBuffer[4] == ' ')
                    {
                        ParseWarning("No TAB found after |||");
                        inputLine  = &lineBuffer[5];
                        len       -= 5;
                    }
                    else
                    {
                        inputLine  = &lineBuffer[4];
                        len       -= 4;
                    }
                }
                else
                {
                    if (lineBuffer[3] == '\t')
                    {
                        inputLine  = &lineBuffer[4];
                        len       -= 4;
                    }
                    else if (lineBuffer[3] == ' ')
                    {
                        ParseWarning("No TAB found after |||");
                        inputLine  = &lineBuffer[4];
                        len       -= 4;
                    }
                    else
                    {
                        inputLine  = &lineBuffer[3];
                        len       -= 3;
                    }
                }

                if (numTabs > 1)
                    ParseWarning("More than one TAB found after |||");
            }

            validAutodocLine = TRUE;
        }

        lineLen = len;

        return len;
    }

    if (errno)
    {
        IOError("Unable to read '%s' file",fileName);
        return -2;
    }

    return -1;
}


/****************************************************************************/


static Manual *GetManual(List *l, const char *name, Autodoc *ad)
{
Manual  *manual;
Manual  *old;
Section *section;

    manual = (Manual *)FindNamedNode(l,name);
    if (manual)
    {
        if (ad)
        {
            if (!manual->mn_Autodoc)
            {
                manual->mn_Autodoc = ad;
            }
            else
            {
                ParseError("Second description of manual '%s' found",manual->mn_Link.n_Name);
                manual = NULL;
            }
        }
    }
    else
    {
        manual = (Manual *)AllocNode(sizeof(Manual),name);
        if (manual)
        {
            manual->mn_Autodoc  = ad;
            manual->mn_FullName = NULL;
            PrepList(&manual->mn_Chapters);
            AddTail(l,(Node *)manual);
        }
        else
        {
            NoMemory();
        }
    }

    if (manual && ad)
    {
        section = (Section *)FindNamedNode(&ad->ad_Sections,"Full Name");
        if (section)
        {
            manual->mn_FullName = (char *)((Line *)FirstNode(&section->s_Lines))->ln_Data;

            RemNode((Node *)manual);
            ScanList(l,old,Manual)
            {
                if (strcasecmp(old->mn_FullName,manual->mn_FullName) >= 0)
                    break;
            }
            InsertNodeBefore((Node *)old, (Node *)manual);
        }
        else
        {
            ParseError("No 'Full Name' section found in manual description autodoc");
            manual = NULL;
        }
    }

    return manual;
}


/****************************************************************************/


static Chapter *GetChapter(List *l, const char *name, Autodoc *ad)
{
Chapter *chapter;
Chapter *old;
Section *section;

    chapter = (Chapter *)FindNamedNode(l,name);
    if (chapter)
    {
        if (ad)
        {
            if (!chapter->ch_Autodoc)
            {
                chapter->ch_Autodoc = ad;
            }
            else
            {
                ParseError("Second descriptions of chapter '%s' found",chapter->ch_Link.n_Name);
                chapter = NULL;
            }
        }
    }
    else
    {
        chapter = (Chapter *)AllocNode(sizeof(Chapter),name);
        if (chapter)
        {
            chapter->ch_Link.n_Name[0] = toupper(chapter->ch_Link.n_Name[0]);
            chapter->ch_FullName   = chapter->ch_Link.n_Name;
            chapter->ch_ManualName = NULL;
            chapter->ch_Autodoc    = ad;
            PrepList(&chapter->ch_Groups);
            AddTail(l,(Node *)chapter);
        }
        else
        {
            NoMemory();
        }
    }

    if (chapter && ad)
    {
        section = (Section *)FindNamedNode(&ad->ad_Sections,"Full Name");
        if (section)
        {
            chapter->ch_FullName = (char *)((Line *)FirstNode(&section->s_Lines))->ln_Data;

            RemNode((Node *)chapter);
            ScanList(l,old,Chapter)
            {
                if (strcasecmp(old->ch_FullName,chapter->ch_FullName) >= 0)
                    break;
            }
            InsertNodeBefore((Node *)old, (Node *)chapter);
        }
        else
        {
            ParseError("No 'Full Name' section found in chapter description autodoc");
            chapter = NULL;
        }

        section = (Section *)FindNamedNode(&ad->ad_Sections,"Manual");
        if (section)
        {
            chapter->ch_ManualName = (char *)((Line *)FirstNode(&section->s_Lines))->ln_Data;
        }
        else
        {
            ParseError("No 'Manual' section found in chapter description autodoc");
            chapter = NULL;
        }
    }

    return chapter;
}


/****************************************************************************/


static Group *GetGroup(Chapter *chapter, char *name)
{
Group *group;

    if (name == NULL)
        name = "";

    group = (Group *)FindNamedNode(&chapter->ch_Groups,name);
    if (!group)
    {
        group = (Group *)AllocNode(sizeof(Group),name);
        if (group)
        {
            PrepList(&group->gr_Autodocs);
            InsertNodeAlpha(&chapter->ch_Groups,(Node *)group);
        }
        else
        {
            NoMemory();
        }
    }

    return group;
}


/****************************************************************************/


static int32 ParseDescription(Autodoc *ad)
{
uint32   start;
int32    len;
Line    *line;

    len = lineLen;
    if (len < 0)
    {
        if (len == -1)
        {
            ParseError("Incomplete autodoc, no short description found");
            return 0;
        }

        return len;
    }

    if (!validAutodocLine)
    {
        ParseError("Incomplete autodoc, no short description found");
        return -1;
    }

    while (TRUE)
    {
        if (len == 0)
            break;

        start = 0;
        while (isspace(inputLine[start]))
            start++;

        line = (Line *)malloc(sizeof(Line) + len - start + 1);
        if (line)
        {
            line->ln_Data = (char *)&line[1];
            strcpy(line->ln_Data,&inputLine[start]);
            AddTail(&ad->ad_Description,(Node *)line);
        }
        else
        {
            NoMemory();
            return -1;
        }

        len = GetLine();
        if (len < 0)
        {
            if (len == -1)
                break;

            return len;
        }

        if (!validAutodocLine)
        {
            ParseError("Autodoc incomplete, nothing found after short description");
            return -1;
        }
    }

    if (IsEmptyList(&ad->ad_Description))
    {
        ParseError("Autodoc incomplete, no text for short description found");
        return -1;
    }

    return 0;
}


/*****************************************************************************/


static int32 ParseSections(List *sectionList, List *defines)
{
uint32      i, j;
int32       len;
bool        skipEmptyLines;
Line       *line;
Section    *section;
bool        skip;
List        ifs;
IfBlock    *ifb;
Define     *def;
FormatType  formatType;
char       *ptr;
uint32      start;

    skipEmptyLines = TRUE;
    section        = NULL;
    skip           = FALSE;
    formatType     = FT_NORMAL;
    PrepList(&ifs);

    while (TRUE)
    {
        len = GetLine();
        if (len < 0)
        {
            if (len == -1)
                return 0;

            return len;
        }

        if (!validAutodocLine)
            break;

        if ((len == 0) && skipEmptyLines)
            continue;

        i = 0;
        while (inputLine[i] == ' ')
            i++;

        if ((i == 0) && (len != 0) && (inputLine[i] != '\t'))
        {
            /* we've hit a "preprocessor" directive */
            if ((strncasecmp(inputLine,"ifdef",5) == 0) &&
                ((inputLine[5] == ' ') || (inputLine[5] == '\t')))
            {
                i = 6;
                while ((inputLine[i] == ' ') || (inputLine[i] == '\t'))
                    i++;

                ifb = (IfBlock *)malloc(sizeof(IfBlock));
                if (!ifb)
                {
                    NoMemory();
                    return -1;
                }

                ifb->ib_Skip = TRUE;
                ifb->ib_Else = FALSE;

                ScanList(defines,def,Define)
                {
                    if (strcasecmp(def->def_Name,&inputLine[i]) == 0)
                    {
                        ifb->ib_Skip = FALSE;
                        break;
                    }
                }

                AddTail(&ifs,(Node *)ifb);
                skip = ifb->ib_Skip;
            }
            else if ((strncasecmp(inputLine,"ifndef",6) == 0) &&
                     ((inputLine[6] == ' ') || (inputLine[6] == '\t')))
            {
                i = 6;
                while ((inputLine[i] == ' ') || (inputLine[i] == '\t'))
                    i++;

                ifb = (IfBlock *)malloc(sizeof(IfBlock));
                if (!ifb)
                {
                    NoMemory();
                    return -1;
                }

                ifb->ib_Skip = FALSE;
                ifb->ib_Else = FALSE;

                ScanList(defines,def,Define)
                {
                    if (strcasecmp(def->def_Name,&inputLine[i]) == 0)
                    {
                        ifb->ib_Skip = TRUE;
                        break;
                    }
                }

                AddTail(&ifs,(Node *)ifb);
                skip = ifb->ib_Skip;
            }
            else if (strcasecmp(inputLine,"else") == 0)
            {
                ifb = (IfBlock *)FirstNode(&ifs);
                if (IsNode(&ifs,(Node *)ifb))
                {
                    if (!ifb->ib_Else)
                    {
                        ifb->ib_Skip = !ifb->ib_Skip;
                        skip = ifb->ib_Skip;
                    }
                    else
                    {
                        ParseError("Second 'else' for one 'if' directive");
                        return -1;
                    }
                }
                else
                {
                    ParseError("'else' directive without a preceeding 'if'");
                    return -1;
                }
            }
            else if (strcasecmp(inputLine,"endif") == 0)
            {
                ifb = (IfBlock *)RemHead(&ifs);
                if (ifb)
                {
                    free(ifb);

                    ifb = (IfBlock *)FirstNode(&ifs);
                    if (IsNode(&ifs,(Node *)ifb))
                        skip = ifb->ib_Skip;
                    else
                        skip = FALSE;
                }
                else
                {
                    ParseError("'endif' directive without a preceeding 'if'");
                    return -1;
                }
            }
            else
            {
                ParseError("'%s' is not a valid directive",inputLine);
                return -1;
            }
        }
        else if (skip)
        {
            continue;
        }
        else if (i == 2)
        {
            /* we've hit a section header */

            if (section)
            {
                /* remove any trailing empty lines at the end of the previous section */
                while (TRUE)
                {
                    line = (Line *)RemTail(&section->s_Lines);
                    if (line == NULL)
                    {
                        if (section->s_Link.n_Name)
                            ParseWarning("Section '%s' was empty so it was removed",section->s_Link.n_Name);
                        else
                            ParseWarning("Section was empty so it was removed",section->s_Link.n_Name);

                        RemNode((Node *)section);
                        FreeNode(section);
                        break;
                    }

                    if (line->ln_Data[0] == 0)
                    {
                        free(line);
                    }
                    else
                    {
                        AddTail(&section->s_Lines,(Node *)line);
                        break;
                    }
                }
            }

            if (inputLine[i] != '-')
            {
                if ((strcasecmp(&inputLine[i],"Synopsis") == 0)
                 || (strcasecmp(&inputLine[i],"Example") == 0)
                 || (strcasecmp(&inputLine[i],"Examples") == 0))
                {
                    formatType = FT_PREFORMATTED;
                }
                else
                {
                    formatType = FT_NORMAL;
                }
            }

            ptr = strchr(&inputLine[i],'-');
            if (ptr)
            {
                j = (uint32)ptr - (uint32)inputLine;
                if (isspace(inputLine[j-1]))
                {
                    j--;
                    while (j > i)
                    {
                        if (!isspace(inputLine[j]))
                        {
                            j++;
                            break;
                        }
                        j--;
                    }
                    inputLine[j++] = 0;

                    while (TRUE)
                    {
                        while (isspace(inputLine[j]))
                            j++;

                        if (inputLine[j] == 0)
                            break;

                        start = j;
                        while (isgraph(inputLine[j]))
                            j++;

                        if (strncasecmp(&inputLine[start],"-preformatted",j-start) == 0)
                        {
                            formatType = FT_PREFORMATTED;
                        }
                        else if ((strncasecmp(&inputLine[start],"-enumerated",j-start) == 0)
                              || (strncasecmp(&inputLine[start],"-normal_format",j-start) == 0))
                        {
                            formatType = FT_NORMAL;
                        }
                        else
                        {
                            ParseError("Invalid option '%.*s' specified in section name",j-start,&inputLine[start]);
                            return -1;
                        }
                    }
                }
            }

            if (inputLine[i] != '-')
                section = (Section *)AllocNode(sizeof(Section), &inputLine[i]);
            else
                section = (Section *)AllocNode(sizeof(Section), NULL);

            if (!section)
            {
                NoMemory();
                return -1;
            }

            PrepList(&section->s_Lines);
            AddTail(sectionList,(Node *)section);
            section->s_Type = formatType;

            skipEmptyLines = TRUE;
        }
        else if ((i > 0) && (i < 4))
        {
            ParseError("Unknown line format");
            return -1;
        }
        else
        {
            if (section == NULL)
            {
                ParseError("No section name found");
                return -1;
            }

            skipEmptyLines = FALSE;

            if (len == 0)
            {
                line = (Line *)malloc(sizeof(Line));
                if (line)
                    line->ln_Data = "";
            }
            else
            {
                if (i > 4)
                    i = 4;

                line = (Line *)malloc(sizeof(Line) + len - i + 1);
                if (line)
                {
                    line->ln_Data = (char *)&line[1];
                    strcpy(line->ln_Data,&inputLine[i]);
                }
            }

            if (!line)
            {
                NoMemory();
                return -1;
            }

            AddTail(&section->s_Lines,(Node *)line);
        }
    }

    if (!IsListEmpty(&ifs))
    {
        ParseError("Unterminated ifdef/ifndef directive");
        return -1;
    }

    if (section == NULL)
    {
        ParseError("Empty autodoc detected");
        return -1;
    }
    else
    {
        /* remove any trailing empty lines at the end of the previous section */
        while (TRUE)
        {
            line = (Line *)RemTail(&section->s_Lines);
            if (line == NULL)
            {
                if (section->s_Link.n_Name)
                    ParseWarning("Section '%s' was empty so it was removed",section->s_Link.n_Name);
                else
                    ParseWarning("Section was empty so it was removed",section->s_Link.n_Name);

                RemNode((Node *)section);
                FreeNode(section);
                break;
            }

            if (line->ln_Data[0] == 0)
            {
                free(line);
            }
            else
            {
                AddTail(&section->s_Lines,(Node *)line);
                break;
            }
        }
    }

    return 0;
}


/****************************************************************************/


static Autodoc *ParseAutodoc(List *defines, const char *name,
                             List *visibleAliases, List *hiddenAliases)
{
Autodoc *ad;
int32    result;
Node    *n;
char     temp[128];

    ad = (Autodoc *)AllocNode(sizeof(Autodoc),name);
    if (ad)
    {
        PrepList(&ad->ad_HiddenAliases);
        PrepList(&ad->ad_VisibleAliases);
        PrepList(&ad->ad_Sections);
        PrepList(&ad->ad_Description);

        while (n = RemHead(visibleAliases))
            AddTail(&ad->ad_VisibleAliases, n);

        while (n = RemHead(hiddenAliases))
            AddTail(&ad->ad_HiddenAliases, n);

        sprintf(temp, "%sVA", name);
        n = AllocNode(sizeof(Node), temp);
        if (n)
        {
            AddTail(&ad->ad_HiddenAliases, n);

            result = ParseDescription(ad);
            if (result >= 0)
            {
                result = ParseSections(&ad->ad_Sections,defines);
            }
        }
        else
        {
            NoMemory();
        }

        if (result < 0)
        {
            FreeNode(ad);
            ad = NULL;
        }
    }
    else
    {
        NoMemory();
    }

    return ad;
}


/****************************************************************************/


static int32 ExtractAutodocs(List *manuals, List *chapters, const char *name,
                             List *defines, bool doPrivate, bool doInternal)
{
Manual  *manual;
Chapter *chapter;
Group   *group;
Autodoc *ad;
uint32   i, start, end;
int32    len;
int32    result;
bool     disabled;
bool     isClassDescription;
bool     isManualDescription;
bool     isPrivate;
bool     isInternal;
char    *autodocName;
char    *groupName;
char    *className;
char     temp[200];
List     visibleAliases;
List     hiddenAliases;
Node    *n;

    fileName       = name;
    lineNum        = 0;
    result         = 0;
    parsingAutodoc = FALSE;

    file = fopen(name,"r");
    if (file)
    {
        VERBOSE(("ADX: Looking in '%s' for autodocs\n",name));

        do
        {
            len = GetLine();
            if (len < 0)
            {
                if (len == -1)
                    result = 0;
                else
                    result = len;

                break;
            }

            if ((strncmp(inputLine,"|||\tAUTODOC ",12) == 0)
             || (strncmp(inputLine,"\\\\\\\tAUTODOC ",12) == 0)
             || (strncmp(inputLine," |||\tAUTODOC ",13) == 0))
            {
                parsingAutodoc = TRUE;

                disabled            = FALSE;
                isClassDescription  = FALSE;
                isManualDescription = FALSE;
                isPrivate           = FALSE;
                isInternal          = FALSE;
                autodocName         = NULL;
                className           = NULL;
                groupName           = NULL;
                PrepList(&visibleAliases);
                PrepList(&hiddenAliases);

                i = 12;
                while (result >= 0)
                {
                    while (isspace(inputLine[i]))
                        i++;

                    if (inputLine[i] == 0)
                    {
                        len = GetLine();
                        if (len < 0)
                        {
                            if (len == -1)
                                result = 0;
                            else
                                result = len;

                            break;
                        }

                        i = 0;
                        while (isspace(inputLine[i]))
                            i++;

                        if (inputLine[i] != '-')
                        {
                            /* no more options! */
                            break;
                        }
                    }

                    start = i;
                    while (isgraph(inputLine[i]))
                        i++;

                    if (strncasecmp(&inputLine[start],"-disabled",i-start) == 0)
                    {
                        disabled = TRUE;
                    }
                    else if (strncasecmp(&inputLine[start],"-private",i-start) == 0)
                    {
                        isPrivate = TRUE;
                    }
                    else if (strncasecmp(&inputLine[start],"-internal",i-start) == 0)
                    {
                        isInternal = TRUE;
                    }
                    else if (strncasecmp(&inputLine[start],"-public",i-start) == 0)
                    {
                        /* just ignore this deprecated option */
                    }
                    else if (strncasecmp(&inputLine[start],"-name",i-start) == 0)
                    {
                        if (autodocName)
                        {
                            ParseError("Multiple names given for autodoc");
                            result = -1;
                        }
                        else
                        {
                            while (isspace(inputLine[i]))
                                i++;

                            start = i;
                            while (isgraph(inputLine[i]))
                                i++;
                            end = i;

                            autodocName = malloc(end - start + 1);
                            if (autodocName)
                            {
                                strncpy(autodocName, &inputLine[start], end - start);
                                autodocName[end - start] = 0;
                            }
                            else
                            {
                                NoMemory();
                                result = -1;
                            }
                        }
                    }
                    else if (strncasecmp(&inputLine[start],"-class",i-start) == 0)
                    {
                        if (className)
                        {
                            ParseError("Multiple classes given for autodoc");
                            result = -1;
                        }
                        else
                        {
                            while (isspace(inputLine[i]))
                                i++;

                            start = i;
                            while (isgraph(inputLine[i]))
                                i++;
                            end = i;

                            className = malloc(end - start + 1);
                            if (className)
                            {
                                strncpy(className, &inputLine[start], end - start);
                                className[end - start] = 0;
                            }
                            else
                            {
                                NoMemory();
                                result = -1;
                            }
                        }
                    }
                    else if (strncasecmp(&inputLine[start],"-group",i-start) == 0)
                    {
                        if (groupName)
                        {
                            ParseError("Multiple group names given for autodoc");
                            result = -1;
                        }
                        else
                        {
                            while (isspace(inputLine[i]))
                                i++;

                            start = i;
                            while (isgraph(inputLine[i]))
                                i++;
                            end = i;

                            groupName = malloc(end - start + 1);
                            if (groupName)
                            {
                                strncpy(groupName, &inputLine[start], end - start);
                                groupName[end - start] = 0;
                            }
                            else
                            {
                                NoMemory();
                                result = -1;
                            }
                        }
                    }
                    else if (strncasecmp(&inputLine[start],"-hidden_alias",i-start) == 0)
                    {
                        while (isspace(inputLine[i]))
                            i++;

                        start = i;
                        while (isgraph(inputLine[i]))
                            i++;
                        end = i;

                        strncpy(temp, &inputLine[start], end - start);
                        temp[end - start] = 0;

                        n = AllocNode(sizeof(Node), temp);
                        if (n)
                        {
                            AddTail(&hiddenAliases, n);
                        }
                        else
                        {
                            NoMemory();
                            result = -1;
                        }
                    }
                    else if (strncasecmp(&inputLine[start],"-visible_alias",i-start) == 0)
                    {
                        while (isspace(inputLine[i]))
                            i++;

                        start = i;
                        while (isgraph(inputLine[i]))
                            i++;
                        end = i;

                        strncpy(temp, &inputLine[start], end - start);
                        temp[end - start] = 0;

                        n = AllocNode(sizeof(Node), temp);
                        if (n)
                        {
                            AddTail(&visibleAliases, n);
                        }
                        else
                        {
                            NoMemory();
                            result = -1;
                        }
                    }
                    else if (strncasecmp(&inputLine[start],"-class_description",i-start) == 0)
                    {
                        isClassDescription = TRUE;
                    }
                    else if (strncasecmp(&inputLine[start],"-manual_description",i-start) == 0)
                    {
                        isManualDescription = TRUE;
                    }
                    else
                    {
                        ParseError("Unknown token '%.*s' in autodoc header",i-start,&inputLine[start]);
                        result = -1;
                    }
                }

                if (result >= 0)
                {
                    if (isClassDescription)
                    {
                        if (autodocName)
                        {
                            ad = ParseAutodoc(defines, autodocName, &visibleAliases, &hiddenAliases);
                            if (ad)
                            {
                                chapter = GetChapter(chapters, autodocName, ad);
                                if (!chapter)
                                {
                                    result = -1;
                                }
                            }
                            else
                            {
                                result = -1;
                            }
                        }
                        else
                        {
                            ParseError("No name specified in autodoc header");
                            result = -1;
                        }
                    }
                    else if (isManualDescription)
                    {
                        if (autodocName)
                        {
                            ad = ParseAutodoc(defines, autodocName, &visibleAliases, &hiddenAliases);
                            if (ad)
                            {
                                manual = GetManual(manuals, autodocName, ad);
                                if (!manual)
                                {
                                    result = -1;
                                }
                            }
                            else
                            {
                                result = -1;
                            }
                        }
                        else
                        {
                            ParseError("No name specified in autodoc header");
                            result = -1;
                        }
                    }
                    else if (autodocName && className)
                    {
                        VERBOSE(("  Found autodoc for '%s'", autodocName));

                        if (disabled)
                        {
                            VERBOSE(("...ignored (disabled)\n"));
                        }
                        else if ((!isPrivate || doPrivate) && (!isInternal || doInternal))
                        {
                            VERBOSE(("\n"));

                            ad = ParseAutodoc(defines, autodocName, &visibleAliases, &hiddenAliases);
                            if (ad)
                            {
                                ad->ad_Private = isPrivate;

                                chapter = GetChapter(chapters, className, NULL);
                                if (chapter)
                                {
                                    group = GetGroup(chapter, groupName);

                                    if (group)
                                    {
                                        InsertNodeAlpha(&group->gr_Autodocs,(Node *)ad);
                                    }
                                    else
                                    {
                                        result = -1;
                                    }
                                }
                                else
                                {
                                    result = -1;
                                }
                            }
                            else
                            {
                                result = -1;
                            }
                        }
                        else
                        {
                            if (isPrivate && !doPrivate)
                                VERBOSE(("...ignored (private)\n"));
                            else
                                VERBOSE(("...ignored (internal)\n"));
                        }
                    }
                    else
                    {
                        if (!autodocName)
                            ParseError("No name specified in autodoc header");
                        else
                            ParseError("No class specified in autodoc header");

                        result = -1;
                    }
                }
                parsingAutodoc = FALSE;

                free(autodocName);
                free(className);
                free(groupName);

                while (n = RemHead(&visibleAliases))
                    FreeNode(n);

                while (n = RemHead(&hiddenAliases))
                    FreeNode(n);
            }
        }
        while (result >= 0);

        fclose(file);
    }
    else
    {
        IOError("Couldn't open '%s' for input\n",name);
        result = -1;
    }

    return result;
}


/*****************************************************************************/


static void FreeManualList(List *manuals)
{
Manual  *manual;
Chapter *chapter;
Group   *group;
Autodoc *ad;
Section *section;
Line    *line;
Node    *n;

    while (manual = (Manual *)RemHead(manuals))
    {
        while (chapter = (Chapter *)RemHead(&manual->mn_Chapters))
        {
            while (group = (Group *)RemHead(&chapter->ch_Groups))
            {
                while (ad = (Autodoc *)RemHead(&group->gr_Autodocs))
                {
                    while (section = (Section *)RemHead(&ad->ad_Sections))
                    {
                        while (line = (Line *)RemHead(&section->s_Lines))
                        {
                            free(line);
                        }
                        FreeNode(section);
                    }

                    while (line = (Line *)RemHead(&ad->ad_Description))
                    {
                        free(line);
                    }

                    while (n = RemHead(&ad->ad_VisibleAliases))
                        FreeNode(n);

                    while (n = RemHead(&ad->ad_HiddenAliases))
                        FreeNode(n);

                    FreeNode(ad);
                }
                FreeNode(group);
            }
            FreeNode(chapter->ch_Autodoc);
            FreeNode(chapter);
        }
        FreeNode(manual->mn_Autodoc);
        FreeNode(manual);
    }
}


/*****************************************************************************/


static int32 PairChaptersToManuals(List *manuals, List *chapters)
{
Manual  *manual;
Manual  *next;
Chapter *chapter;
Chapter *old;
int32    result;

    while (chapter = (Chapter *)RemHead(chapters))
    {
        if (IsEmptyList(&chapter->ch_Groups))
        {
            free(chapter->ch_Autodoc);
            free(chapter);
        }
        else
        {
            if (chapter->ch_ManualName)
            {
                manual = (Manual *)FindNamedNode(manuals,chapter->ch_ManualName);
                if (!manual)
                {
                    fprintf(stderr,"ADX Error: The description for chapter '%s' specifies\n",chapter->ch_Link.n_Name);
                    fprintf(stderr,"           the '%s' manual, and there was no description\n",chapter->ch_ManualName);
                    fprintf(stderr,"           found for this manual\n");
                    result = -1;
                    break;
                }
            }
            else
            {
                manual = GetManual(manuals,"LC",NULL);
                if (manual)
                {
                    manual->mn_FullName = "*** Lonely Chapters ***";
                }
                else
                {
                    result = -1;
                    break;
                }
            }

            ScanList(&manual->mn_Chapters,old,Chapter)
            {
                if (strcasecmp(old->ch_FullName,chapter->ch_FullName) >= 0)
                    break;
            }
            InsertNodeBefore((Node *)old, (Node *)chapter);
        }
    }

    for (manual = (Manual *)FirstNode(manuals); IsNode(manuals,manual); manual = next)
    {
        next = (Manual *)NextNode(manual);

        if (IsEmptyList(&manual->mn_Chapters))
        {
            RemNode((Node *)manual);
            free(manual->mn_Autodoc);
            free(manual);
        }
    }

    return result;
}


/****************************************************************************/


static void PrintUsage(const char* progName)
{
    printf("Usage: %s [-ascii <ascii output directory>]\n", progName);
    printf("           [-man <man output directory>]\n");
    printf("           [-html <html output directory>]\n");
    printf("           [-411 <411 output directory>]\n");
    printf("           [-print <print-ready output directory>]\n");
    printf("           [-include_dir <reference include file directory>\n");
    printf("           [-private]\n");
    printf("           [-internal]\n");
    printf("           {-D <symbol name>}\n");
    printf("           [-output_platform <mac | unix | pc>]\n");
    printf("           [-verbose]\n");
    printf("           <source file1> [source files]\n");
}


/****************************************************************************/


int main(int argc, char **argv)
{
uint32   i, j;
List     manuals;
List     chapters;
int32    result;
List     defines;
Define  *def;
Node    *node;

    if (argc == 1)
    {
        PrintUsage(argv[0]);
        return 0;
    }

    PrepList(&includeDirs);
    PrepList(&manuals);
    PrepList(&chapters);
    PrepList(&defines);
    extractPrivate  = FALSE;
    extractInternal = FALSE;
    outputPlatform  = PLATFORM_MAC;
    verbose         = FALSE;

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if ((strcmp(argv[i],"-help") == 0) ||
                (strcmp(argv[i],"-h") == 0) ||
                (strcmp(argv[i],"-?") == 0))
            {
                PrintUsage(argv[0]);
                return 0;
            }
            else if (strcmp(argv[i],"-private") == 0)
            {
                extractPrivate = TRUE;
            }
            else if (strcmp(argv[i],"-internal") == 0)
            {
                extractInternal = TRUE;
            }
            else if (strcmp(argv[i],"-verbose") == 0)
            {
                verbose = TRUE;
            }
            else if (strcmp(argv[i],"-output_platform") == 0)
            {
                i++;
                if (i == argc)
                {
                    printf("ADX Error: No platform output type specified with the -output_platform option\n");
                    return -1;
                }

                if (strcasecmp(argv[i],"mac") == 0)
                {
                    outputPlatform = PLATFORM_MAC;
                }
                else if (strcasecmp(argv[i],"pc") == 0)
                {
                    outputPlatform = PLATFORM_PC;
                }
                else if (strcasecmp(argv[i],"unix") == 0)
                {
                    outputPlatform = PLATFORM_UNIX;
                }
                else
                {
                    printf("ADX Error: Illegal output platform, must be mac, pc, or unix\n");
                    return -1;
                }
            }
            else if (strcmp(argv[i],"-include_dir") == 0)
            {
                i++;
                if (i == argc)
                {
                    printf("ADX Error: No directory name for the '-include_dir' option\n",argv[i-1]);
                    return -1;
                }

                node = AllocNode(sizeof(Node),argv[i]);
                if (!node)
                {
                    NoMemory();
                    return -1;
                }
                AddTail(&includeDirs, node);
            }
            else if (strcmp(argv[i],"-D") == 0)
            {
                i++;
                if (i == argc)
                {
                    printf("ADX Error: No symbol name for the '-D' option\n",argv[i-1]);
                    return -1;
                }

                def = (Define *)AllocNode(sizeof(Define),argv[i]);
                if (!def)
                {
                    NoMemory();
                    return -1;
                }
                AddTail(&defines,(Node *)def);
            }
            else
            {
                j = 0;
                while (TRUE)
                {
                    if (outputTypes[j].ot_TypeName == NULL)
                    {
                        printf("ADX Error: '%s' is an unknown option\n",argv[i]);
                        return -1;
                    }

                    if (strcmp(outputTypes[j].ot_TypeName,&argv[i][1]) == 0)
                    {
                        i++;
                        if (i == argc)
                        {
                            printf("ADX Error: No argument for the '%s' option\n",argv[i-1]);
                            return -1;
                        }

                        outputTypes[j].ot_OutputDir = argv[i];
#ifdef macintosh
						/* strip trailing colon, if any */
						if (strlen(argv[i]) > 1 && argv[i][strlen(argv[i])-1] == ':')
							argv[i][strlen(argv[i])-1] = '\0';
#endif
                        break;
                    }
                    j++;
                }
            }
            argv[i] = NULL;
        }
    }

    for (i = 1; i < argc; i++)
    {
        if (argv[i] && (argv[i][0] != '-'))
        {
            result = ExtractAutodocs(&manuals,&chapters,argv[i],&defines,extractPrivate,extractInternal);
            if (result < 0)
                break;
        }
    }

    if (result >= 0)
    {
        result = PairChaptersToManuals(&manuals,&chapters);
        if (result >= 0)
        {
            j = 0;
            while (outputTypes[j].ot_TypeName)
            {
                if (outputTypes[j].ot_OutputDir)
                {
                    result = (* outputTypes[j].ot_OutputFunc)(&manuals,outputTypes[j].ot_OutputDir);
                    if (result < 0)
                        break;
                }
                j++;
            }
        }
    }

    if (result > 0)
        result = 0;
    else if (result < 0)
        result = 1;

    FreeManualList(&manuals);

    while (node = RemHead(&defines))
        FreeNode(node);

    while (node = RemHead(&includeDirs))
        FreeNode(node);

    return (int)result;
}
