/* @(#) pfinnrfp.h 96/03/21 1.6 */
/***************************************************************
** Compile FP routines.
** This file is included from "pf_inner.c"
**
** These routines could be left out of an execute only version.
**
** Author: Darren Gibbs, Phil Burk
** Copyright 1994 3DO, Phil Burk, Larry Polansky, Devid Rosenboom
**
** The pForth software code is dedicated to the public domain,
** and any third party may reproduce, distribute and modify
** the pForth software code or any derivative works thereof
** without any compensation or license.  The pForth software
** code is provided on an "as is" basis without any warranty
** of any kind, including, without limitation, the implied
** warranties of merchantability and fitness for a particular
** purpose and their equivalents under the laws of any jurisdiction.
**
****************************************************************
**
***************************************************************/

#ifdef PF_SUPPORT_FP

	case ID_FP_D_TO_F: /* ( dlo dhi -- ) ( F: -- r ) */
		PUSH_FP_TOS;
		M_DROP;   /* Throw away high word. !!! */
		FP_TOS = ((PF_FLOAT) TOS);  /* Convert TOS and push on FP stack. */
		M_DROP;   /* drop dlo */
		break;

	case ID_FP_FSTORE: /* ( addr -- ) ( F: r -- ) */
		*((PF_FLOAT *) TOS) = FP_TOS;
		M_FP_DROP;		/* drop FP value */
		M_DROP;			/* drop addr */
		break; 

	case ID_FP_FTIMES:  /* ( F: r1 r2 -- r1*r2 ) */
		FP_TOS = M_FP_POP * FP_TOS;
		break;

	case ID_FP_FPLUS:  /* ( F: r1 r2 -- r1+r2 ) */
		FP_TOS = M_FP_POP + FP_TOS;
		break;
			
	case ID_FP_FMINUS:  /* ( F: r1 r2 -- r1-r2 ) */
		FP_TOS = M_FP_POP - FP_TOS;
		break;

	case ID_FP_FSLASH:  /* ( F: r1 r2 -- r1/r2 ) */
		FP_TOS = M_FP_POP / FP_TOS;
		break;

	case ID_FP_F_ZERO_LESS_THAN: /* ( -- flag )  ( F: r --  ) */
		PUSH_TOS;
		TOS = (FP_TOS < 0.0) ? FTRUE : FFALSE ;
		M_FP_DROP;
		break;

	case ID_FP_F_ZERO_EQUALS: /* ( -- flag )  ( F: r --  ) */
		PUSH_TOS;
		TOS = (FP_TOS == 0.0) ? FTRUE : FFALSE ;
		M_FP_DROP;
		break;

	case ID_FP_F_LESS_THAN: /* ( -- flag )  ( F: r1 r2 -- ) */
		PUSH_TOS;
		TOS = (M_FP_POP < FP_TOS) ? FTRUE : FFALSE ;
		M_FP_DROP;
		break;
		
	case ID_FP_F_TO_D: /* ( -- dlo dhi) ( F: r -- ) */
		PUSH_TOS;   /* Save old TOS */
		TOS = (int32) FP_TOS;
		M_FP_DROP;
		PUSH_TOS;
		/* Fake high word. !!! */
		TOS = (TOS < 0) ? -1 : 0;  /* push sign extension */
		break;

	case ID_FP_FFETCH:  /* ( addr -- ) ( F: -- r ) */
		PUSH_FP_TOS;
		FP_TOS = *((PF_FLOAT *) TOS);
		M_DROP;
		break;
		
	case ID_FP_FDEPTH: /* ( -- n ) ( F: -- ) */
		PUSH_TOS;
	/* Add 1 to account for FP_TOS in cached in register. */
		TOS = (( M_FP_SPZERO - FP_STKPTR) + 1);
		break;
		
	case ID_FP_FDROP: /* ( -- ) ( F: r -- ) */
		M_FP_DROP;
		break;
		
	case ID_FP_FDUP: /* ( -- ) ( F: r -- r r ) */
		PUSH_FP_TOS;
		break;
		
	case ID_FP_FLOAT_PLUS: /* ( addr1 -- addr2 ) ( F: -- ) */
		TOS = TOS + sizeof(PF_FLOAT);
		break;
		
	case ID_FP_FLOATS: /* ( n -- size ) ( F: -- ) */
		TOS = TOS * sizeof(PF_FLOAT);
		break;
		
	case ID_FP_FLOOR: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) floor( FP_TOS );
		break;
		
	case ID_FP_FMAX: /* ( -- ) ( F: r1 r2 -- r3 ) */
		fpScratch = M_FP_POP;
		FP_TOS = ( FP_TOS > fpScratch ) ? FP_TOS : fpScratch ;
		break;
		 
	case ID_FP_FMIN: /* ( -- ) ( F: r1 r2 -- r3 ) */
		fpScratch = M_FP_POP;
		FP_TOS = ( FP_TOS < fpScratch ) ? FP_TOS : fpScratch ;
		break;
		
	case ID_FP_FNEGATE:
		FP_TOS = -1.0 * FP_TOS;
		break;
		
	case ID_FP_FOVER: /* ( -- ) ( F: r1 r2 -- r1 r2 r1 ) */
		PUSH_FP_TOS;
		FP_TOS = M_FP_STACK(1);
		break;
		
	case ID_FP_FROT: /* ( -- ) ( F: r1 r2 r3 -- r2 r3 r1 ) */
		fpScratch = M_FP_POP;		/* r2 */
		fpTemp = M_FP_POP;			/* r1 */
		M_FP_PUSH( fpScratch );		/* r2 */
		PUSH_FP_TOS;				/* r3 */
		FP_TOS = fpTemp;			/* r1 */
		break;
		
	case ID_FP_FROUND:
		printf("\n Not Yet!! \n");
		break;
		
	case ID_FP_FSWAP: /* ( -- ) ( F: r1 r2 -- r2 r1 ) */
		fpScratch = FP_TOS;
		FP_TOS = *FP_STKPTR;
		*FP_STKPTR = fpScratch;
		break;
		
	case ID_FP_FSTAR_STAR: /* ( -- ) ( F: r1 r2 -- r1^r2 ) */
		fpScratch = M_FP_POP;
		FP_TOS = pow(fpScratch, FP_TOS);
		break;
		
	case ID_FP_FABS: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) fabs( FP_TOS );
		break;
		
	case ID_FP_FACOS: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) acos( FP_TOS );
		break;
		
	case ID_FP_FACOSH: /* ( -- ) ( F: r1 -- r2 ) */
		/* acosh(x) = log(y + sqrt(y^2 - 1) */
		FP_TOS = (PF_FLOAT) log(FP_TOS + (sqrt((FP_TOS * FP_TOS) - 1)));
		break;
		
	case ID_FP_FALOG: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) pow(10.0,FP_TOS);
		break;
		
	case ID_FP_FASIN: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) asin( FP_TOS );
		break;
		
	case ID_FP_FASINH: /* ( -- ) ( F: r1 -- r2 ) */
		/* asinh(x) = log(y + sqrt(y^2 + 1) */
		FP_TOS = (PF_FLOAT) log(FP_TOS + (sqrt((FP_TOS * FP_TOS) + 1)));
		break;
		
	case ID_FP_FATAN: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) atan( FP_TOS );
		break;
		
	case ID_FP_FATAN2: /* ( -- ) ( F: r1 r2 -- atan(r1/r2) ) */
		FP_TOS = (PF_FLOAT) atan2( FP_TOS, M_FP_POP );
		break;
		
	case ID_FP_FATANH: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) (0.5 * log((1 + FP_TOS) / (1 - FP_TOS)));
		break;
		
	case ID_FP_FCOS: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) cos( FP_TOS );
		break;
		
	case ID_FP_FCOSH: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) cosh( FP_TOS );
		break;
		
#ifndef PF_NO_SHELL
	case ID_FP_FLITERAL:
		ffFPLiteral( FP_TOS );
		M_FP_DROP;
		endcase;
#endif  /* !PF_NO_SHELL */

	case ID_FP_FLITERAL_P:
		PUSH_FP_TOS;
#if 0
/* Some wimpy compilers can't handle this! */
		FP_TOS = *(((PF_FLOAT *)InsPtr)++);
#else
		{
			PF_FLOAT *fptr;
			fptr = (PF_FLOAT *)InsPtr;
			FP_TOS = *fptr++;
			InsPtr = (cell *) fptr;
		}
#endif
		endcase;

	case ID_FP_FLN: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) log(FP_TOS);
		break;
		
	case ID_FP_FLNP1: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) (log(FP_TOS) + 1.0);
		break;
		
	case ID_FP_FLOG: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) log10( FP_TOS );
		break;
		
	case ID_FP_FSIN: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) sin( FP_TOS );
		break;
		
	case ID_FP_FSINCOS: /* ( -- ) ( F: r1 -- r2 r3 ) */
		M_FP_PUSH((PF_FLOAT) sin(FP_TOS));
		FP_TOS = (PF_FLOAT) cos(FP_TOS);
		break;
		
	case ID_FP_FSINH: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) sinh( FP_TOS );
		break;
		
	case ID_FP_FSQRT: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) sqrt( FP_TOS );
		break;
		
	case ID_FP_FTAN: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) tan( FP_TOS );
		break;
		
	case ID_FP_FTANH: /* ( -- ) ( F: r1 -- r2 ) */
		FP_TOS = (PF_FLOAT) tanh( FP_TOS );
		break;

#endif
