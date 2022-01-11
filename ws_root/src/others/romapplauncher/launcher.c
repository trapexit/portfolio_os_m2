/* @(#) launcher.c 96/08/06 1.15 */

/* This program is meant to serve as LaunchMe for rom-based bootups.
 *
 * This program will scan all read-only file systems looking for files called
 * "ROMApps.txt" located in the root directory. This file contains a list
 * of rom apps available on the file system. The list is of the form:
 *
 *   <relative path> <type> <priority>
 *
 * The path name describes where the rom app is located within this file system,
 * and the priority defines in where this rom app should go in the list
 * of all rom apps in the system. Priorities range from 0 to 255, with 255
 * being the highest priority.  The type field specifies what type of
 * media the RomApp expects to work with.  Only RomApps whose type field
 * matches the type of CD are put into the list.
 *
 * Once the prioritized list of rom apps is constructed, this program then
 * simply loops through the list and executes every app.
 */

#include <kernel/types.h>
#include <kernel/list.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/item.h>
#include <kernel/task.h>
#include <file/filefunctions.h>
#include <misc/script.h>
#include <stdio.h>

#define	MyPrio	10

#ifdef BUILD_STRINGS
#define	DBUG(x)   printf x
#else
#define	DBUG(x)
#endif

/*****************************************************************************/


extern const char *RomAppMediaType(void);
extern List *GetComponentList(const char *fileName, const char *type);
extern void FreeComponentList(List *l);


/*****************************************************************************/

int
main(void)
{
	List *l;
	Node *n;
	int32 status;
	Err result;
	const char *discType;

	result = OpenScriptFolio();
	if (result < 0)
	{
		DBUG(("Cannot open script folio, err %x\n", result));
		return 0;
	}

	/* Get the list of all available RomApps. */
	l = GetComponentList("RomApps.txt", "launcher");
	if (l != NULL && !IsEmptyList(l))
	{
		/*
		* There is another launcher in the system.
		* See if it is newer than me.
		*/
		n = RemHead(l);
		if (n->n_Priority > MyPrio)
		{
			DBUG(("Yielding to newer launcher %s\n", n->n_Name));
			(void) ExecuteCmdLineVA(n->n_Name, &status, 
					SCRIPT_TAG_MUST_BE_SIGNED, (TagData)1,
					TAG_END);
			(void) CloseScriptFolio();
			return 0;
		}
	}

	/* Get list of appropriate RomApps. */
	discType = RomAppMediaType();
	l = GetComponentList("ROMApps.txt", discType);

#ifdef BUILD_PARANOIA
	if (l == NULL || IsListEmpty(l))
	{
		printf("WARNING: rom app launcher could not find any rom apps to execute!\n");
	}
#endif
	/* If no RomApps for this type of disc, use the "unknown" app. */
	if (IsListEmpty(l))
	{
		FreeComponentList(l);
		l = NULL;
	}
	if (l == NULL)
	{
		l = GetComponentList("ROMApps.txt", "unknown");
	}

#ifdef BUILD_STRINGS
	DBUG(("RomAppLauncher: list of apps:\n"));
	ScanList(l,n,Node)
	{
		DBUG(("  %s, prio %d\n", n->n_Name, n->n_Priority));
	}
#endif

	/*
	* Scan the list and start up each RomApp.
	* Any which are not appropriate for the RomApp media will exit.
	* Eventually (we hope), one will be the "right" one and will execute.
	*/
	ScanList(l,n,Node)
	{
		DBUG(("RomAppLauncher: trying %s\n", n->n_Name));
		result = ExecuteCmdLineVA(n->n_Name, &status, 
				SCRIPT_TAG_MUST_BE_SIGNED, (TagData)1,
				TAG_END);
		DBUG(("%s done: result %x, status %x\n", 
			n->n_Name, result, status));
		if (result >= 0 && status == 0)
		{
			/* Successfully ran the RomApp; don't try any others. */
			break;
		}
	}
	(void) CloseScriptFolio();
	return 0;
}
