/*
 * rsasign
 * @(#) rsasign.c 95/07/20 1.3
 *
 * RSA-sign a file.
 *
 * rsasign [-k key] [-s # -e #]... [-p] [-v] filename
 *
 *	Generates an RSA signature for the named file.
 *	If -p is specified, the signature is written to standard output.
 *	Otherwise, the signature is appended to the file.
 *	-s specifies the starting position within the file of the data to sign.
 *	-e specifies the ending position within the file of the data to sign.
 *	Several -s,-e pairs may be specified; this specifies a 
 *	scattered area to sign.
 *	-k specifies the key to use.  Current keys supported are:
 *		opera	Opera 64-byte key.
 *		m2	M2 128-byte key.
 *	-v specifies verbose mode.  Dumps all the data being signed.
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "global.h"
#include "bsafe2.h"
#include "demochos.h"
#include "demoutil.h"
#include "keydesc.h"

typedef struct Area
{
	struct Area *next;
	long startp;
	long endp;
} Area;

#define NULL_SURRENDER_PTR ((A_SURRENDER_CTX *)NULL_PTR)
#define	min(a,b)	(((a) < (b)) ? (a) : (b))

static void PrintError(char *, int);

B_ALGORITHM_OBJ randomAlgo;

extern KeyDesc KeyDescs[];
extern int NumKeys;

/*
 * Print usage message and exit.
 */
static void 
usage(void)
{
	fprintf(stderr, "usage: rsasign [-k key] [-s # -e #]... filename\n");
	fprintf(stderr, "   or: rsasign [-k key] [-s # -e #]... -p filename >sigfile\n");
	exit(1);
}

/*
 * Initialize global variables.
 */
static int 
InitGlobals(void)
{
	int status;

	if (status = B_CreateAlgorithmObject(&randomAlgo))
	{
		PrintError("creating algorithm", status);
		return status;
	}
        if (status = B_SetAlgorithmInfo(randomAlgo, AI_MD2Random, NULL_PTR))
	{
        	PrintError("setting algorithm", status);
		return status;
	}
        if (status = B_RandomInit(randomAlgo, DEMO_ALGORITHM_CHOOSER,
                       (A_SURRENDER_CTX *)NULL_PTR))
	{
		PrintError("initializing random", status);
		return status;
	}
	return 0;
}

/*
 * Initialize a key.  Return public and private key objects.
 */
	int
InitKey(KeyDesc *k, B_KEY_OBJ *publicKey, B_KEY_OBJ *privateKey)
{
	int status;

        if (status = B_CreateKeyObject(publicKey))
	{
		PrintError("creating public key", status);
		return status;
	}
        if (status = B_SetKeyInfo(*publicKey, KI_RSAPublicBER, 
		(POINTER)&k->key_PublicKey))
	{
		PrintError("setting public key64", status);
		return status;
	}

        if (status = B_CreateKeyObject(privateKey))
	{
		PrintError("creating private key", status);
		return status;
	}
        if (status = B_SetKeyInfo(*privateKey, KI_PKCS_RSAPrivateBER,
               (POINTER)&k->key_PrivateKey))
	{
		PrintError("setting private key64", status);
		return status;
	}
	return 0;
}

/*
 * Begin signing.
 */
	int
SignInit(B_KEY_OBJ privKey, void **pContext)
{
	int status;
	B_ALGORITHM_OBJ algo;

	if (status = B_CreateAlgorithmObject(&algo))
		return status;
	if (status = B_SetAlgorithmInfo(algo, 
			AI_MD5WithRSAEncryption, NULL_PTR))
	{
		B_DestroyAlgorithmObject(&algo);
		return status;
	}
	if (status = B_SignInit(algo, privKey, 
			DEMO_ALGORITHM_CHOOSER, NULL_SURRENDER_PTR))
	{
		B_DestroyAlgorithmObject(&algo);
		return status;
	}
	*pContext = algo;
	return 0;
}

/*
 * Add some more data to the data being signed.
 */
	int
SignUpdate(void *context, void *data, unsigned int dataLen)
{
	int status;
	B_ALGORITHM_OBJ algo = context;

	if (status = B_SignUpdate(algo, data, dataLen, NULL_SURRENDER_PTR))
		return status;
	return 0;
}

/*
 * Finish signing data, and return the signature.
 */
	int
SignFinal(void *context, unsigned char *sig, unsigned int *pSigLen, unsigned int maxSigLen)
{
	int status;
	B_ALGORITHM_OBJ algo = context;

	if (status = B_SignFinal(algo, sig, pSigLen, maxSigLen, 
				NULL_PTR, NULL_SURRENDER_PTR))
		return status;
	return 0;
}

void
DumpData(unsigned char *buffer, int size)
{
	int i;

	for (i = 0;  i < size;  i++)
	{
		fprintf(stderr, "%02x ", buffer[i]);
		if ((i % 16) == 15)  fprintf(stderr, "\n");
	}
	if ((size % 16) != 0)
		fprintf(stderr, "\n");
}

/*
 * Sign a file.
 */
static int
SignFile(char *filename, B_KEY_OBJ key, Area *areas, int append, int verbose)
{
	FILE *file;
	int status;
	long pos;
	int size;
	Area *area;
	void *context;
	unsigned char sig[256];
	unsigned int sigLen;
	unsigned char buffer[1024];

	static Area defaultAreas = { NULL, 0, -1 };

	if ((file = fopen(filename, "rb")) == NULL)
	{
		PrintError("opening input file", 0);
		return -1;
	}

	if (status = SignInit(key, &context))
	{
		PrintError("SignInit", status);
		return status;
	}

	if (areas == NULL)
		areas = &defaultAreas;
	for (area = areas;  area != NULL;  area = area->next)
	{
		fseek(file, area->startp, 0);
		pos = area->startp;
		while (pos < area->endp || area->endp == -1)
		{
			size = min(area->endp - pos, sizeof(buffer));
			size = fread(buffer, 1, size, file);
			if (size <= 0)
				break;
			pos += size;
			if (verbose)
				DumpData(buffer, size);
			if (status = SignUpdate(context, buffer, size))
			{
				PrintError("SignUpdate", status);
				return status;
			}
		}
	}
	fclose(file);
	if (status = SignFinal(context, sig, &sigLen, sizeof(sig)))
	{
		PrintError("SignFinal", status);
		return status;
	}

	if (append)
	{
		/* Append the signature to the file. */
		file = fopen(filename, "a");
		if (file == NULL)
		{
			PrintError("opening file to append signature", 0);
			return -1;
		}
		size = fwrite(sig, 1, sigLen, file);
	} else
	{
		/* Write the signature to standard output. */
		size = fwrite(sig, 1, sigLen, stdout);
	}

	if (size != sigLen)
	{
		PrintError("appending signature", 0);
		return -1;
	}
	return 0;
}

/*
 * Return the error string for the type, or (char *)NULL_PTR if the type
 * is not recognized.
 */
static char *
BSAFE2_ErrorString(int type)
{
	switch (type)
	{
	case BE_ALGORITHM_ALREADY_SET:
		return ("Algorithm object has already been set with algorithm info");
	case BE_ALGORITHM_INFO:
		return ("Invalid algorithm info format");
	case BE_ALGORITHM_NOT_INITIALIZED:
		return ("Algorithm object has not been initialized");
	case BE_ALGORITHM_NOT_SET:
		return ("Algorithm object has not been set with algorithm info");
	case BE_ALGORITHM_OBJ:
		return ("Invalid algorithm object");
	case BE_ALG_OPERATION_UNKNOWN:
		return ("Unknown operation for an algorithm or algorithm info type");
	case BE_ALLOC:
		return ("Insufficient memory");
	case BE_CANCEL:
		return ("Operation was cancelled by the surrender function");
	case BE_DATA:
		return ("Generic data error");
	case BE_EXPONENT_EVEN:
		return ("Invalid even value for public exponent in keypair generation");
	case BE_EXPONENT_LEN:
		return
		  ("Invalid exponent length for public exponent in keypair generation");
	case BE_HARDWARE:
		return ("Cryptographic hardware error");
	case BE_INPUT_DATA:
		return ("Invalid format for input data");
	case BE_INPUT_LEN:
		return ("Invalid length for input data");
	case BE_KEY_ALREADY_SET:
		return ("Key object has already been set with key info");
	case BE_KEY_INFO:
		return ("Invalid key info format");
	case BE_KEY_NOT_SET:
		return ("Key object has not been set with key info");
	case BE_KEY_OBJ:
		return ("Invalid key object");
	case BE_KEY_OPERATION_UNKNOWN:
		return ("Unknown operation for a key info type");
	case BE_MEMORY_OBJ:
		return ("Invalid internal memory object");
	case BE_MODULUS_LEN:
		return ("Invalid modulus length in public or private key");
	case BE_NOT_SUPPORTED:
		return ("Unsupported operation requested");
	case BE_OUTPUT_LEN:
		return ("Output data is larger than supplied buffer");
	case BE_OVER_32K:
		return ("Data block exceeds 32,767 bytes");
	case BE_RANDOM_NOT_INITIALIZED:
		return ("Random algorithm has not been initialized");
	case BE_RANDOM_OBJ:
		return ("Invalid algorithm object for the random algorithm");
	case BE_SIGNATURE:
		return ("Invalid signature");
	case BE_WRONG_ALGORITHM_INFO:
		return ("Wrong type of algorithm info");
	case BE_WRONG_KEY_INFO:
		return ("Wrong type of key info");
	default:
		return ((char *)NULL_PTR);
	}
}

/*
 *  If type is zero, simply print the task string, otherwise convert the
 *  type to a string and print task and type.
 */
static void 
PrintError(char *task, int type)
{
	char *typeString, buf[80];

	if (type == 0)
	{
		fprintf(stderr, "ERROR while %s\n", task);
		return;
	}

	/*
	 *  Convert the type to a string if it is recognized.
	 */
	if ((typeString = BSAFE2_ErrorString(type)) == (char *)NULL_PTR)
	{
		sprintf(buf, "Code 0x%04x", type);
		typeString = buf;
	}
	fprintf(stderr, "ERROR: %s while %s\n", typeString, task);  
}

void
AddArea(Area **list, long startp, long endp)
{
	Area *area;
	Area *lastarea;

	lastarea = NULL;
	for (area = *list;  area != NULL;  lastarea = area, area = area->next)
		continue;
	if (lastarea == NULL || 
	    (lastarea->startp != -1 && lastarea->endp != -1))
	{
		area = (Area *) malloc(sizeof(Area));
		area->next = NULL;
		area->startp = area->endp = -1;
		if (lastarea == NULL)
			*list = area;
		else
			lastarea->next = area;
		lastarea = area;
	}
	if (startp != -1)
	{
		if (lastarea->startp != -1)
		{
			fprintf(stderr, "Two -s without -e is illegal\n");
			exit(1);
		}
		lastarea->startp = startp;
	}
	if (endp != -1)
	{
		if (lastarea->endp != -1)
		{
			fprintf(stderr, "Two -e without -s is illegal\n");
			exit(1);
		}
		lastarea->endp = endp;
	}
}


int 
main(int argc, char *argv[])
{
	B_KEY_OBJ publicKey, privateKey;
	KeyDesc *k;
	int ch;
	Area *areas;
	char *whichkey = "m2";
	int append;
	int verbose;

	extern int optind;
	extern char *optarg;

	areas = NULL;
	append = 1;
	verbose = 0;
	while ((ch = getopt(argc, argv, "e:k:ps:v")) != -1)
		switch (ch)
		{
		case 'e':
			AddArea(&areas, -1, strtol(optarg, NULL, 0));
			break;
		case 'k':
			whichkey = optarg;
			break;
		case 'p':
			append = 0;
			break;
		case 's':
			AddArea(&areas, strtol(optarg, NULL, 0), -1);
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
		}

	if (optind != argc-1)
		usage();

	for (k = KeyDescs;  k < &KeyDescs[NumKeys];  k++)
	{
		if (strcmp(whichkey, k->key_Name) == 0)
			break;
	}
	if (k >= &KeyDescs[NumKeys])
	{
		fprintf(stderr, "\"-k %s\" specifies unknown key\n", whichkey);
		return 1;
	}

	if (InitGlobals())
		return 1;
	if (InitKey(k, &publicKey, &privateKey))
		return 1;
	if (SignFile(argv[optind], privateKey, areas, append, verbose))
		return 1;
	return 0;
}

