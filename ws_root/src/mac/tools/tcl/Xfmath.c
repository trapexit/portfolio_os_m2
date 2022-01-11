#pragma segment Xfmath

/* 
 * fmath.c --
 *
 *      Contains the TCL trig and floating point math functions.
 *---------------------------------------------------------------------------
 * Copyright 1991 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include <math.h>
#include <sane.h>
#include "tcl.h"

#define FALSE		0
#define TRUE		1

#define CHECK_FP_ERROR()	\
		( testexception(INVALID | UNDERFLOW | OVERFLOW | DIVBYZERO) )

/*
 * Flag used to indicate if a floating point math routine is currently being
 * execu-ed.  Used to determine if a fmatherr belongs to Tcl.
 */
static int G_inTclFPMath = FALSE;

/*
 * Flag indicating if a floating point math error occured during the execution
 * of a library routine called by a Tcl command.  Will not be set by the trap
 * handler if the error did not occur while the `G_inTclFPMath' flag was
 * set.  If the error did occur the error type and the name of the function
 * that got the error are sa e here.
 */
static int   G_gotTclFPMathErr = FALSE;
static char *G_functionName;
static int   G_errorType;

/*
 * Prototypes of internal functions.
 */
int
Tcl_UnaryFloatFunction _ANSI_ARGS_((Tcl_Interp *interp,
                                    int         argc,
                                    char      **argv,
                                    double (*function)()));


/*
 *----------------------------------------------------------------------
 *
 * ReturnFPMathError --
 *    Routine to set an interpreter result to contain a floating point
 * math error message.  Will clear the `G_gotTclFPMathErr' flag.
 * This routine al ays returns the value TCL_ERROR, so if can be called
 * as the argument to `return'.
 *
 * Globals:
 *   o G_gotTclFPMathErr (O) - Flag indicating an error occured, will be 
 *     cleared.
 *   o G_functionName (I) - Name of function that got the error.
 *   o G_errorType (I) - Type of error that occured.
 *----------------------------------------------------------------------
 */
static int
ReturnFPMathError(interp)
    Tcl_Interp *interp;
	{
    char *ers;

	if (testexception(INVALID))
		ers = "INVALID";
	else if (testexception(UNDERFLOW))
		ers = "UNDERFLOW";
	else if (testexception(OVERFLOW))
		ers = "OVERFLOW";
	else if (testexception(DIVBYZERO))
		ers = "DIVBYZERO";
	else if (testexception(INEXACT))
		ers = "INEXACT";

    sprintf(interp->result, "%s: floating point %s error", G_functionName, ers);

    return TCL_ERROR;
	}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UnaryFloatFunction --
 *    Helper routine that implements Tcl unary floating point
 *     functions by validating parameters, converting the
 *     argument, applying the function (the address of which
 *     is passed as an argument), and converting the result to
 *     a string and storing it in the result buffer
 *
 * Results:
 *      Returns TCL_OK if number is present, conversion succeeded,
 *        the function was performed,  tc.
 *      Return TCL_ERROR for any error; an appropriate error message
 *        is placed in the result string in this case.
 *
 *----------------------------------------------------------------------
 */
static int
Tcl_UnaryFloatFunction(interp, argc, argv, function)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
    double (*function)();
{
    double dbVal;

	G_functionName = argv[0];

    if (argc != 2) {
        Tcl_AppendResult (interp, "wrong # args: ", argv [0], " val",
                          (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetDouble (interp, argv[1], &dbVal) != TCL_OK)
        return TCL_ERROR;

    G_inTclFPMath = TRUE;
    sprintf(interp->result, "%g", (*function)(dbVal));
    G_inTclFPMath = FALSE;

	if (CHECK_FP_ERROR())
        return ReturnFPMathError (interp);

    return TCL_OK;
}
/*
 *----------------------------------------------------------------------
 *
 * Tcl_StrToLong --
 *      Convert an Ascii string to an long number of the specified base.
 *
 * Parameters:
 *   o string (I) - String containing a number.
 *   o base (I) - The base to use for the number 8, 10 or 16 or zero to decide
 *     based on the leading characters of the number.  Zero to let the number
 *     determine the base.
 *   o longPtr (O) - Place to return the converted number.  Will be 
 *     unchanged if there is an error.
 *
 * Returns:
 *      Returns 1 if the string was a valid number, 0 invalid.
 *----------------------------------------------------------------------
 */
int
Tcl_StrToLong (string, base, longPtr)
    CONST char *string;
    int         base;
    long       *longPtr;
{
    char *end;
    long  num;

    num = strtol(string, &end, base);
    while ((*end != '\0') && isspace(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0))
        return FALSE;
    *longPtr = num;
    return TRUE;

} /* Tcl_StrToLong */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_StrToInt --
 *      Convert an Ascii string to a number of the specified base.
 *
 * Paramet-rs:
 *   o string (I) - String containing a number.
 *   o base (I) - The base to use for the number 8, 10 or 16 or zero to decide
 *     based on the leading characters of the number.  Zero to let the number
 *     det-rmine the base.
 *   o intPtr (O) - Place to return the converted number.  Will be 
 *     unchanged if there is an error.
 *
 * Returns:
 *      Returns 1 if the string was a valid number, 0 invalid.
 *----------------------------------------------------------------------
 */
int
Tcl_StrToInt (string, base, intPtr)
    CONST char *string;
    int         base;
    int        *intPtr;
{
    char *end;
    int   num;

    num = strtol(string, &end, base);
    while ((*end != '\0') && isspace(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0))
        return FALSE;
    *intPtr =-num;
    return TRUE;

} /* Tcl_StrToInt */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_StrToUnsigned --
 *      Convert an Ascii string to an unsigned int of the specified base.
 *
 * Parameters:
 *   o string (I) - String containing a number.
 *   o base (I) - The base to use for the number 8, 10 or 16 or zero to decide
 *     based on the-leading characters of the number.  Zero to let the number
 *     determine the base.
 *   o unsignedPtr (O) - Place to return the converted number.  Will be 
 *     unchanged if there is an error.
 *
 * Returns:
 *      Returns 1 if the string was a valid number, 0 invalid.
 *----------------------------------------------------------------------
 */
int
Tcl_StrToUnsigned (string, base, unsignedPtr)
    CONST char *string;
    int         base;
    unsigned   *unsignedPtr;
{
    char          *end;
    unsigned long  num;

    num = strtoul (string, &end, base);
    while ((*end != '\0') && isspace(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0))
        return FALSE;
    *unsignedPtr = num;
    return TRUE;

} /* Tcl_StrToUnsigned */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_StrToDouble --
 *   Convert a string to a double percision floating point number.
 *
 * Parameters:
 *   string (I) - Buffer containing double value to convert.
 *   doubleP-r (O) - The convert floating point number.
 * Returns:
 *   TRUE if the number is ok, FALSE if it is illegal.
 *-----------------------------------------------------------------------------
 */
int
Tcl_StrToDouble (string, doublePtr)
    CONST char *string;
    double     *doublePtr;
{
    char   *end;
    double  num;

    num = strtod (string, &end);
    while ((*end != '\0') && isspace(*end)) {
        end++;
    }
    if ((end == string) || (*end != 0))
        return FALSE;

    *doublePtr = num;
    return TRUE;

} /* Tcl_StrToDouble */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DownShift --
 *     Utility procedure to down-shift a string.  It is written in such
 *     a way as that the target string maybe the same as the source string.
 *
 * Parameters:
 *   o targetStr (I) - String to store the down-shifted string in.  Must
 *     have enough space allocated to store the string.  If NULL is specified,
 *     then the string will be dynamicly allocated and returned as the
 *     result of the function. May also be the same as the source string to
 *     shift in place.
 *   o sourceStr (I) - The string to down-shift.
 *
 * Returns:
 *   A pointer to the down-shifted string
 *----------------------------------------------------------------------
 */
char *
Tcl_DownShift (targetStr, sourceStr)
    char       *targetStr;
	CONST char *sourceStr;
{
    register char theChar;
	extern char *malloc();
	
    if (targetStr == NULL)
        targetStr = ckalloc (strlen ((char *) sourceStr) + 1);

    for (; (theChar = *sourceStr) != '\0'; sourceStr++) {
        if (isupper (theChar))
            theChar = tolower (theChar);
        *targetStr++ = theChar;
    }
    *targetStr = '\0';
    return targetStr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UpShift --
 *     Utility procedure to up-shift a string.
 *
 * Parameters:
 *   o targetStr (I) - String to store the up-shifted string in.  Must
 *     have enough space allocated to store the string.  If NULL is specified,
 *     then the string will be dynamicly allocated and returned as the
 *     result of the function. May also be the same as the source string to
 *     shift in place.
 *   o sourceStr (I) - The string to up-shift.
 *
 * Returns:
 *   A pointer to the up-shifted string
 *----------------------------------------------------------------------
 */
char *
Tcl_UpShift (targetStr, sourceStr)
    char       *targetStr;
	CONST char *sourceStr;
{
    register char theChar;
	extern char *malloc();
	
    if (targetStr == NULL)
        targetStr = ckalloc (strlen ((char *) sourceStr) + 1);

   for (; (theChar = *sourceStr) != '\0'; sourceStr++) {
        if (islower (theChar))
            theChar = toupper (theChar);
        *targetStr++ = theChar;
    }
    *targetStr = '\0';
    return targetStr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AcosCmd --
 *    Implements the TCL arccosine command:
 *        acos num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_AcosCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, acos);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AsinCmd --
 *    Implements the TCL arcsin command:
 *        asin num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_AsinCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, asin);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AtanCmd --
 *    Implements the TCL arctangent command:
 *        atan num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_AtanCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, atan);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CosCmd --
 *    Implements the TCL cosine command:
 *        cos num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_CosCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, cos);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SinCmd --
 *    Implements the TCL sin command:
 *        sin num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_SinCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, sin);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TanCmd --
 *    Implements the TCL tangent command:
 *        tan num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_TanCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, tan);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CoshCmd --
 *    Implements the TCL hyperbolic cosine command:
 *        cosh num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_CoshCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, cosh);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SinhCmd --
 *    Implements the TCL hyperbolic sin command:
 *        sinh num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_SinhCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, sinh);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TanhCmd --
 *    Implements the TCL hyperbolic tangent command:
 *        tanh num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_TanhCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, tanh);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ExpCmd --
 *    Implements the TCL exponent command:
 *        exp num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_ExpCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, exp);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LogCmd --
 *    Implements the TCL logarithm command:
 *        log num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_LogCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, log);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Log10Cmd --
 *    Implements the TCL base-10 logarithm command:
 *        log10 num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_Log10Cmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, log10);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SqrtCmd --
 *    Implements the TCL square root command:
 *        sqrt num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_SqrtCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, sqrt);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FabsCmd --
 *    Implements the TCL floating point absolute value command:
 *        fabs num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_FabsCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, fabs);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FloorCmd --
 *    Implements the TCL floor command:
 *        floor num
 *
 * Resu-ts:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_FloorCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, floor);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CeilCmd --
 *    Implements the TCL ceil command:
 *        ceil num
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_CeilCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    return Tcl_UnaryFloatFunction(interp, argc, argv, ceil);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FmodCmd --
 *    Implements the TCL floating modulo command:
 *        fmod num1 num2
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_FmodCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    double dbVal, dbDivisor;

	G_functionName = argv[0];

    if (argc != 3) {
        Tcl_AppendResult (interp, "wrong # args: ", argv [0], " val divisor",
                          (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetDouble (interp, argv[1], &dbVal) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_GetDouble (interp, argv[2], &dbDivisor) != TCL_OK)
        return TCL_ERROR;

    G_inTclFPMath = TRUE;
    sprintf(interp->result, "%g", fmod(dbVal,dbDivisor));
    G_inTclFPMath = FALSE;

	if (CHECK_FP_ERROR())
        return ReturnFPMathError (interp);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PowCmd --
 *    Implements the TCL power (exponentiation) command:
 *        pow num1 num2
 *
 * Results:
 *      Returns TCL_OK if number is present and conversion succeeds.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_PowCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    double dbVal, dbExp;

	G_functionName = argv[0];

    if (argc != 3) {
        Tcl_AppendResult (interp, "wrong # args: ", argv [0], " val exp",
                          (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetDouble (interp, argv[1], &dbVal) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_GetDouble (interp, argv[2], &dbExp) != TCL_OK)
        return TCL_ERROR;

    G_inTclFPMath = TRUE;
    sprintf(interp->result, "%g", pow(dbVal,dbExp));
    G_inTclFPMath = FALSE;

	if (CHECK_FP_ERROR())
        return ReturnFPMathError (interp);

    return TCL_OK;
}

int
Tcl_PiCmd(clientData, interp, argc, argv)
    ClientData  clientData;
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
{
    strcpy(interp->result, "3.141592654");
    return TCL_OK;
}


extern int Tcl_MaxCmd();
extern int Tcl_MinCmd();
extern int Tcl_RandomCmd();

Tcl_InitXmath(interp)
Tcl_Interp	*interp;
{
    /*
    ** from fmath.c
    */
    Tcl_CreateCommand(interp, "acos", Tcl_AcosCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "asin", Tcl_AsinCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "atan", Tcl_AtanCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "cos", Tcl_CosCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "sin", Tcl_SinCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "tan", Tcl_TanCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "cosh", Tcl_CoshCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "sinh", Tcl_SinhCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "tanh", Tcl_TanhCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "exp", Tcl_ExpCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "log", Tcl_LogCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "log10", Tcl_Log10Cmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "sqrt", Tcl_SqrtCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "fabs", Tcl_FabsCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "floor", Tcl_FloorCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "ceil", Tcl_CeilCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "fmod", Tcl_FmodCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "pow", Tcl_PowCmd, 
                     (ClientData)NULL, (void (*)())NULL);
    Tcl_CreateCommand(interp, "pi", Tcl_PiCmd, 
                     (ClientData)NULL, (void (*)())NULL);

    /*
     * from math.c
     */
    Tcl_CreateCommand (interp, "max", Tcl_MaxCmd, (ClientData)NULL, 
             (void (*)())NULL);
    Tcl_CreateCommand (interp, "min", Tcl_MinCmd, (ClientData)NULL, 
             (void (*)())NULL);
    Tcl_CreateCommand (interp, "random", Tcl_RandomCmd, (ClientData)NULL, 
             (void (*)())NULL);

	}
