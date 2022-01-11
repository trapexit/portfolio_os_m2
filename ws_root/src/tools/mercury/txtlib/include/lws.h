/****
 *
 *  @(#) lws.h 95/07/10 1.42
 *  Copyright 1994, The 3DO Company
 *
 * Basic types and constants for LWS
 *
 ****/
#ifndef _FWLWS
#define _FWLWS


#ifdef __cplusplus
extern "C" {
#endif

/***
 *
 * PUBLIC C API
 *
 ***/
LWS*		LWS_Open(char *filename);
M2Err		LWS_Parse(LWS* lws, char *filename);
M2Err		LWS_Close(LWS*);
void		LWS_Print(LWS*, char*);

#ifdef __cplusplus
}
#endif

#endif	/* _FWLWS */
