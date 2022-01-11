/* @(#) drive.h 95/09/18 1.1 */

/* Seek.h */

/* This file defines two macros which are the lowest level at which seek 
 * testing needs to access the drive. 
 *
 * The first call is MyCDOpen. This expects two int arguments. The first is a
 * boolean TRUE for double speed mode, and the second is an int to indicate one
 * of three sector formats for accessing the disc. MyCDDA, and MyCDG are both
 * for audio testing, and are for sectors which return only the 2352 bytes of
 * audio data, or the 2352 bytes of audio plus the 98 bytes of subcode 
 * information respectively.
 *
 * Note that the Open call must be made with a disc in the drive, and the 
 * number of blocks on the disc is returned.
 *
 * The second call is my cd read. This expects a pointer to a buffer to store 
 * data, a sector number, and the number of sectors to be read. The buffer size
 * must be big enough to handle the total number of sectors requested. 
 */


#define MyCDOpen(fast, type, DeviceName)	\
		CDKludge(0, DeviceName, fast, type)
#define MyCDRead(buf, sector, length)		\
		CDKludge(1, buf, sector, length)

int CDKludge(int operation, unsigned char *buf, int sector, int length);

#define MyCDG	(0)
#define MyCDDA	(1)
#define MyCDROM	(2)
