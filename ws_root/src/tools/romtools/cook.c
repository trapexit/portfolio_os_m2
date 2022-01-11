
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

/* #define	DEBUG */

#ifdef	DEBUG
#define	DBUG(x)		printf x
#else
#define DBUG(x)		/* */
#endif

#define	DEFBUFSIZE	2*1024*1024
#define	MAXLINELEN	2048
#define	MAXVARS		100
#define	MAXVARNAME	32
#define	MAXDIRARGS	3
#define	MAXPRINTARGS	10
#define	IMAGEFILLCHAR	0xFF
#define WHITESPACE	" \t\n\r\f"
#define	OPERATOR	"+-*/%"

#define CPPCOMMAND	"/usr/lib/cpp -I. -B"
#define CSHCOMMAND	"/bin/csh -cf"
#define	CSHSETUP	"set nonomatch; unset noglob; unset noclobber"

#define	BYTETYPE	char
#define	HALFWORDTYPE	short int
#define	WORDTYPE	long int

#define	NOERRS		0
#define	ERRMISC		1

int	buffersize = 0,
	destaddr,
	imagesize,
	numlines = 0,
	parseflag = 1,
	numvars = 0,
	varvalue[MAXVARS],
	processid,
	quiet = 0,
	logoutput = 0,
	error = 0;

char	*imagebuffer = NULL,
	logfilename[MAXLINELEN] = "cook.log",
	recipename[MAXLINELEN],
	commandargs[MAXLINELEN],
	varname[MAXVARS][MAXVARNAME],
	*argument[MAXDIRARGS],
	*directive,
	*revision;

FILE	*outstream = stdout,
	*logfile;

static char sccsid[] = "@(#) cook.c 96/04/29 1.5";

void PrintLog(char *format, ...) {

	va_list		arguments;

	va_start(arguments, format);
	if (!quiet) vfprintf(outstream, format, arguments);
	if (logoutput == 2) vfprintf(logfile, format, arguments);
}

void PrintError(char *format, ...) {

	va_list		arguments;
	char		string[MAXLINELEN];

	outstream = stderr;
	quiet = 0;
	PrintLog("\n");
	PrintLog("**********\n");
	PrintLog("\n");

	if (numlines) PrintLog("cook: Error encountered at line %i of %s\n", numlines, recipename);
	else PrintLog("cook: Error encountered\n");

	va_start(arguments, format);
	if (!quiet) vfprintf(outstream, format, arguments);
	if (logoutput == 2) vfprintf(logfile, format, arguments);

	PrintLog("\n");
	PrintLog("**********\n");
	PrintLog("\n");
	exit(ERRMISC);
}

void ShowCredits(void) {
	PrintLog("\n");
	PrintLog("cook\n");
	PrintLog("Version 2, revision %s\n", revision);
	PrintLog("By Drew Shell\n");
	PrintLog("The 3DO Company - 1996\n\n");
	PrintLog("\n");
}

void ShowUsage(void) {
	ShowCredits();
	printf("usage: cook recipefile [-l|{-L logfile}] [-q|-Q] [args...]\n");
	printf("\n");
	printf("If -l is used, output is logged to cook.log.\n");
	printf("If -L is used, output is logged to logfile.\n");
	printf("If -q is used, cook runs quietly unless recipefile contains VERBOSE directives.\n");
	printf("If -Q is used, cook runs quietly in all cases.\n");
	printf("All args are passed as is to cpp when recipefile is parsed.\n");
	printf("Directive arguments that contain whitespace must be delimited by { and }.\n");
	printf("\n");
	printf("The following directives, and all cpp features, may be used in recipefile:\n\n");
	printf("  ADDRESS Expression        Set buffer pointer to value of Expression\n");
	printf("  ALIGN Expression          Align buffer pointer to value of Expression\n");
	printf("  BYTE Expression           Append value of Expression as a single byte\n");
	printf("  CMDWARGS Command          Execute Command as a system call with parent's args\n");
	printf("  COMMAND Command           Execute Command as a system call\n");
	printf("  HALFWORD Expression       Append value value of Expression as a single halfword\n");
	printf("  INITBUF [Exp]             Reinitialize image buffer (of size Exp if given)\n");
	printf("  OUTPUT File [Exp1 Exp2]   Write all or part (if Exp's given) of buffer to File\n");
	printf("  PARSEOFF                  Disable directive parsing (but not cpp features)\n");
	printf("  PARSEON                   Re-enable directive parsing\n");
	printf("  POKEBYTE Addr Exp         Poke value of Exp into Addr as a byte\n");
	printf("  POKEHALF Addr Exp         Poke value of Exp into Addr as a half word\n");
	printf("  POKEWORD Addr Exp         Poke value of Exp into Addr as a word\n");
	printf("  QUIET                     Switch to quiet mode\n");
	printf("  READFILE File [Exp1 Exp2] Append all or part (if Exp's given) of File\n");
	printf("  STRING [CALC] String      Append String (if CALC then String is an expression)\n");
	printf("  VARIABLE Name Expression  Set variable Name to value of Expression\n");
	printf("  VERBOSE                   Switch to verbose mode\n");
	printf("  WORD Expression           Append value value of Expression as a single word\n");
	printf("\n");
	exit(NOERRS);
}

void ParseDirective(char *inputline) {

	int		index;

	char		*endofline;

	for (index = 0; index < MAXDIRARGS; index ++) argument[index] = NULL;
	endofline = inputline + strlen(inputline);
	inputline += strspn(inputline, WHITESPACE);
	if (inputline < endofline) directive = strtok(inputline, WHITESPACE);
	else directive = NULL;
	if (directive != NULL) if (parseflag || !strcmp(directive, "#")) {
		inputline = directive + strlen(directive) + 1;
		inputline += strspn(inputline, WHITESPACE);
		index = 0;
		while (inputline < endofline) {
			if (*(char *)inputline == '{') {
				(*(char *)inputline = '}');
				argument[index] = strtok(inputline, "}");
			} else {
				argument[index] = strtok(inputline, WHITESPACE);
			}
			inputline = argument[index] + strlen(argument[index]) + 1;
			inputline += strspn(inputline, WHITESPACE);
			if (inputline > endofline) inputline = endofline;
			if (index++ == MAXDIRARGS) {
				PrintError("Too many directive arguments\n");
			}
		}
	}
}

int Calculate(char *equation_str) {

	int		openfound,
			result;

	char		string[MAXLINELEN],
			tempstring[MAXLINELEN],
			*open,
			*close;

	strcpy(string, equation_str);
	openfound = 0;
	while (open = strrchr(string, '(')) {
		openfound = 1;
		if (!(close = strchr(open, ')'))) {
			PrintError("Unmatched (\n");
		}
		*open = 0;
		*close = 0;
		result = CalcSimpleEqn(open + 1);
		sprintf(tempstring, "%s %d %s", string, result, close + 1);
		if (strlen(tempstring) >= MAXLINELEN) {
			PrintError("Intermediate equation longer than %i bytes (%s)\n", MAXLINELEN, tempstring);
		}
		strcpy(string, tempstring);
	}
	if (close = strchr(string, ')')) {
		PrintError("Unmatched )\n");
	}
	result = CalcSimpleEqn(string);
	return(result);
}

int CalcSimpleEqn(char *equation_str) {

	int		number,
			accumulator,
			accumsign,
			tempvalue,
			tempoperator,
			lasttokenoperator,
			thistokenoperator;

	char		equation[MAXLINELEN],
			*pointer,
			*token;

	strcpy(equation, equation_str);
	pointer = equation;
	accumulator = 0;
	accumsign = 1;
	tempvalue = 1;
	tempoperator = '*';
	lasttokenoperator = 1;
	while (token = strtok(pointer, WHITESPACE)) {
		if (!strcmp(token, "+")) {
			accumulator += accumsign * TempCalc(tempvalue, number, tempoperator);
			accumsign = 1;
			tempvalue = 1;
			tempoperator = '*';
			thistokenoperator = 1;
		} else if (!strcmp(token, "-")) {
			accumulator += accumsign * TempCalc(tempvalue, number, tempoperator);
			accumsign = -1;
			tempvalue = 1;
			tempoperator = '*';
			thistokenoperator = 1;
		} else if (!strcmp(token, "*")) {
			tempvalue = TempCalc(tempvalue, number, tempoperator);
			tempoperator = '*';
			thistokenoperator = 1;
		} else if (!strcmp(token, "/")) {
			tempvalue = TempCalc(tempvalue, number, tempoperator);
			tempoperator = '/';
			thistokenoperator = 1;
		} else if (!strcmp(token, "%")) {
			tempvalue = TempCalc(tempvalue, number, tempoperator);
			tempoperator = '%';
			thistokenoperator = 1;
		} else {
			number = TokenValue(token);
			thistokenoperator = 0;
		}
		if (thistokenoperator == lasttokenoperator) {
			PrintError("Bad equation syntax\n");
		}
		lasttokenoperator = thistokenoperator;
		pointer = NULL;
/*		printf("acc = %i, accsign = %i\n", accumulator, accumsign); */
/*		printf("temp = %i, tempop = %c\n", tempvalue, tempoperator); */
	}
	if (lasttokenoperator) {
		PrintError("Equation ends with operator\n");
	}
	accumulator += accumsign * TempCalc(tempvalue, number, tempoperator);
	return(accumulator);
}

int TokenValue(char *token_str) {

	int		number,
			match,
			index,
			typesize,
			bytesread;

	char		datatype,
			index_str[MAXLINELEN],
			file_str[MAXLINELEN];

	FILE		*filehandle;

	struct stat	fileinfo;

	if (sscanf(token_str, "%i", &number)) {
	} else if (!strcmp(token_str, "ADDRESS")) {
		number = destaddr;
	} else if (number = GetLength(token_str)) {
	} else if (token_str[1] == ':') {
		DBUG(("Think I found an indexed token (%s,%c)\n", token_str, token_str[1]));
		if (sscanf(token_str, "%1s:%[^:]:%s", &datatype, index_str, file_str) != 3) {
			DBUG(("Type, index, file = (%c, %s %s)\n", datatype, index_str, file_str));
			PrintError("Bad format for indexed token (%s)\n", token_str);
		}
		DBUG(("Type, index, file = (%c, %s %s)\n", datatype, index_str, file_str));
		if (!sscanf(index_str, "%i", &index)) {
			index = TokenValue(index_str);
		}
		if (datatype == 'B') typesize = 1;
		else if (token_str[0] == 'H') typesize = 2;
		else if (token_str[0] == 'W') typesize = 4;
		else {
			PrintError("Bad data type for indexed token (%s)\n", token_str);
		}
		if (index < 0) {
			PrintError("Index less than zero (%i)\n", index);
		}
		if (!strcmp(file_str, "IMAGEBUF")) {
			if (index + typesize > buffersize) {
				PrintError("Index plus type size exceeds buffer length (0x%X)\n", index + typesize);
			}
			if (typesize == 1) number = (int)*((BYTETYPE *)(imagebuffer + index));
			else if (typesize == 2) number = (int)*((HALFWORDTYPE *)(imagebuffer + index));
			else number = (int)*((WORDTYPE *)(imagebuffer + index));
		} else {
			if (strpbrk(file_str, "~$")) ExpandFileName(file_str);
			if (!(file_str)) {
				PrintError("Problem expanding index file\n");
			}
			if (!(filehandle = fopen(file_str, "rb"))) {
				PrintError("Problem opening index file (%s)\n", file_str);
			}
			if (stat(file_str, &fileinfo)) {
				PrintError("Problem getting index file info (%s)\n", file_str);
			}
			if (index + typesize > fileinfo.st_size) {
				PrintError("Index plus type size exceeds file length (0x%X)\n", index + typesize);
			}
			if (fseek(filehandle, index, 0)) {
				PrintError("Problem seeking to desired index (0x%X)\n", index);
			}
			number = 0;
			bytesread = fread((char *)&number + 4 - typesize, 1, typesize, filehandle);
			if (ferror(filehandle) || bytesread != typesize) {
				PrintError("Problem reading indexed file (%s)\n", file_str);
			}
			fclose(filehandle);
		}
	} else {
		match = 0;
		for (index = 0; index < numvars; index++) if (!strcmp(token_str, varname[index])) {
			number = varvalue[index];
			match = 1;
			index = numvars;
		}
		if (match == 0) {
			PrintError("Unresolved token (%s)\n", token_str);
		}
	}
	return(number);
}

int TempCalc(int operand1, int operand2, int operator) {

	if (operator == '*') {
		return (operand1 * operand2);
	} else if (operator == '/') {
		if (operand2 == 0) {
			PrintError("Divide by zero\n");
		}
		return (operand1 / operand2);
	} else if (operator == '%') {
		if (operand2 == 0) {
			PrintError("Modulo by zero\n");
		}
		return (operand1 % operand2);
	} else {
		PrintError("Unknown */% level operator (%s)\n", operator);
	}
}

int GetLength(char *filename) {

	char		expandname[MAXLINELEN];

	struct stat	fileinfo;

	if (strpbrk(filename, "~$")) {
		strcpy(expandname, filename);
		ExpandFileName(expandname);
		if (expandname) filename = expandname;
	}
	if (stat(filename, &fileinfo)) return(0);
	else return(fileinfo.st_size);
}

int ExpandFileName(char *filename) {

	char		string[MAXLINELEN];

	int		returncode;

	FILE		*stream;

	sprintf(string, "%s '%s; glob %s'", CSHCOMMAND, CSHSETUP, filename);
	if (strlen(string) >= MAXLINELEN) {
		PrintError("Expansion command longer than %i bytes (%s)\n", MAXLINELEN, string);
	}
	stream = popen(string, "r");
	fgets(filename, MAXLINELEN, stream);
	returncode = pclose(stream);
	if (returncode) filename = NULL;
}

void DoADDRESS(char *address_eqn) {

	destaddr = Calculate(address_eqn);
	if (destaddr > buffersize) {
		PrintError("ADDRESS overflow (0x%X)\n", destaddr);
	}
	if (destaddr < imagesize) {
		PrintError("ADDRESS overlap (0x%X)\n", destaddr);
	}
	PrintLog("Setting address to 0x%08X (%s)\n", destaddr, address_eqn);
	if (destaddr > imagesize) imagesize = destaddr;
}

void DoALIGN(char *modulus_eqn) {

	int		modulus,
			addtoalign;

	if (modulus_eqn == NULL) modulus = 4;
	else modulus = Calculate(modulus_eqn);
	addtoalign = (modulus - (destaddr % modulus)) % modulus;
	if (destaddr + addtoalign > buffersize) {
		PrintError("ALIGN overflow (0x%X)\n", addtoalign);
	}
	PrintLog("0x%08X   0x%08X   -align-      %s\n", destaddr, addtoalign, modulus_eqn);
	destaddr = destaddr + addtoalign;
	if (destaddr > imagesize) imagesize = destaddr;
}

void DoBYTE(char *byte_eqn) {

	int		constant;

	if (destaddr + 1 > buffersize) {
		PrintError("BYTE overflow\n");
	}
	constant = Calculate(byte_eqn);
	*((BYTETYPE *)(imagebuffer + destaddr)) = constant;
	PrintLog("0x%08X   0x00000001   0x%08X   %s\n", destaddr, constant, byte_eqn);
	destaddr = destaddr + 1;
	if (destaddr > imagesize) imagesize = destaddr;
}

void DoCOMMAND(char *command_str) {

	int		returncode;

	char		*bracket,
			evaluated_str[MAXLINELEN],
			fullcommand_str[MAXLINELEN],
			outputline[MAXLINELEN];

	FILE		*commandstream;

	if (command_str == NULL) {
		PrintError("No command string found\n");
	} 

	*evaluated_str = NULL;
	while (bracket = strpbrk(command_str, "[")) {
		*bracket = NULL;
		sprintf(evaluated_str, "%s%s", evaluated_str, command_str);
		if (strlen(evaluated_str) >= MAXLINELEN) {
			PrintError("Command longer than %i bytes (%s)\n", MAXLINELEN, evaluated_str);
		}
		command_str = bracket + 1;
		if (!(bracket = strpbrk(command_str, "]"))) {
			PrintError("Unmatched [\n");
		}
		*bracket = NULL;
		sprintf(evaluated_str, "%s%i", evaluated_str, Calculate(command_str));
		if (strlen(evaluated_str) >= MAXLINELEN) {
			PrintError("Command longer than %i bytes (%s)\n", MAXLINELEN, evaluated_str);
		}
		command_str = bracket+1;
	}
	sprintf(evaluated_str, "%s%s", evaluated_str, command_str);
	if (strlen(evaluated_str) >= MAXLINELEN) {
		PrintError("Command longer than %i bytes (%s)\n", MAXLINELEN, evaluated_str);
	}
	PrintLog("command = %s\n", evaluated_str);

	sprintf(fullcommand_str, "%s '%s; (%s)'", CSHCOMMAND, CSHSETUP, evaluated_str);
	if (strlen(fullcommand_str) >= MAXLINELEN) {
		PrintError("Command longer than %i bytes (%s)\n", MAXLINELEN, fullcommand_str);
	}
	DBUG(("command string = %s\n", fullcommand_str));
	commandstream = popen(fullcommand_str, "r");
	while (fgets(outputline, MAXLINELEN, commandstream)) {
		PrintLog("%s", outputline);
	}
	returncode = pclose(commandstream);
	if (returncode != 0) {
		PrintError("Command returned error code (%i)\n", returncode);
	}
}

void DoCMDWARGS(char *command_str) {

	char		cmdwargs_str[MAXLINELEN];

	sprintf(cmdwargs_str, "%s %s", command_str, commandargs);
	DoCOMMAND(cmdwargs_str);
}

void DoHALFWORD(char *halfword_eqn) {

	int		halfword;

	if (destaddr + 2 > buffersize) {
		PrintError("HALFWORD overflow\n");
	}
	if (destaddr % 2 != 0) {
		PrintError("HALFWORD address not halfword aligned (0x%X)\n", destaddr);
	}
	halfword = Calculate(halfword_eqn);
	*((HALFWORDTYPE *)(imagebuffer + destaddr)) = halfword;
	PrintLog("0x%08X   0x00000002   0x%08X   %s\n", destaddr, halfword, halfword_eqn);
	destaddr = destaddr + 2;
	if (destaddr > imagesize) imagesize = destaddr;
}

void DoINITBUF(char *size_eqn) {

	int		size;

	if (size_eqn == NULL) size = DEFBUFSIZE;
	else size = Calculate(size_eqn);
	if (size <= 0) {
		PrintError("Requested INITBUF size less than 1 byte (%i)\n", size);
	}

	if (size != buffersize) {
		if (imagebuffer != NULL) free(imagebuffer);
		imagebuffer = (char *)malloc(size);
		if (imagebuffer == NULL) {
			PrintError("Can't allocate 0x%X byte buffer\n", size);
		}
		buffersize = size;
	}

	for (destaddr = 0; destaddr < buffersize; destaddr++) imagebuffer[destaddr] = IMAGEFILLCHAR;
	imagesize = 0;
	destaddr = 0;
	PrintLog("Buffer of size 0x%08X (%i) initialized with 0x%02X\n", buffersize, buffersize, IMAGEFILLCHAR);
}

void DoOUTPUT(char *file_str, char *arg1_str, char *arg2_str) {

	int		startindex,
			endindex,
			length,
			sizewritten;

	char		expandname[MAXLINELEN];

	FILE		*filehandle;

	if (file_str == NULL) {
		PrintError("No OUTPUT file specified\n");
	}
	if (strpbrk(file_str, "~$")) {
		strcpy(expandname, file_str);
		ExpandFileName(expandname);
		if (!(expandname)) {
			PrintError("Problem expanding OUTPUT file (%s)\n", file_str);
		}
		file_str = expandname;
	}
	if (!(filehandle = fopen(file_str, "wb"))) {
		PrintError("Problem opening OUTPUT file (%s)\n", file_str);
	}
	startindex = 0;
	endindex = imagesize;
	if (arg1_str != NULL) {
		if (arg2_str == NULL) {
			PrintError("Bad OUTPUT directive format\n");
		}
		if (!strcmp(arg1_str, "START")) startindex = Calculate(arg2_str);
		else if (!strcmp(arg1_str, "END")) endindex = Calculate(arg2_str);
		else {
			startindex = Calculate(arg1_str);
			endindex = Calculate(arg2_str);
		}
	}
	if (startindex < 0 || endindex > imagesize || startindex >= endindex) {
		PrintError("END index overflow (0x%X)\n", endindex);
	}
	length = endindex - startindex;
	sizewritten = fwrite(imagebuffer + startindex, 1, length, filehandle);
	fclose(filehandle);
	if (sizewritten < length) {
		PrintError("Problem writing to OUTPUT file (%s)\n", file_str);
	}
	PrintLog("\nWriting %d bytes from index %d to %s\n\n", length, startindex, file_str);
}

void DoPARSEOFF(void) {
	parseflag = 0;
	PrintLog("Parsing disabled at line %i of %s\n", numlines, recipename);
}

void DoPARSEON(void) {
	parseflag = 1;
	PrintLog("Parsing enabled at line %i of %s\n", numlines, recipename);
}

void DoPOKEBYTE(char *address_eqn, char *byte_eqn) {

	int		address,
			byte;

	address = Calculate(address_eqn);
	if (address > buffersize) {
		PrintError("POKEBYTE overflow (0x%X)\n", address);
	}
	byte = Calculate(byte_eqn);
	*((BYTETYPE *)(imagebuffer + address)) = byte;
	PrintLog("0x%08X   0x00000004   0x%08X   %s\n", address, byte, byte_eqn);
}

void DoPOKEHALF(char *address_eqn, char *halfword_eqn) {

	int		address,
			halfword;

	address = Calculate(address_eqn);
	if (address > buffersize) {
		PrintError("POKEHALF overflow (0x%X)\n", address);
	}
	if (address % 2 != 0) {
		PrintError("POKEWORD address not halfword aligned (0x%X)\n", address);
	}
	halfword = Calculate(halfword_eqn);
	*((HALFWORDTYPE *)(imagebuffer + destaddr)) = halfword;
	PrintLog("0x%08X   0x00000004   0x%08X   %s\n", address, halfword, halfword_eqn);
}

void DoPOKEWORD(char *address_eqn, char *word_eqn) {

	int		address,
			word;

	address = Calculate(address_eqn);
	if (address > buffersize) {
		PrintError("POKEWORD overflow (0x%X)\n", address);
	}
	if (address % 4 != 0) {
		PrintError("POKEWORD address not word aligned (0x%X)\n", address);
	}
	word = Calculate(word_eqn);
	*((WORDTYPE *)(imagebuffer + address)) = word;
	PrintLog("0x%08X   0x00000004   0x%08X   %s\n", address, word, word_eqn);
}

void DoQUIET(void) {
	if (quiet != 2) quiet = 1;
}

void DoREADFILE(char *file_str, char *arg1_str, char *arg2_str) {

	int		filelength,
			startindex,
			endindex,
			length,
			bytesread;

	char		expandname[MAXLINELEN];

	FILE		*filehandle;

	struct stat	fileinfo;

	if (file_str == NULL) {
		PrintError("No READFILE specified\n");
	}
	if (strpbrk(file_str, "~$")) {
		strcpy(expandname, file_str);
		ExpandFileName(expandname);
		if (!(expandname)) {
			PrintError("Problem expanding READFILE (%s)\n", file_str);
		}
		file_str = expandname;
	}
	if (!(filehandle = fopen(file_str, "rb"))) {
		PrintError("Problem opening READFILE (%s)\n", file_str);
	}
	if (stat(file_str, &fileinfo)) {
		PrintError("Problem getting READFILE info (%s)\n", file_str);
	}
	filelength = fileinfo.st_size;
	startindex = 0;
	endindex = filelength;
	if (arg1_str != NULL) {
		if (arg2_str == NULL) {
			PrintError("Bad READFILE directive format\n");
		}
		if (!strcmp(arg1_str, "START")) startindex = Calculate(arg2_str);
		else if (!strcmp(arg1_str, "END")) endindex = Calculate(arg2_str);
		else {
			startindex = Calculate(arg1_str);
			endindex = Calculate(arg2_str);
		}
	}
	if (endindex < 0 || startindex >= endindex) {
		PrintError("END index overflow (0x%X)\n", endindex);
	}
	length = endindex - startindex;
	if (destaddr + length > buffersize) {
		PrintError("READFILE overflow (%s)\n", file_str);
	}
	if (fseek(filehandle, startindex, 0)) {
		PrintError("Problem seeking to READFILE start index (0x%X)\n", startindex);
	}
	bytesread = fread(imagebuffer + destaddr, 1, length, filehandle);
	if (ferror(filehandle) || bytesread != length) {
		PrintError("Problem reading READFILE (%s)\n", file_str);
	}
	fclose(filehandle);
	PrintLog("0x%08X   0x%08X   -readfile-   %s", destaddr, length, file_str);
	if (startindex > 0)		PrintLog(", start index = 0x%X", startindex);
	if (endindex < filelength)	PrintLog(", end index = 0x%X", endindex);
	PrintLog("\n");
	destaddr = destaddr + length;
	if (destaddr > imagesize) imagesize = destaddr;
}

void DoSTRING(char *arg1_str, char *arg2_str) {

	int		length;

	char		string[MAXLINELEN];

	if (!strcmp(arg1_str, "CALC")) sprintf(string, "%i",  Calculate(arg2_str));
	else strcpy(string, arg1_str);
	length = strlen(string);
	if (destaddr + length > buffersize) {
		PrintError("STRING overflow (0x%X)\n", length);
	}
	memcpy(imagebuffer + destaddr, string, length);
	PrintLog("0x%08X   0x%08X   -string-     {%s}\n", destaddr, length, string);
	destaddr = destaddr + length;
	if (destaddr > imagesize) imagesize = destaddr;
}

void DoVARIABLE(char *name_str, char *value_eqn) {

	int	match,
		index;

	if (value_eqn == NULL) {
		PrintError("Bad VARIABLE definition\n");
	} 
	match = 0;
	for (index = 0; (index < numvars) && !match;) {
		if (!strcmp(name_str, varname[index])) match = 1;
		else index++;
	}
	if (!match) {
		if (numvars == MAXVARS) {
			PrintError("Maximum number of variables is %d\n", MAXVARS);
		}
		if (strlen(name_str) >= MAXVARNAME) {
			PrintError("Maximum variable name length is %d\n", MAXVARNAME);
		}
		index = numvars;
		numvars = numvars + 1;
		strcpy(varname[index], name_str);
	}
	varvalue[index] = Calculate(value_eqn);
	PrintLog("Setting variable, %s = 0x%08X (%s)\n", varname[index], varvalue[index], value_eqn);
}

void DoVERBOSE(void) {
	if (quiet != 2) quiet = 0;
}

void DoWORD(char *word_eqn) {

	int		word;

	if (destaddr + 4 > buffersize) {
		PrintError("WORD overflow\n");
	}
	if (destaddr % 4 != 0) {
		PrintError("WORD address not word aligned (0x%X)\n", destaddr);
	}
	word = Calculate(word_eqn);
	*((WORDTYPE *)(imagebuffer + destaddr)) = word;
	PrintLog("0x%08X   0x00000004   0x%08X   %s\n", destaddr, word, word_eqn);
	destaddr = destaddr + 4;
	if (destaddr > imagesize) imagesize = destaddr;
}

main(argc, argv) int argc; char **argv; {

	int		returnvalue;
	char		recipeline[MAXLINELEN],
			commandstring[MAXLINELEN];
	FILE		*recipestream;

	revision = strtok(sccsid, WHITESPACE);
	revision = strtok(NULL, WHITESPACE);
	revision = strtok(NULL, WHITESPACE);
	revision = strtok(NULL, WHITESPACE);

	if (sizeof(BYTETYPE) != 1) {
		PrintError("Byte data type not 1 byte!?\n");
	}
	if (sizeof(HALFWORDTYPE) != 2) {
		PrintError("Halfword data type not 2 bytes!?\n");
	}
	if (sizeof(WORDTYPE) != 4) {
		PrintError("Error, word type not 4 bytes!?\n");
	}

	if (argc < 2) ShowUsage();
	processid = getpid();
	DBUG(("Process ID = %i\n", processid));

	--argc;
	strcpy(recipename, *++argv);
	DBUG(("recipe = (%s), length = %i\n", recipename, strlen(recipename)));
	if (strlen(recipename) >= MAXLINELEN) {
		PrintError("Recipe filename longer than %i bytes (%s)\n", MAXLINELEN, recipename);
	}
	strcpy(commandargs, "");

	while (--argc > 0) {
		DBUG(("command line arg = %s\n", *(argv + 1)));
		if (!strcmp(*++argv, "-l")) logoutput = 1;
		else if (!strcmp(*argv, "-L")) {
			--argc;
			strcpy(logfilename, *++argv);
			if (strlen(logfilename) >= MAXLINELEN) {
				PrintError("Log filename longer than %i bytes (%s)\n", MAXLINELEN, logfilename);
			}
			logoutput = 1;
		} else if (!strcmp(*argv, "-q")) quiet = 1;
		else if (!strcmp(*argv, "-Q")) quiet = 2;
		else {
			sprintf(commandargs, "%s %s", commandargs, *argv);
			if (strlen(commandargs) >= MAXLINELEN) {
				PrintError("Command line args longer than %i bytes (%s)\n", MAXLINELEN, commandargs);
			}
		}
	}
	DBUG(("Logoutput = %i\n", logoutput));
	if (logoutput) {
		if (!(logfile = fopen(logfilename, "w"))) {
			PrintError("Problem opening log file (%s)\n", logfilename);
		} else logoutput = 2;
	}
	ShowCredits();
	PrintLog("Commandline args for cpp pass =%s\n\n", commandargs);
	if (access(recipename, R_OK)) {
		PrintError("Recipe file not readable (%s)\n", recipename);
	}
	DBUG(("Recipe file is ok\n"));

	sprintf(commandstring, "%s %s %s", CPPCOMMAND, commandargs, recipename);
	if (strlen(commandstring) >= MAXLINELEN) {
		PrintError("cpp command longer than %i bytes (%s)\n", MAXLINELEN, commandstring);
	}
	recipestream = popen(commandstring, "r");
	DoINITBUF(NULL);
	PrintLog("Address      Size         Data         Argument\n");
	while (fgets(recipeline, MAXLINELEN, recipestream)) {
		numlines++;
		if (!strchr(recipeline, '\n')) {
			PrintError("Line longer than %i bytes (%s)\n", MAXLINELEN, recipeline);
		}
		ParseDirective(recipeline);
		if (directive == NULL) continue;
		DBUG(("Directive, arg0 = (%s, %s)\n", directive, argument[0]));
		if (!strcmp(directive, "#")) {
			sscanf(argument[0], "%i", &numlines);
			sscanf(argument[1], "\"%[^\"]", recipename);
			numlines--;
		}
		else if (!strcmp(directive, "PARSEON"))   DoPARSEON();
		else if (parseflag != 1);
		else if (!strcmp(directive, "ADDRESS"))   DoADDRESS(argument[0]);
		else if (!strcmp(directive, "ALIGN"))     DoALIGN(argument[0]);
		else if (!strcmp(directive, "BYTE"))      DoBYTE(argument[0]);
		else if (!strcmp(directive, "CMDWARGS"))  DoCMDWARGS(argument[0]);
		else if (!strcmp(directive, "COMMAND"))   DoCOMMAND(argument[0]);
		else if (!strcmp(directive, "HALFWORD"))  DoHALFWORD(argument[0]);
		else if (!strcmp(directive, "INITBUF"))   DoINITBUF(argument[0]);
		else if (!strcmp(directive, "OUTPUT"))    DoOUTPUT(argument[0], argument[1], argument[2]);
		else if (!strcmp(directive, "PARSEOFF"))  DoPARSEOFF();
		else if (!strcmp(directive, "POKEBYTE"))  DoPOKEBYTE(argument[0], argument[1]);
		else if (!strcmp(directive, "POKEHALF"))  DoPOKEHALF(argument[0], argument[1]);
		else if (!strcmp(directive, "POKEWORD"))  DoPOKEWORD(argument[0], argument[1]);
		else if (!strcmp(directive, "QUIET"))     DoQUIET();
		else if (!strcmp(directive, "READFILE"))  DoREADFILE(argument[0], argument[1], argument[2]);
		else if (!strcmp(directive, "STRING"))    DoSTRING(argument[0], argument[1]);
		else if (!strcmp(directive, "VARIABLE"))  DoVARIABLE(argument[0], argument[1]);
		else if (!strcmp(directive, "VERBOSE"))   DoVERBOSE();
		else if (!strcmp(directive, "WORD"))      DoWORD(argument[0]);
		else {
			PrintError("Unknown directive (%s)\n", directive);
		}
	}
	returnvalue = pclose(recipestream);
	if (returnvalue > 0) {
		printf("returnvalue = %i\n", returnvalue);
		PrintError("Problem running cpp command (%s)\n", commandstring);
	}
	numlines = 0;
	PrintLog("cook exiting with no errors\n\n");
	exit(NOERRS);
}

