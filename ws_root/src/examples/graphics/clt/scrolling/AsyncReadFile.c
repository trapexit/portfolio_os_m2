/******************************************************************************
**
**  @(#) AsyncReadFile.c 96/08/20 1.5
**
******************************************************************************/
#include <kernel/types.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <stdio.h>
#include <file/fileio.h>
#include <string.h>

#define MAXFILENAMELENGTH 100

typedef struct ARFinfo {
  char filename[MAXFILENAMELENGTH];
  int numbytes;
  void **buffer;
  int *result;
  uint32 donesignal;
  Item parenttask;
  } ARFinfo;
/*this structure contains all the information that needs to be passed to the internal thread*/  
  
  
void internalasyncreadfile(ARFinfo *arfinfo)
/*this function contains the actual code for the internal thread. Note that the one argument
 *passed to it is a pointer to an ARFinfo structure. This is because only two 32-bit values
 *can be passed into threads, so it's easiest to just have one of them be a pointer to whatever
 *arguments you really want to pass
 */

{
  ARFinfo internalarfinfo;
  
  
  RawFile *file;
  FileInfo info;
  
  bool allocedmem=FALSE;
    /*this variable keeps track of whether we've actually allocated memory into the
	 *buffer. If we have, we have to deallocate it if there's an error
	 */
	
  memcpy(&internalarfinfo, arfinfo, sizeof(ARFinfo));
   /*the first thing we do is make a copy in local memory of all arguments we were passed,
    *since we can't depend on them remaining where they are
	*/
 
 
  if ((*internalarfinfo.result=OpenRawFile(&file, internalarfinfo.filename, FILEOPEN_READ))<0)
    goto ERROR;
   /*we then attempt to open up the named file*/	
	
  if (internalarfinfo.numbytes==0) {
	if ((*internalarfinfo.result=(GetRawFileInfo(file, &info, sizeof(FileInfo))))<0) 
	  goto ERROR;
    internalarfinfo.numbytes=info.fi_ByteCount;
	}
   /*if numbytes is zero, we get file info and set numbytes to equal the file size*/

  	
  if (*internalarfinfo.buffer==NULL) {
    *internalarfinfo.buffer=AllocMem(internalarfinfo.numbytes, MEMTYPE_NORMAL);
  	if (*internalarfinfo.buffer==NULL) {
	  *internalarfinfo.result=-1;
	  goto ERROR;
	  }
	else allocedmem=TRUE;  
	}
    /*if the buffer is NULL, we allocate a buffer of the appropriate size*/
 
  *internalarfinfo.result=ReadRawFile(file, *internalarfinfo.buffer, internalarfinfo.numbytes);
    /*we then do the actual reading*/
	
  ERROR:
  /*CloseRawFile(file);*/
  
  if ((*internalarfinfo.result<0) && (allocedmem)) 
    FreeMem(*internalarfinfo.buffer, internalarfinfo.numbytes);
	/*if we had an error, we deallocate any memory we had allocated*/
	
  SendSignal(internalarfinfo.parenttask, internalarfinfo.donesignal);
    /*we then send our signal, because we're done*/
	
}  

void AsyncReadFile(char *filename, int numbytes, void **buffer, uint32 donesignal, int *result)
{
  ARFinfo arfinfo;
  TagArg threadtags[2];
  
  strcpy(arfinfo.filename, filename);
  arfinfo.numbytes=numbytes;
  arfinfo.buffer=buffer;
  arfinfo.result=result;
  arfinfo.parenttask=CURRENTTASKITEM;
  arfinfo.donesignal=donesignal;
    /*first of all, we fill in our arfinfo structure with the appropriate values*/
   
  threadtags[0].ta_Tag=CREATETASK_TAG_ARGC;
  threadtags[0].ta_Arg=(TagData) &arfinfo;
  threadtags[1].ta_Tag=TAG_END;
   /*setting up our tags like this will result in the first argument (ARGC) passed into the
    *thread being a pointer to our arfinfo struct
	*/
   
  CreateThread(internalasyncreadfile, "readfilethread", CURRENTTASK->t.n_Priority+1,  2048, threadtags);
    /*now our thread is created, and away we go...*/

}
