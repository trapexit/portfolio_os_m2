#include "bench.h"
#include <kernel/time.h>

float dtime(int initialize)
{
    TimeVal stop, d;
    float retval;
    static TimeVal start;

    if( initialize ) {
        retval=0.0;
        SampleSystemTimeTV( &start );
    } else {
        SampleSystemTimeTV( &stop );
	SubTimes( &start, &stop, &d );
	retval = (float)d.tv_Seconds + (float)d.tv_Microseconds/1000000.0;
    }
    return( retval );
}
