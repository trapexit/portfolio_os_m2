#ifndef	__ACROFS_H
#define	__ACROFS_H


/******************************************************************************
**
**  @(#) acrofs.h 96/05/10 1.22
**
******************************************************************************/


#ifdef	EXTERNAL_RELEASE
/* cause an error if this file gets included somehow */
#include "THIS_FILE_IS_3DO-INTERNAL,_AND_IS_NOT_FOR_THE_USE_OF_DEVELOPERS"
#endif

#ifndef	__KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __FILE_FILESYSTEM_H
#include <file/filesystem.h>
#endif

#ifdef	BUILD_STRINGS
#define ACRO_INFO(x)	Superkprintf x
#else	/* BUILD_STRINGS */
#define ACRO_INFO(x)	/* x */
#endif	/* BUILD_STRINGS */

/*
 *	kernel data structures for acrobat filesystem
 */

#ifdef	SUPER
#define FSBLK2BYT(fsp, fblks)	((fblks) * (fsp)->fs_VolumeBlockSize)
#define BYT2FSBLK(fsp, byts)	((byts) / (fsp)->fs_VolumeBlockSize)
#define FSBLK2DEVBLK(fsp, fblks)	\
			((fblks) * (fsp)->fs_DeviceBlocksPerFilesystemBlock)
#define DEVBLK2FSBLK(fsp, dblks)	\
			((dblks) / (fsp)->fs_DeviceBlocksPerFilesystemBlock)
#define	DEVBLK2BYT(fsp, dblks)	\
			(FSBLK2BYT(fsp, DEVBLK2FSBLK(fsp, dblks)))
#define	BYT2DEVBLK(fsp, byts)	\
			(FSBLK2DEVBLK(fsp, BYT2FSBLK(fsp, byts)))
#define	RESINDX_2INUM(mfsp, resin)	\
			((mfsp)->mf_dfsp->df_inums[resin])

#define	RESINDX_2IP(mfsp, resin)	((mfsp)->mf_metaip[resin])

#define	INODES_PER_IBLK(fsp, ifip)	\
			(ifip->di_fbsize / \
			 ((mfs_t *) (fsp))->mf_dfsp->df_isize)
#define	WHICH_FBLK(mfsp, ifip, inum)	\
		((inum) / INODES_PER_IBLK(mfsp, ifip))
#define	WHICH_FOFFSET(mfsp, ifip, inum)	\
	(((inum) % INODES_PER_IBLK(mfsp, ifip)) * (mfsp)->mf_dfsp->df_isize)

#define	UNIQ2INUM(uniq)		((uniq) - 1)
#define	INUM2UNIQ(inum)		((inum) + 1)
#define ACRO_INUM(fp)		(UNIQ2INUM((fp)->fi_UniqueIdentifier))

#endif	/* SUPER */


/*
 *	common bitmap stuff
 */
#define	INUSE_BIT		1	/* not to change arbitrarily	*/
#define	FREE_BIT		0
#define	BYTE_FULL		0xff

#define	POWEROF2(i)		(((i) & ((i) - 1)) == 0)
#define MIN(a,b)	        ((a < b)? a: b)
#define MAX(a,b)	        ((a > b)? a: b)
#define HOWMANY(sz, unt)        ((sz + (unt - 1)) / unt)
#define ROUNDDOWN(sz, unt)      ((sz + (unt)) * unt)
#define	BYTE_ROUNDUP(n)		(((n) + BYTE_MASK) & ~BYTE_MASK)
#define	BITS_PER_BYTE		8
#define	BITS_PER_BYTE_SHIFT	3
#define	BYTE_MASK		(BITS_PER_BYTE - 1)
#define	BIT_PATTERN(n)		(1 << (BYTE_MASK - ((n) & BYTE_MASK)))
#define	WHICH_BYTE(n)		((n) >> BITS_PER_BYTE_SHIFT)
#define IS_BIT_INUSE(mstart, n) \
			((*((mstart) + WHICH_BYTE((n)))) & BIT_PATTERN((n)))
#define	MARK_INUSE(mstart, n)	\
			(*((mstart) + WHICH_BYTE((n))) |= BIT_PATTERN((n)))
#define	MARK_FREE(mstart, n)	\
			(*((mstart) + WHICH_BYTE((n))) &= ~(BIT_PATTERN((n))))

/*
 *	single layer bitmap specifics
 */
#define	SMAPSZ_INBYTES(bits)	((bits) >> BITS_PER_BYTE_SHIFT)
#define	SMAPSZ_INBITS(byts)	((byts) << BITS_PER_BYTE_SHIFT)


/*
 *	multi layer bitmap specifics
 */
#define	MYBUD(n)		\
		(((n) & 1)? ((n) & ((uint32) - 2)): ((n) | 1))
#define	BITS_IN_LAYER(msize, layer)	\
				((msize) >> (layer))
#define	BLKS_PER_BIT(layer)	(1 << (layer))
#define	MMAPSZ_INBYTES(blks)	((blks) >> (BITS_PER_BYTE_SHIFT - 1))



/*
 *	acrobat general defines
 */
#define	ACRO_DI_EXTENTS		4	/* may actually be more		*/
#define	ACRO_DF_MAGIC		0x6F08C5A7
#define	ACRO_DF_VERS		0x101

#define	ALLOC_CONTIG		1	/* contiguous blocks only	*/
#define	ALLOC_BESTFIT		2	/* minimize fragmentation	*/
#define	ALLOC_MINSEEK		3	/* best proximity		*/

#define	ACRO_MAX_EXTENT_LIST	8
#define	ACRO_PLIST_MAX 		20	/* max number of cache counts held */
#define	ACRO_IGROW_RATE		16	/* ifile grows by this many inodes */


/*
 *	fixed inumbers for meta data stuff, only at format time
 *	they may get relocated at a later time.
 *	do not change the order arbitrarily.
 */
#define	ACRO_INDX_BLANK		((uint32) -1)
#define	ACRO_INDX_IAREA		0	/* must be 0 at format time or on   */
					/* fs block boundary at other times */
#define	ACRO_INDX_SUPER		1
#define	ACRO_INDX_COMMAP	2
#define	ACRO_INDX_IMAP		3
#define	ACRO_INDX_LOG		4
#define	ACRO_INDX_ROOT		5
#define	ACRO_INDX_MAXRESERVE	6	/* must be the last one		*/

#define	ACRO_NAME_FREE		"_sys_ifree"
#define	ACRO_NAME_IAREA		"_sys_iarea"
#define	ACRO_NAME_SUPER		"_sys_super"
#define	ACRO_NAME_COMMAP	"_sys_commap"
#define	ACRO_NAME_IMAP		"_sys_imap"
#define	ACRO_NAME_LOG		"_sys_log"
#define	ACRO_NAME_ROOT		"_sys_root"

/*	mount states	*/
enum mt_states {
	inactive,
	intrans,
	active,
	failed
};

/*
 *	extent information
 */
typedef	struct	extinfo {
	uint32	xi_addr;	/* fs block number of extent		*/
	uint16	xi_size;	/* size of extent in fs block size	*/
	uint16	xi_type;	/* see bits below			*/
} extinfo_t;

/*
 *	bits for xi_type
 */
#define	ACRO_XI_DIRECT		0x1

/*
 *	device resident inode structure
 */
typedef	struct	dino {
	char		di_name[FILESYSTEM_MAX_NAME_LEN];
					      /* 0x00: file/dir name	*/
	uint32		di_type;	      /* 0x20: opera compat	*/
	uint32		di_flag;	      /* 0x24: see bits below	*/
	uint32		di_res;	      	      /* 0x28: for future use   */
	uint32		di_fsbcnt;	      /* 0x2c: in fs bsize	*/
	uint32		di_byts;	      /* 0x30: file size in byts     */
	uint32		di_fbsize;	      /* 0x34: byts, file block size */
	extinfo_t	di_ext[ACRO_DI_EXTENTS];  /* 0x38: actually longer   */
} dino_t;

/*
 *	bits for di_flags
 */
#define	ACRO_DI_META		0x01	/* inode holds meta data	*/
#define	ACRO_DI_DIR		0x02	/* inode is a directory		*/
#define	ACRO_DI_FREE		0x04	/* free inode, for non IMAP	*/
#define	ACRO_DI_STATIC		0x08	/* file is not to be moved	*/

/*
 *	acro operation vector and cache reservation count
 */
typedef	struct	aops {
	uint32	ao_cmd;		/* device cmd number			*/
				/* function to op handler		*/
	Err	(*ao_func) (FileSystem *fsp, FileIOReq *reqp);
	uint32	ao_maxres;	/* max cahe pages ever needed for an op */
} aops_t;


/*
 *	device filesystem struct is mostly static info about fs and
 *	describes the geometry of the filesystem
 */
typedef	struct	dfs {
	uint32		df_version;	/* version info			     */
	uint32		df_flag;	/* see bits below		     */
	uint32		df_mcnt;	/* mount count for this fs	     */
	uint32		df_bsize;	/* byts, fs logical block size	     */
	uint32		df_bcount;	/* fs size in fs blocks		     */
	uint16		df_isize;	/* byts, device inode size	     */
	uint16		df_lbcnt;	/* blks, logarea size 		     */
	uint16		df_dirsz;	/* min dir ents per dir block	     */
	uint16		df_indirsz;	/* indirect extent holds at least    */
					/* this many extent info entries     */
	uint32		df_nextents;	/* total number of extents in inode  */
	uint32		df_ibnum;	/* fs blk num of inode of inode file */
	uint32		df_lbnum;	/* fs block number of log area	     */
	uint32		df_reserved[4];	/* reserved for future use	     */
	uint32		df_inums[ACRO_INDX_MAXRESERVE];
	TimeVal		df_createtm;	/* fs creation time ticks	     */
	TimeVal		df_reorgtm;	/* last fs reorganization time ticks */
	uint32		df_res[4];	/* reserved for future use	     */
	uint32		df_magic;	/* MUST BE THE LAST STATIC FIELD     */
} dfs_t;

/*
 *	bits for dfs_flag
 */
#define	ACRO_DF_CLEAN		0x01
#define	ACRO_DF_LOGAVAIL	0x02
#define	ACRO_DF_IMAPAVAIL	0x04
#define	ACRO_DF_MORTAL		0x08


/*
 *	logging macros and data types
 */
#define	ACRO_DT_VARY		1
#define	ACRO_LOG_FIXHDR		(sizeof(dtoc_t) - (sizeof(uint32)*ACRO_DT_VARY))
#define ACRO_LOG_HDRSZ(nblks)	(ACRO_LOG_FIXHDR + ((nblks - 1) * \
						    sizeof(uint32)))
#define ACRO_LOG_HDRBLKS(fsp, nblks)	\
				(HOWMANY(ACRO_LOG_HDRSZ(nblks), \
					 (fsp)->fs_VolumeBlockSize))
#define	ACRO_LOG_ENDS(dfsp)	 (dfsp->df_lbnum + dfsp->df_lbcnt)
#define	ACRO_LOG_TAILHDR(dfsp, hdrblks)	\
				(ACRO_LOG_ENDS(dfsp) - (hdrblks))
#define	ACRO_LOG_ZIG(mtp)	((mtp)->mt_flg |= ACRO_MT_ZIG)
#define	ACRO_LOG_ZAG(mtp)	((mtp)->mt_flg &= ~ACRO_MT_ZIG)
#define	ACRO_LOG_ISZIG(mtp)	((mtp)->mt_flg & ACRO_MT_ZIG)
#define	ACRO_LOG_ISZAG(mtp)	(!((mtp)->mt_flg & ACRO_MT_ZIG))
#define	ACRO_LOG_BLKNUM(mtp, i)		\
				(((mtp)->mt_flg & ACRO_MT_ZIG)? \
				 ((mtp)->mt_startbnum + (i)): \
				 ((mtp)->mt_startbnum - (i)))
#define	ACRO_LOG_HDRBLKNUM(dfps, mtp)	\
				(((mtp)->mt_flg & ACRO_MT_ZIG)? \
				 dfsp->df_lbnum: \
				 ACRO_LOG_TAILHDR(dfsp, (mtp)->mt_hdrblks))

/*
 *	device toc
 */
typedef struct dtoc {
	uint32	dt_nblk;		/* # of valid entries in dstblk list */
	uint32	dt_ser;			/* log serial number		     */
	uint32	dt_dstblk[ACRO_DT_VARY]; /* logged blks, actually longer     */
} dtoc_t;

/*
 *	incore toc
 */
typedef struct mtoc {
	uint16	mt_stat;		/* state of the log: defined below */
	uint16	mt_flg;			/* see bits below		   */
	uint32	mt_hdrblks;		/* size of the header in block	   */
	uint32	mt_startbnum;		/* blk # of first usable blk	   */
	uint32	mt_totblks;		/* # of usable blocks in log	   */
	dtoc_t	*mt;			/* device toc			   */
} mtoc_t;


/*
 *	status of the log
 */
#define	ACRO_MT_LOGREADY	1	/* log is ready to be opened 	*/
#define	ACRO_MT_LOGOPEN		2	/* log in progress		*/
#define	ACRO_MT_LOGCLOSED	3	/* log is being flushed		*/


/*
 *	bits for log flag
 */
#define	ACRO_MT_ZIG		0x1	/* position of zigzag log 	*/


/*
 *	incore filesystem struct
 */
typedef	struct	mfs {
	FileSystem	mf_fs;
	uint32		mf_flag;	/* see bits below		*/
	uint32		mf_devbsize;	/* device block size mounted on	*/
	uint32		mf_devboff;	/* device block offset mounted on */
	IOReq		*mf_rawp;	/* raw device ioreq		*/
	mtoc_t		mf_log;		/* logging information		*/
	uint32		mf_startsig;	/* thread start signal		*/
	uchar		mf_niperi;	/* number of inodes per iblk	*/
	uchar		mf_filler[3];	/* reserved			*/
	dino_t		*mf_metaip[ACRO_INDX_MAXRESERVE];
	OFile		*mf_metafp[ACRO_INDX_MAXRESERVE];
	char		*mf_imap;	/* incore inode bitmap		*/
	uint32		mf_imapsz;	/* size of incore imap		*/
	char		*mf_commap;	/* incore inode commap		*/
	dfs_t		*mf_dfsp;	/* device super block		*/
	enum mt_states	mf_mtstate;	/* mount state 			*/
	DiscLabel	*mf_dlp;	/* temp place holder for label  */
	Err		mf_mterr;	/* mount error code		*/
} mfs_t;


/*
 *	bits for incore filesystem struct flag
 */
#define	ACRO_MF_LOGGING		0x1

/*
 *	directory entry
 */
typedef struct	dent {
	uint32	de_nhash;
	uint32	de_inum;
} dent_t;


#endif	/* __ACROFS_H */
