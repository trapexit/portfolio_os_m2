/* @(#) mytime.c 95/12/05 1.2 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/device.h>
#include <kernel/time.h>
#include <strings.h>
#include <stdio.h>

void PrintfSysErr(Item);
void exit(int);

#define CHK4ERR(err, str)	\
		if ((err) < 0) { printf (str); PrintfSysErr(err); exit(9); }

uint32 MyTime(void)
{
	Item	IOReqItem;
	IOInfo	ioInfo;
	int32	ret;
	uint32	mytime;
	struct timeval tv;
	
	IOReqItem = CreateTimerIOReq();
	CHK4ERR(IOReqItem, "Could not create ioReq\n");
	
	memset(&ioInfo, 0, sizeof(ioInfo));
	
	ioInfo.ioi_Command = TIMERCMD_GETTIME_USEC;
	ioInfo.ioi_Recv.iob_Buffer = &tv;
	ioInfo.ioi_Recv.iob_Len = sizeof(tv);

	ret = DoIO(IOReqItem, &ioInfo);
	CHK4ERR(ret, "Error in DoIO()\n");

	mytime = (tv.tv_sec * 1000000) + tv.tv_usec;

	DeleteTimerIOReq(IOReqItem);

	return (mytime);
}
