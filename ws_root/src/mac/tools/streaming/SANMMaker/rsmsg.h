#####################################
##
##	@(#) rsmsg.h 95/05/26 1.1
##
#####################################
#
#define tccbHdr "CCB "
#define tPLUT "PLUT"
#define tpixels "PDAT"

OSErr	ReadAChunk( Int16 v3DORefNum,char *chunkType, void **buffer);
