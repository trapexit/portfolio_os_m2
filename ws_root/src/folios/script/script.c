/* @(#) script.c 96/09/04 1.28 */

#include <kernel/types.h>
#include <kernel/tags.h>
#include <kernel/mem.h>
#include <kernel/item.h>
#include <kernel/msgport.h>
#include <kernel/operror.h>
#include <kernel/kernel.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/fileio.h>
#include <misc/script.h>
#define LOAD_FOR_PORTFOLIO
#include <loader/loadererr.h>
#include <loader/loader3do.h>
#include <string.h>
#include <stdio.h>
#include "commands.h"


/*****************************************************************************/


static Err ExecBuiltIn(ScriptContext *sc, ScriptCommand cmds[], char *command,
                       char *cmdLine, bool *found)
{
uint32  index;
char    local[32];
uint32  i,j;
char   *name;
Err     result;
char    old;

    if (cmds)
    {
        index = 0;
        while (TRUE)
        {
            name = cmds[index].sc_Name;
            if (!name)
                break;

            i = 0;
            while (name[i])
            {
                j = 0;
                while (name[i] && (name[i] != ','))
                    local[j++] = name[i++];

                if (name[i])
                    i++;

                local[j] = 0;

                if (strcasecmp(local,command) == 0)
                {
                    *found = TRUE;

                    if (*cmdLine == '"')
                    {
                        /* strip away the quotes */
                        cmdLine++;
                        i = 0;
                        while ((cmdLine[i] != '"') && (cmdLine[i]))
                            i++;
                        old = cmdLine[i];
                        cmdLine[i] = 0;

                        result = (* cmds[index].sc_Command)(sc,cmdLine);

                        cmdLine[i] = old;
                        return result;
                    }
                    return (* cmds[index].sc_Command)(sc,cmdLine);
                }
            }

            index++;
        }
    }

    *found = FALSE;

    return -1;
}


/*****************************************************************************/


static const TagArg modTags[] =
{
    MODULE_TAG_MUST_BE_SIGNED, (TagData)TRUE,
    TAG_END
};

static Err RunProgram(const char *filename, const char *cmdline,
                      bool background, bool mustBeSigned)
{
Err     result;
Item    replyPort;
Item    msg;
Item    module;
Module *mptr;

    module = OpenModule(filename, OPENMODULE_FOR_TASK,
                        (mustBeSigned ? modTags : NULL));
    if (module < 0)
        return module;

    mptr = (Module *)LookupItem(module);
    if (background)
    {
        result = CreateTaskVA(module, mptr->n.n_Name,
                              CREATETASK_TAG_CMDSTR, cmdline,
                              TAG_END);
        if (result >= 0)
            result = 0;

	CloseModule(module);
    }
    else
    {
        replyPort = result = CreateMsgPort(NULL, 0, 0);
        if (replyPort >= 0)
        {
            result = CreateTaskVA(module, mptr->n.n_Name,
                                  CREATETASK_TAG_CMDSTR,       cmdline,
                                  CREATETASK_TAG_MSGFROMCHILD, replyPort,
                                  TAG_END);

	    CloseModule(module);

            if (result >= 0)
            {
                msg = result = WaitPort(replyPort, 0);
                if (msg >= 0)
                {
                    result = MESSAGE(msg)->msg_Result;
                    DeleteMsg(msg);
                }
            }
            DeleteMsgPort(replyPort);
        }
    }

    return result;
}


/*****************************************************************************/


typedef struct Script
{
    struct Script *spt_NextScript;
    char          *spt_NextLine;
    char          *spt_End;
    char           spt_Script[1];
} Script;


/*****************************************************************************/


static Err LoadScript(Script **script, const char *scriptName)
{
RawFile  *file;
Err       result;
FileInfo  fileInfo;
Script   *scpt;

    scpt = NULL;

    result = OpenRawFile(&file, scriptName, FILEOPEN_READ);
    if (result >= 0)
    {
        result = GetRawFileInfo(file, &fileInfo, sizeof(fileInfo));
        if (result >= 0)
        {
            scpt = (Script *)AllocMem(sizeof(Script) + fileInfo.fi_ByteCount, MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL);
            if (scpt)
            {
                result = ReadRawFile(file, scpt->spt_Script, fileInfo.fi_ByteCount);
                if (result >= 0)
                {
		    scpt->spt_NextLine   = scpt->spt_Script;
		    scpt->spt_End        = scpt->spt_Script + fileInfo.fi_ByteCount;
		    scpt->spt_NextScript = NULL;
		}
		else
		{
		    FreeMem(scpt, TRACKED_SIZE);
		    scpt = NULL;
		}
            }
            else
            {
                result = NOMEM;
            }
        }
        CloseRawFile(file);
    }

    *script = scpt;

    return result;
}


/*****************************************************************************/


static char *GetNextLine(Script **scripts)
{
Script *ns = *scripts;
char   *nl;
char   *el;

    while (ns)
    {
	nl = ns->spt_NextLine;
	el = ns->spt_End;
	if (nl >= el)
	{
	    *scripts = ns->spt_NextScript;
	    FreeMem(ns, TRACKED_SIZE);
	    ns = *scripts;
	    continue;
	}

	while (nl < el)
	{
	    if ((*nl == '\n') || (*nl == '\r'))
		break;
	    nl++;
	}
	*nl = '\0';
	el = ns->spt_NextLine;
	ns->spt_NextLine = nl+1;
	return el;
    }

    return NULL;
}


/*****************************************************************************/


static const TagArg filesystemSearch[] =
{
    FILESEARCH_TAG_SEARCH_FILESYSTEMS,  (TagData) DONT_SEARCH_UNBLESSED,
    TAG_END
};

static Err ExecuteLine(const char *originalLine, ScriptContext *sc, int32 *pStatus)
{
Err     result;
int32   status;
char   *pokeSpace;
char   *cmdArgs;
char   *cp;
char   *temp;
bool    found;
bool    background;
bool    mustBeSigned;
char    filename[FILESYSTEM_MAX_PATH_LEN];
char    local[256];
char   *line;
Script *scripts;

    strncpy(local, originalLine, sizeof(local));
    local[sizeof(local) - 1] = 0;
    line = local;

    scripts = NULL;
    do
    {
        /* skip leading spaces and tabs on the command-line */
        while ((*line == ' ') || (*line == '\t'))
            line++;

        background   = sc->sc_DefaultBackgroundMode;
        mustBeSigned = sc->sc_MustBeSigned;

        /* remove comments... # also forces foreground execution */
        cp = strchr(line, '#');
        if (cp)
        {
            *cp = '\0';
            background = FALSE;
        }

        /* should we force background launching? */
        cp = strchr(line,'&');
        if (cp)
        {
            *cp = '\0';
            background = TRUE;
        }

        cp = line;
        while (*cp && (*cp != ' ') && (*cp != '\t'))
            cp++;

        if (cp == line)
        {
            /* empty command-line */
            status = result = 0;
            continue;
        }

        pokeSpace = NULL;
        if (*cp == ' ')
        {
            pokeSpace = cp;
            *cp = '\0';
            while (*++cp == ' ');
        }

        cmdArgs = cp;

        ScavengeMem();

        /* is it a built-in command? */
        result = ExecBuiltIn(sc, builtIns, line, cmdArgs, &found);
        if (found)
        {
            status = result;
            continue;
        }

        strcpy(filename, line);
        if (pokeSpace)
            *pokeSpace = ' ';

        result = RunProgram(filename, line, background, mustBeSigned);
        if ((result == FILE_ERR_NOFILE)
         || (result == FILE_ERR_NOFILESYSTEM))
        {
            /* if the file couldn't be found using the supplied path,
             * try to find the file in the Programs directory, for shell
             * convenience.
             *
             * The FILE_ERR_NOFILESYSTEM error will occur if the user has done
             * something like:
             *
             * /remote> cd /
             * /remote> ls
             */

            temp = AllocMem(strlen(filename) + 20, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE);
            if (temp)
            {
            char pathname[FILESYSTEM_MAX_PATH_LEN];

                sprintf(temp, "System.m2/Programs/%s", filename);
                result = FindFileAndIdentify(pathname, sizeof(pathname), temp, filesystemSearch);
                if (result >= 0)
                    result = RunProgram(pathname, line, background, mustBeSigned);

                FreeMem(temp, TRACKED_SIZE);
            }
        }
        else if (result == FILE_ERR_NOTAFILE || result == LOADER_ERR_BADFILE)
        {
        Err cdresult;

            if (pokeSpace)
                *pokeSpace = '\0';

            cdresult = ExecBuiltIn(sc, builtIns, "cd", line, &found);
            if (cdresult >= 0)
            {
                status = result = cdresult;
                continue;
            }
        }

        if (result == LOADER_ERR_BADFILE)
        {
        Script *ts;

            if (pokeSpace)
                *pokeSpace = '\0';

            result = LoadScript(&ts, line);
            if (result >= 0)
            {
                ts->spt_NextScript = scripts;
                scripts = ts;
            }
        }
        status = result;
    }
    while ((result >= 0) && (line = GetNextLine(&scripts)));

    while (scripts)
    {
        Script *ts = scripts->spt_NextScript;
        FreeMem(scripts, TRACKED_SIZE);
        scripts = ts;
    }

    if (pStatus)
        *pStatus = status;

    return result;
}


/*****************************************************************************/


static Err ParseTags(ScriptContext *sc, const TagArg *tags)
{
TagArg *tag;

    while ((tag = NextTagArg(&tags)) != NULL)
    {
        switch (tag->ta_Tag)
        {
            case SCRIPT_TAG_BACKGROUND_MODE : sc->sc_DefaultBackgroundMode = (tag->ta_Arg != 0) ? TRUE : FALSE;
                                              break;

	    case SCRIPT_TAG_MUST_BE_SIGNED  : sc->sc_MustBeSigned = (tag->ta_Arg != 0) ? TRUE : FALSE;
                                              break;

	    case SCRIPT_TAG_CONTEXT         : break;

            default                         : return BADTAG;
        }
    }

    return 0;
}


/*****************************************************************************/


Err ExecuteCmdLine(const char *cmdLine, Err *status, const TagArg *tags)
{
Err            result;
ScriptContext  ctx;
ScriptContext *sc;

    sc = GetTagArg(tags,SCRIPT_TAG_CONTEXT,&ctx);
    if (sc == NULL)
    {
        memset(&ctx,0,sizeof(ctx));
        sc = &ctx;
    }

    result = ParseTags(sc,tags);
    if (result >= 0)
        result = ExecuteLine(cmdLine, sc, status);

    return result;
}


/*****************************************************************************/


Err CreateScriptContext(ScriptContext **sc, const TagArg *tags)
{
    *sc = AllocMem(sizeof(ScriptContext),MEMTYPE_ANY | MEMTYPE_FILL);
    if (*sc)
        return ParseTags(*sc,tags);

    return NOMEM;
}


/*****************************************************************************/


Err DeleteScriptContext(ScriptContext *sc)
{
    FreeMem(sc,sizeof(ScriptContext));
    return 0;
}
