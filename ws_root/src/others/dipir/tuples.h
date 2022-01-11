/*
 * @(#) tuples.h 95/10/19 1.2
 *
 * Definitions of PCMCIA tuples.
 */

#define	MAX_TUPLES	64	/* The 3DO tuple will be before this many. */

#define	CISTPL_NULL	0x00	/* The Null tuple. */
#define	CISTPL_VERS_1	0x15	/* The Product Version tuple. */
#define	CISTPL_END	0xFF	/* The End of tuple chain marker */
