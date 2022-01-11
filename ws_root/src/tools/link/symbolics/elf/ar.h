// @(#) ar.h 96/07/25 1.6
// @(#) ar.h 95/06/26 1.4
#ifndef __AR_H__
#define __AR_H__
//ar - archive file format (copied from unix spec)

/* AR header */
#define ARMAG "!<arch>\x0a"	//magic string
#define SARMAG 8
#define ARFMAG "`\x0a"	//header trailer string
struct ar_hdr {
	char ar_name[16];
	char ar_date[12];
	char ar_uid[6];
	char ar_gid[6];
	char ar_mode[8];
	char ar_size[10];
	char ar_fmag[2];
	};

#endif  /* __AR_H__ */

