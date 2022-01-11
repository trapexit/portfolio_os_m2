/*
 *	@(#) diplib.list.c 96/07/02 1.2
 *	Copyright 1996, The 3DO Company
 *
 * List functions.
 * This is a diplib function, callable from device-dipirs.
 */

#include "kernel/types.h"
#include "kernel/list.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

extern const DipirRoutines *dipr;

void
PrepList(List *list)
{
	list->l_Head = (Link *) &list->l_Filler;
	list->l_Filler = NULL;
	list->l_Tail = (Link *) &list->l_Head;
}
