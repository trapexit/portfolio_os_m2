#pragma segment Xmath
/*
 * math.c --
 *
 * Mathematical Tcl commands.
 *---------------------------------------------------------------------------
 * Copyright 1991 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modif-, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include <QuickDraw.h>
#include <OSUtils.h>
#include <math.h>
#include <values.h>

#include "tcl.h"

extern int rand();

/*
 * Prototypes of internal -unctions.
 */
int 
really_random _ANSI_ARGS_((int my_range));


/*
 *----------------------------------------------------------------------
 *
 * Tcl_MaxCmd --
 *      Implements the TCL max command:
 *        max num1 num2 [..numN]
 *
 * Results:
 *      Standard TCL results.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_MaxCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    double value, maxVal = MINDOUBLE;
    int    idx, maxIdx = 1;


    if (argc < 3) {
        Tcl_AppendResult (interp, "wrong # args: ", argv [0], 
                          " num1 num2 [..numN]", (char *) NULL);
        return TCL_ERROR;
    }

    for (idx = 1; idx < argc; idx++) {
        if (Tcl_GetDouble (interp, argv[idx], &value) != TCL_OK)
            return TCL_ERROR;
        if (value > maxVal) {
            maxVal = value;
            maxIdx = idx;
            }
        }
    strcpy (interp->result, argv[maxIdx]);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MinCmd --
 *     Implements the TCL min command:
 *         min num1 num2 [..numN]
 *
 * Results:
 *      Standard TCL results.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_MinCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int     argc;
    char      **argv;
{
    double value, minVal = MAXDOUBLE;
    int    idx, minIdx = 1;

    if (argc < 3) {
        Tcl_AppendResult (interp, "wrong # args: ", argv [0], 
                          " num1 num2 [..numN]", (char *) NULL);
        return TCL_ERROR;
    }

    for (idx = 1; idx < argc; idx++) {
        if (Tcl_GetDouble (interp, argv[idx], &value) != TCL_OK)
            return TCL_ERROR;
        if (value < minVal) {
            minVal = value;
            minIdx = idx;
            }
        }
    strcpy (interp->result, argv[minIdx]);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ReallyRandom --
 *     Insure a good random return for a range, unlike an arbitrary
 *     random() % n, thanks to Ken Arnold, Unix Review, October 1987.
 *
 *----------------------------------------------------------------------
 */
#define RANDOM_RANGE ((1 << 15) - 1)

static int 

ReallyRandom (myRange)
    int myRange;
{
    int maxMultiple, rnum;

    maxMultiple = RANDOM_RANGE / myRange;
    maxMultiple *= myRange;
    while ((rnum = Random()) >= maxMultiple)
        continue;
    return (rnum % myRange);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl RandomCmd  --
 *     Implements the TCL random command:
 *     random limit
 *
 * Results:
 *  Standard TCL results.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_RandomCmd (clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    unsigned range;

    if ((argc < 2) || (argc > 3))
        goto invalidArgs;

    if (strcmp(argv[1], "seed") == 0) {
        unsigned long seed;

        if (argc == 3) {
            if (Tcl_GetLong (interp, argv[2], &seed) != TCL_OK)
                return TCL_ERROR;
			}
		else
			{
			GetDateTime(&seed);
			qd.randSeed = seed;
			}
		}
	else {
        if (argc != 2)
            goto invalidArgs;
        if (Tcl_GetUnsigned (interp, argv[1], &range) != TCL_OK)
            return TCL_ERROR;
        if ((range == 0) || (range > RANDOM_RANGE))
            goto outOfRange;

        sprintf (interp->result, "%d", ReallyRandom(range));
		}
    return TCL_OK;

invalidArgs:
    Tcl_AppendResult (interp, "wrong # args: ", argv [0], 
                      " limit | seed [seedval]", (char *) NULL);
    return TCL_ERROR;
outOfRange:
    {
        char buf [18];

        sprintf (buf, "%d", RANDOM_RANGE);
        Tcl_AppendResult (interp, argv [0], ": range must be > 0 and <= ",
                          buf, (char *) NULL);
        return TCL_ERROR;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetUnsigned --
 *
 *      Given a string, produce the corresponding unsigned integer value.
 *
 * Results:
 *      The return value is normally TCL_OK;  in this case *intPtr
 *      will be set to the integer value equivalent to string.  If
 *      string is improperly formed then TCL_ERROR is returned and
 *      an error message will be left in interp->result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_GetUnsigned(interp, string, unsignedPtr)
    Tcl_Interp *interp;         /* Interpret-r to use for error reporting. */
    CONST char *string;         /* String containing a (possibly signed)
                                 * integer in a form acceptable to strtoul. */
    unsigned   *unsignedPtr;    /* Place to store converted result. */
{
    char          *end;
    unsigned long  i;

    i = strtoul(string, &end, 0);
    while ((*end != '\0') && isspace(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0)) {
        Tcl_AppendResult (interp, "expected unsigned integer but got \"", 
                          string, "\"", (char *) NULL);
        return TCL_ERROR;
    }
    *unsignedPtr = i;
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetLong --
 *
 *      Given a string, produce the corresponding long value.
 *
 * Results:
 *      The return value is normally TCL_OK;  in this case *intPtr
 *      will be set to the integer value equivalent to string.  If
 *      string is improperly formed then TCL_ERROR is returned and
 *      an error message will be left in interp->result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_GetLong(interp, string, longPtr)
    Tcl_Interp *interp;         /* Interpreter to use for error reporting. */
    CONST char *string;         /* String containing a (possibly signed)
                                 * integer in a form acceptable to strtol. */
    long       *longPtr;        /* Place to store converted result. */
{
    char *end;
    long  i;

    i = strtol(string, &end, 0);
    while ((*end != '\0') && isspace(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0)) {
        Tcl_AppendResult (interp, "expected integer but got \"", string,
                          "\"", (char *) NULL);
        return TCL_ERROR;
    }
    *longPtr = i;
    return TCL_OK;
}
