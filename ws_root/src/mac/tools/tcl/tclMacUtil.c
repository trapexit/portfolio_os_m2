#ifdef macintosh
#	pragma segment tclMacUtil
#endif


#include "tclInt.h"
#include "errMac.h"

int		macintoshErr = 0;

char *
Tcl_MacErrorID(errno)
int		errno;
{
static char	macMsg[64];

	sprintf(macMsg, "Err#%d", errno);
	return macMsg;
	}

char *
Tcl_MacError (interp)
Tcl_Interp *interp;
{
int		idx;
char	*id, *msg;

	id = Tcl_MacErrorID(macintoshErr);
	for (	idx = 0 ;	(ErrorCodeTab [idx].errNum  != macintoshErr) && 
						(ErrorCodeTab [idx].errNum  != -1);
     		idx++  )
		;
	msg = ErrorCodeTab[idx].errCode;
    Tcl_SetErrorCode(interp, "MAC", id, msg, (char *) NULL);
    return msg;
}
