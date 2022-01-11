#include <kernel/types.h>
#ifdef TESTDCBI
#include <hardware/PPCasm.h>

void TestDCache(void)
{
	register volatile uint32 *seg1 = (uint32 *)0x400d2800;
	register volatile uint32 *seg2 = (uint32 *)0x40152da0;
	register volatile uint32 *seg3 = (uint32 *)0x401d2000;
	register p1,p2,p3;
	register q1,q2,q3;
	register qwe;
	p1 = 0xdead2800;
	p2 = 0xbeef2da0;
	p3 = 0xf00d2000;
	q1 = 0x11112800;
	q2 = 0x22222da0;
	q3 = 0x33332000;

	/* first flush the cache */

	qwe = seg1[0x1000];
	qwe = seg2[0x1000];
	qwe = seg3[0x1000];

	_sync();

	*seg1 = p1;
	*seg2 = p2;
	*seg3 = p3;

	_dcbf(seg1);
	_dcbf(seg2);
	_dcbf(seg3);

	_sync();

	*seg1 = q1;
	*seg2 = q2;
	*seg3 = q3;

	_sync();

	q2 = *seg2;

	_sync(); _sync(); _sync(); _sync(); _sync(); _sync(); _sync(); _sync();

	_dcbi(seg3);

	if (*seg2 != q2)
	{
		PrintStr("error failed!\n");
	}
	else	PrintStr("test passed\n");
	while (1);
}
#endif
