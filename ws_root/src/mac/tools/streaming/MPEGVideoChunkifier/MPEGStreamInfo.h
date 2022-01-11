/******************************************************************************
**
**  @(#) MPEGStreamInfo.h 96/03/29 1.1
**
******************************************************************************/

/****************************************************************************
*																			*
*	File:		MPEGStreamInfo.h											*
*				Version 1.0													*
*				MPEG chunkifier 											*
*	Date:		09-13-1995													*
*	Written by:	Shahriar Vaghar 											*
*																			*
****************************************************************************/

#ifndef __TMPEGSTREAMINFO__
#define __TMPEGSTREAMINFO__

class TMPEGStreamInfo {
public:
	
	// Constructor.
	TMPEGStreamInfo();
	
	// Destructor.
	~TMPEGStreamInfo();

	// Returns the present distance between reference frames (I/P)
	// in a MPEG bitstream. For example if the current picture is an
	// I frame and its temporal reference is 2, then the next reference
	// frame is 3 pictures away. Hence the distance is 3.
	// NOTE: This assumes temporal references for each picture in the
	//        MPEG stream is valid.
	int ReferenceFramesDist() const { return fRefFrameDistance; }
	
	// Sets the current picture type.
	// NOTE/WARNING: TemporalRef must be called before PictureType.
	void PictureType(int aType);

	// Sets the temporal reference number of the current picture.
	// NOTE/WARNING: TemporalRef must be called before PictureType.
	void TemporalRef(int aRef);
										
	// Increments fFrameCount by one. Keeps track of total number of
	// MPEG pictures chunkified. Starts at zero. Should be called after
	// picture is chunkified.
	void IncrementFrameCount();
	
	// Increments fGOPFrameCount by one. Total number of MPEG pictures
	// since the beginning of a Group Of Pictures. fGOPFrameCount will
	// be reset to zero at the beginning of a GOP. Should be called after
	// the picture is chunkified.
	void IncrementGOPFrameCount();	
	
	// Printfs data members to the output.
	void Dump();
	
private:
	int fFrameCount;				// Number of frames chunkified
	int fCurrentPictureType;		// Current MPEG picture type.
	int fCurrentTemporalRef;		// Current MPEG picture temporal reference.
	int fRefFrameDistance;			// Number of frames between reference frames (I or P).
	int fGOPFrameCount;				// Number of frames since beginning of a GOP.
};



inline void
TMPEGStreamInfo::IncrementFrameCount()
{
	fFrameCount++;
}

inline void
TMPEGStreamInfo::IncrementGOPFrameCount()
{
	fGOPFrameCount++;
}

inline void
TMPEGStreamInfo::TemporalRef(int aRef)
{
	// NOTE/WARNING: TemporalRef must be called before PictureType.
	
	fCurrentTemporalRef = aRef;
}

#endif /* __TMPEGSTREAMINFO__ */

