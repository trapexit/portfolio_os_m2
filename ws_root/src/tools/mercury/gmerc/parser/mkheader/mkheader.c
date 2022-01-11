#include <stdio.h>
#include <stdlib.h>
#include "../parsertypes.h"
#include "../parsererrors.h"
#include "../parser.h"

void CleanUp(int err);
extern FILE *outFile;
Symbol *symbols = NULL;
extern Syntax File;

int main(int argc, char *argv[])
{
    Err err = NO_ERROR;

    outFile = fopen("sdftokens.h", "w");
    if (outFile == NULL)
    {
        printf("Could not create sdftokens.h\n");
        exit(0);
    }

    /* InitParser() will actually create header file in this case */
    err = InitParser(&File);
    
    printf("finished with error %ld\n", err);
    CleanUp(err);
}

void CleanUp(int err)
{
    fclose(outFile);
    DeleteParser();
    printf("exit(%ld)\n", err);
    exit(err);
}

