/* @(#) shell.c 96/10/18 1.100 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/time.h>
#include <kernel/kernel.h>
#include <kernel/task.h>
#include <kernel/debug.h>
#include <kernel/mem.h>
#include <kernel/devicecmd.h>
#include <loader/loader3do.h>
#include <misc/date.h>
#include <misc/batt.h>
#include <file/filefunctions.h>
#include <file/fileio.h>
#include <misc/script.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errors.h"


/*****************************************************************************/


#undef BUILD_SERIAL

#ifndef BUILD_DEBUGGER
#define BUILD_SERIAL
#endif


/*****************************************************************************/


/* if you change this name, you should also change ValidateSignals() in the
 * kernel's signal.c source.
 */
#define CONSOLE_THREAD_NAME "Shell Console"

/* from the operator */
extern void ProvideItemServices(Item serverPort);

static const TagArg searchForFile[] =
{
    FILESEARCH_TAG_SEARCH_FILESYSTEMS, (TagData) DONT_SEARCH_UNBLESSED,
    TAG_END
};


/*****************************************************************************/


static void SetSystemClock(void)
{
Item          timer;
IOInfo        ioInfo;
TimeVal       tv;
GregorianDate gd;

    timer = CreateTimerIOReq();
    if (timer >= 0)
    {
        if (OpenDateFolio() >= 0)
        {
            if (OpenBattFolio() >= 0)
            {
                if (ReadBattClock(&gd) >= 0)
                {
                    if (ConvertGregorianToTimeVal(&gd, &tv) >= 0)
                    {
                        memset(&ioInfo, 0, sizeof(IOInfo));
                        ioInfo.ioi_Command         = TIMERCMD_SETTIME_USEC;
                        ioInfo.ioi_Send.iob_Buffer = &tv;
                        ioInfo.ioi_Send.iob_Len    = sizeof(tv);
                        DoIO(timer, &ioInfo);
                    }
                }

                CloseBattFolio();
            }
            CloseDateFolio();
        }
        DeleteTimerIOReq(timer);
    }
}


/*****************************************************************************/


static void LaunchEventBroker(void)
{
Item  module;
char  ebPath[100];
int32 sigs;

    if (FindFileAndIdentify(ebPath, sizeof(ebPath), "System.m2/Tasks/eventbroker", searchForFile) >= 0)
    {
        sigs = AllocSignal(0);
        if (sigs > 0)
        {
            module = OpenModule(ebPath, OPENMODULE_FOR_THREAD, NULL);
            if (module >= 0)
            {
	        if (CreateModuleThreadVA(module, "EventBroker",
                                         CREATETASK_TAG_ARGC, sigs,
                                         CREATETASK_TAG_ARGP, CURRENTTASKITEM,
                                         TAG_END) >= 0)
                {
                    WaitSignal(sigs);
                }
                CloseModule(module);
            }
            FreeSignal(sigs);
        }
    }
}


/*****************************************************************************/


#ifdef BUILD_DEBUGGER
static void DebuggerConsole(void)
{
Item           consoleDev;
Item           consoleIO;
IOInfo         ioInfo;
Err            err;
ScriptContext *sc;
char           prompt[64];
char           inputBuffer[129];
int32          status;
List          *list;

    err = CreateDeviceStackListVA(&list,
		"cmds", DDF_EQ, DDF_INT, 1, HOSTCONSOLE_CMD_GETCMDLINE,
		NULL);
    if (err < 0)
    {
	printf("FATAL: shell cannot get device list for host console\n");
	WaitSignal(0);
    }

    if (IsEmptyList(list))
    {
	printf("FATAL: shell cannot find host console\n");
	WaitSignal(0);
    }
    consoleDev = OpenDeviceStack((DeviceStack *) FirstNode(list));
    DeleteDeviceStackList(list);
    if (consoleDev < 0)
    {
	printf("FATAL: shell cannot open host console\n");
	WaitSignal(0);
    }

    if (consoleDev >= 0)
    {
        consoleIO = CreateIOReq(0,0,consoleDev,0);
        if (consoleIO >= 0)
        {
            memset(&ioInfo,0,sizeof(ioInfo));
            ioInfo.ioi_Command         = HOSTCONSOLE_CMD_GETCMDLINE;
            ioInfo.ioi_Send.iob_Buffer = prompt;
            ioInfo.ioi_Recv.iob_Buffer = inputBuffer;
            ioInfo.ioi_Recv.iob_Len    = sizeof(inputBuffer) - 1;

            if (OpenScriptFolio() >= 0)
            {
                if (CreateScriptContextVA(&sc, SCRIPT_TAG_BACKGROUND_MODE, TRUE, TAG_END) >= 0)
                {
                    while ((KernelBase->kb_CPUFlags & KB_NODBGR) == 0)
                    {
                        /* wait for something to happen */

                        GetDirectory(prompt,sizeof(prompt) - 3);
                        strcat(prompt,"> ");
                        ioInfo.ioi_Send.iob_Len = strlen(prompt);

                        err = DoIO(consoleIO, &ioInfo);
                        if (err >= 0)
                        {
                            inputBuffer[IOREQ(consoleIO)->io_Actual] = 0;

                            if (strcasecmp(inputBuffer, "rom") == 0)
                            {
				printf("Quitting interactive shell\n");
				break;
		            }

                            err = ExecuteCmdLineVA(inputBuffer, &status, SCRIPT_TAG_CONTEXT, sc, TAG_END);
                            if (err < 0)
                                PrintfSysErr(err);
                        }
                    }
                    DeleteScriptContext(sc);
                }
                CloseScriptFolio();
            }
            DeleteIOReq(consoleIO);
        }
        CloseDeviceStack(consoleDev);
    }
}
#endif


/*****************************************************************************/


#ifdef BUILD_SERIAL

#define	CTL(x)	((x)&0x1F)
#define	KEY_DELC	CTL('H')
#define	KEY_DELL	CTL('X')
#define	KEY_REDRAW	CTL('R')
#define	KEY_NEWL	CTL('M')
#define	KEY_REPL	CTL('G')

static void SerialConsole(void)
{
int32          ch;
int32          charCnt;
char           inputBuffer[128];
char           lastCommand[128];
Item           timer;
ScriptContext *sc;
Err            err;
char           prompt[64];
int32          status;
uint32         vblCnt;

#ifdef BUILD_STRINGS
    printf("WARNING: Starting serial shell, which pulls in the script folio,\n");
    printf("         which costs 20-30K of RAM. This doesn't happen in a\n");
    printf("         production system. Keep this in mind if you're\n");
    printf("         doing footprint calculations.\n\n");
#endif

    if (OpenScriptFolio() >= 0)
    {
        if (CreateScriptContextVA(&sc, SCRIPT_TAG_BACKGROUND_MODE, TRUE, TAG_END) >= 0)
        {
            timer = CreateTimerIOReq();
            if (timer >= 0)
            {
                vblCnt         = 300;
                lastCommand[0] = 0;
                while (TRUE)
                {
                    charCnt        = 0;
                    inputBuffer[0] = 0;

                    GetDirectory(prompt,sizeof(prompt) - 3);
                    strcat(prompt,"> ");

                    printf(prompt);
                    while (TRUE)
                    {
                        ch = MayGetChar();
                        if (ch < 0)
                        {
                            WaitTimeVBL(timer, vblCnt);
                            continue;
                        }

                        vblCnt = 1;
                        if (ch == KEY_DELC)
                        {
                            if (charCnt)
                            {
                                printf("%c",8);
                                charCnt--;
                                if (!charCnt)
                                    printf("\n");
                            }
                        }
                        else
                        {
                            if (ch == KEY_DELL)
                            {
                                printf("\n%s", prompt);
                                charCnt = 0;
                            }
                            else if (ch == KEY_REDRAW)
                            {
                                printf("\n%s%s", prompt, inputBuffer);
                                continue;
                            }
                            else if (ch == KEY_NEWL)
                            {
                                strcpy(lastCommand, inputBuffer);
                                break;
                            }
                            else if (ch == KEY_REPL)
                            {
                                strcpy(inputBuffer, lastCommand);
                                printf("\n%s%s", prompt, inputBuffer);
                                break;
                            }
                            else if (charCnt < sizeof(inputBuffer) - 1)
                            {
                                printf("%c",ch);
                                inputBuffer[charCnt++] = ch;
                            }
                            else
                            {
                                printf("%c",7);
                                break;
                            }
                        }
                        inputBuffer[charCnt] = 0;
                    }
                    printf("\n");

                    if (strcasecmp(inputBuffer, "rom") == 0)
                    {
                        printf("Quitting interactive shell\n");
                        break;
                    }

                    err = ExecuteCmdLineVA(inputBuffer, &status, SCRIPT_TAG_CONTEXT, sc, TAG_END);
                    if (err < 0)
                        PrintfSysErr(err);
                }
            }
            DeleteScriptContext(sc);
        }
        CloseScriptFolio();
    }
}
#endif


/*****************************************************************************/


static void DoStartupScript(const char *scriptName)
{
int32 status;

    if (OpenScriptFolio() >= 0)
    {
        ExecuteCmdLine(scriptName, &status, NULL);
        CloseScriptFolio();
    }
}


/*****************************************************************************/


static void SystemStartupScriptThread(void)
{
char scriptName[100];

    if (FindFileAndIdentify(scriptName, sizeof(scriptName), "System.m2/Scripts/SystemStartup", searchForFile) >= 0)
        DoStartupScript(scriptName);
}


/*****************************************************************************/


#ifdef BUILD_DEBUGGER
static void DevStartupScriptThread(void)
{
char scriptName[100];

    if (FindFileAndIdentify(scriptName, sizeof(scriptName), "DevStartup", searchForFile) >= 0)
        DoStartupScript(scriptName);
}
#endif


static void TmpItemServices(Item syncSignal)
{
int32 serverSignal;
Item  serverPort;

    serverPort = CreateMsgPort("ItemServer",201,0);
    serverSignal = MSGPORT(serverPort)->mp_Signal;
    SendSignal(FindTask("shell"), syncSignal);

    while (TRUE)
    {
        WaitSignal(serverSignal);
        ProvideItemServices(serverPort);
    }
}


/*****************************************************************************/


static void WaitForChild(Item childItem, Item serverPort)
{
Task  *task;
uint32  mask;

    mask = (SIGF_DEADTASK | MSGPORT(serverPort)->mp_Signal);

    while (TRUE)
    {
        /* is the child still around? */
        task = (Task *)CheckItem(childItem,KERNELNODE,TASKNODE);
        if (!task)
            break;

        /* did we loose ownership of the task? */
        if (task->t.n_Owner != CURRENTTASKITEM)
            break;

        WaitSignal(mask);
        ProvideItemServices(serverPort);
    }

    ProvideItemServices(serverPort);
}


/*****************************************************************************/


static Err RunProgram(const char *filename)
{
Err     result;
Item    module;
Module *mptr;

    module = result = OpenModule(filename, OPENMODULE_FOR_TASK, NULL);
    if (module >= 0)
    {
        mptr = (Module *)LookupItem(module);
        result = CreateTaskVA(module, mptr->n.n_Name,
                              CREATETASK_TAG_CMDSTR, filename,
                              TAG_END);
        CloseModule(module);
    }

    return result;
}


/*****************************************************************************/


void main(void)
{
Item file;
Item launchMeTask;
Item scriptThread;
Item serverPort;
Item itemServerThread;
Item consoleThread;
Item syncSignal;

    serverPort       = CreateMsgPort("ItemServer", 200, 0);
    itemServerThread = CreateThread(TmpItemServices, "TmpUserServices", 100, 2048, NULL);

    ChangeDirectory("/");
    ChangeDirectory(KernelBase->kb_AppVolumeName);

#ifdef BUILD_STRINGS
    AttachErrors();
#endif

    file = FindFileAndOpen("System.m2/Scripts/SystemStartup", searchForFile);

    if (file >= 0)
    {
        CloseFile(file);
        scriptThread = CreateThread(SystemStartupScriptThread,"SystemStartup", 0, 3072, NULL);
        WaitForChild(scriptThread, serverPort);
    }

#ifdef BUILD_DEBUGGER
    file = FindFileAndOpen("DevStartup", searchForFile);
    if (file >= 0)
    {
        CloseFile(file);
        scriptThread = CreateThread(DevStartupScriptThread,"DevStartup", 0, 3072, NULL);
        WaitForChild(scriptThread, serverPort);
    }
#endif

    /* do some misc system setup */
    SetMountLevel(MOUNT_EXTERNAL_RW_NOBOOT);
    SetSystemClock();
    LaunchEventBroker();

#ifdef BUILD_DEBUGGER
#ifdef BUILD_PCDEBUGGER
	ChangeDirectory("/remote");
#endif
    consoleThread = CreateThread(DebuggerConsole, CONSOLE_THREAD_NAME, 0, 3072, NULL);
#else
#ifdef BUILD_SERIAL
    if (KernelBase->kb_CPUFlags & KB_SERIALPORT)
    {
        consoleThread = CreateThread(SerialConsole, CONSOLE_THREAD_NAME, 0, 3072, NULL);
    }
    else
#endif
    {
        consoleThread = -1;
    }
#endif

    launchMeTask = -1;
    syncSignal = AllocSignal(0);
    while (TRUE)
    {
        if ((LookupItem(consoleThread) == NULL)
         && (LookupItem(launchMeTask) == NULL))
        {
            if (LookupItem(itemServerThread) == NULL)
	    {
                itemServerThread = CreateThreadVA(
			TmpItemServices, "TmpUserServices", 100, 2048,
			CREATETASK_TAG_ARGC, syncSignal,
			TAG_END);
		WaitSignal(syncSignal);
	    }
            launchMeTask = RunProgram("LaunchMe.m2");
        }

        DeleteThread(itemServerThread);
        ScavengeMem();

        if (LookupItem(launchMeTask) == NULL)
            WaitSignal(SIGF_DEADTASK | MSGPORT(serverPort)->mp_Signal);

        WaitForChild(launchMeTask, serverPort);
    }
}
