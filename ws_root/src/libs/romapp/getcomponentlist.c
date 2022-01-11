/* @(#) getcomponentlist.c 95/11/29 1.7 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/list.h>
#include <file/fileio.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef BUILD_STRINGS
#define	DBUG(x) printf x
#else
#define	DBUG(x)
#endif


/*****************************************************************************/


static const char *
SkipSpace(const char *s)
{
	while (*s == ' ' || *s == '\t')
		s++;
	return s;
}


/*****************************************************************************/


static const char *
SkipNonspace(const char *s)
{
	while (*s != ' ' && *s != '\t' && *s != '\0')
		s++;
	return s;
}


/*****************************************************************************/


static Node *
ProcessLine(const char *line, const char *wantType, const char *filename)
{
	const char *name;
	const char *nameEnd;
	const char *type;
	const char *typeEnd;
	int priority;
	Node *n;

	/* Skip leading blanks and tabs */
	name = SkipSpace(line);

	/* # indicates the rest of the line is a comment */
	if (*name == '#' || *name == '\0')
		return NULL;

	if (*name != '"')
	{
		/* Name is the next whitespace-delimited word. */
		nameEnd = SkipNonspace(name);
		type = nameEnd;
	} else
	{
		/* Name is enclosed in quotes.  Find the terminating quote. */
		name++;
		for (nameEnd = name; *nameEnd != '"';  nameEnd++)
			if (*nameEnd == '\0')
				return NULL;
		type = nameEnd+1;
	}

	/* Type is the next whitespace-delimited word. */
	type = SkipSpace(type);
	typeEnd = SkipNonspace(type);

	/* Priority is a decimal integer. */
	priority = atoi(SkipSpace(typeEnd));

	if (strncmp(wantType, type, typeEnd - type) != 0)
	{
		/* Not the type we're looking for. */
		return NULL;
	}

	/* Allocate a node and store the pathname and priority in it. */
	n = AllocMem(sizeof(Node) + strlen(filename) + (nameEnd - name) + 3,
		MEMTYPE_TRACKSIZE | MEMTYPE_ANY);
	if (n == NULL)
	{
		DBUG(("GetComponentList: cannot alloc node\n"));
		return NULL;
	}
	n->n_Name = (char *) &n[1];
	n->n_Priority = priority;
	sprintf(n->n_Name, "/%s/%.*s", filename, nameEnd - name, name);
	return n;
}

/**
|||	AUTODOC -private -class LibROMApp -name GetComponentList
|||	Extract a list of components by parsing a fixed-name file
|||	that can appear on all mounted read-only file systems.
|||
|||	  Synopsis
|||
|||	    List *GetComponentList(const char *fileName, const char *type);
|||
|||	  Description
|||
|||	    This function is used to construct lists of available
|||	    components within the 3DO file system universe.
|||
|||	    You give this function a standard name for a component file.
|||	    A component file is an ASCII text file of the form:
|||
|||	      <relative component path> <type> <priority>
|||
|||	    There can be any number of lines similar to the above. This
|||	    function looks on all mounted file systems for component files,
|||	    and builds up a list in memory of all the components that it
|||	    found within all of the component files whose <type> field
|||	    matches the type argument to this function.
|||
|||	    Within the component file, the paths specified are relative
|||	    to the root of the file system. The priority is a value from
|||	    0 to 255, where 255 is the highest priority.
|||
|||	    Here is an example component file:
|||
|||	      bin/AudioCD audio 3
|||	      bin/VideoCD video 123
|||	      bin/PhotoCD photo 214
|||
|||	    The above component file specifies that the given file system
|||	    contains three components, and gives their paths, types,
|||	    and priorities.
|||
|||	  Arguments
|||
|||	    fileName
|||	        The name of the file to look for on every device.
|||	    type
|||	        The type of component to consider when building the list.
|||
|||	  Return Value
|||
|||	    A pointer to a List structure filled with nodes representing the
|||	    different components. The nodes on the list have a valid name
|||	    pointer and priority. The list is sorted from higher to lower
|||	    priority nodes.
|||
|||	  See Also
|||
|||	    FreeComponentList()
|||
**/

List *GetComponentList(const char *filename, const char *type)
{
	Directory     *dir;
	RawFile       *file;
	char           buffer[300];
	uint32         i;
	DirectoryEntry de;
	bool           done;
	List          *l;
	Node          *n;

	l = AllocMem(sizeof(List), MEMTYPE_ANY);
	if (l == NULL)
	{
		DBUG(("GetComponentList: cannot alloc list\n"));
		return NULL;
	}
	InitList(l, NULL);

	dir = OpenDirectoryPath("/");
	if (dir == NULL)
	{
		DBUG(("GetComponentList: cannot open root dir\n"));
		FreeMem(l, sizeof(List));
		return NULL;
	}

	/*
	 * Look at each file named "/xxx/filename",
	 * where "filename" is the argument filename.
	 */
	while (ReadDirectory(dir, &de) >= 0)
	{
		if (!(de.de_Flags & FILE_IS_READONLY))
		{
			/* Only look at readonly files. */
			continue;
		}
		sprintf(buffer, "/%s/%s", de.de_FileName, filename);
		if (OpenRawFile(&file, buffer, FILEOPEN_READ) < 0)
		{
			/* File cannot be opened. */
			continue;
		}

		done = FALSE;
		do
		{
			/* Read a line from the file. */
			i = 0;
			while (i < sizeof(buffer) - 1)
			{
				if (ReadRawFile(file, &buffer[i], sizeof(char)) <= 0)
				{
					done = TRUE;
					break;
				}
				if (buffer[i] == '\n' || buffer[i] == '\r')
					break;
				i++;
			}
			buffer[i] = '\0';

			n = ProcessLine(buffer, type, de.de_FileName);
			if (n != NULL)
				InsertNodeFromTail(l, n);

		} while (!done);

		CloseRawFile(file);
	}
	CloseDirectory(dir);
	return l;
}


/*****************************************************************************/


/**
|||	AUTODOC -private -class LibROMApp -name FreeComponentList
|||	Release resources allocated by GetComponentList().
|||
|||	  Synopsis
|||
|||	    void FreeComponentList(List *l);
|||
|||	  Description
|||
|||	    This functions releases any resources that GetComponentList()
|||	    might have allocated.
|||
|||	  Arguments
|||
|||	    l
|||	        A List pointer, as returned by GetComponentList()
|||
|||	  See Also
|||
|||	    GetComponentList()
|||
**/

void 
FreeComponentList(List *l)
{
	Node *n;

	if (l == NULL)
		return;
	while ((n = RemHead(l)) != NULL)
		FreeMem(n, -1);
	FreeMem(l, sizeof(List));
}
