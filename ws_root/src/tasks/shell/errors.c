/* @(#) errors.c 96/05/16 1.20 */

#ifdef BUILD_STRINGS

#include <kernel/types.h>
#include <file/filesystem.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <file/filefunctions.h>
#include <loader/loader3do.h>
#include <stdio.h>


/*****************************************************************************/


void AttachErrors(void)
{
Directory      *fsdir;
DirectoryEntry  fsde;
Directory      *dir;
DirectoryEntry  de;
Item            module;
char            path[80];
TagArg         *moduleTags;
Module         *mod;

    fsdir = OpenDirectoryPath("/");
    if (fsdir)
    {
        while (ReadDirectory(fsdir, &fsde) >= 0)
        {
	  if (!(fsde.de_Flags & FILE_IS_BLESSED)) {
	    continue;
	  }

            sprintf(path,"/%s/System.m2/Errors",fsde.de_FileName);

            dir = OpenDirectoryPath(path);
            if (dir)
            {
                while (ReadDirectory(dir, &de) >= 0)
                {
                    sprintf(path,"/%s/System.m2/Errors/%s",fsde.de_FileName, de.de_FileName);

                    module = OpenModule(path, OPENMODULE_FOR_THREAD, NULL);
                    if (module >= 0)
                    {
                        moduleTags = (TagArg *)ExecuteModule(module, 0, NULL);
                        mod        = (Module *)LookupItem(module);

                        CreateItemVA(MKNODEID(KERNELNODE,ERRORTEXTNODE),
                                     TAG_ITEM_PRI,      mod->n.n_Priority,
                                     TAG_ITEM_VERSION,  mod->n.n_Version,
                                     TAG_ITEM_REVISION, mod->n.n_Revision,
                                     TAG_JUMP,          moduleTags);
                    }
                }
                CloseDirectory(dir);
            }
        }
        CloseDirectory(fsdir);
    }
}
#else
	extern	int	foo; /* Make compiler happy */
#endif
