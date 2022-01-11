/* @(#) watchdie.c 95/08/29 1.5 */

/*
   Watchdie.c

   By: Kevin Hester

   This little hack just sets the TCR on a 602 to enable the watchdog
   interrupt.  This should currently trigger death a short time later!
*/

#include <kernel/types.h>
#include <hardware/PPCasm.h>
#include <stdio.h>

extern void _esa(void), _dsa(void);
void _mttcr(uint32 n);
uint32 _mftcr(void);


void	main(void)
{
    uint32 oldmode;

    printf("Welcome to watchdie, get ready for death!\n");
    printf("(Huh, Huh, Death is Cool!)\n");

    _esa();
    oldmode = _mftcr();
    _dsa();

    printf("Old TCR %x\n", oldmode);

    _esa();
    _mttcr(0xfc000000);
    _dsa();

    printf("Leaving watchdie\n");
}

