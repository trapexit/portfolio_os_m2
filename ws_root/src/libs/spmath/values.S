/* @(#) values.S 96/02/07 1.1  */

	.global	_huge_val_f
	.global	_NaN_f

	.text

_huge_val_f:	.long	0x7f800000
_NaN_f:		.long	0x7fC00000

