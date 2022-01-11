\ @(#) approx.fth 96/03/07 1.2
\ test Taylor expansions for accuracy

anew task-approx.fth

2.0 FLN fconstant FLN2.0
fvariable F-XL2
fvariable F-SUM

: FACTORIAL  ( n -- n! )
	dup 1 >
	IF
		dup 1- recurse *
	ELSE
		drop 1
	THEN
;

: 2**X ( fx n -- )
\	1.0 f- 1.0014 f* 1.0 f+
	FLN2.0 f*  F-XL2 f!
	1.0 F-SUM f!
	0 ?DO
		F-XL2 f@
		i  0
		?DO
			F-XL2 f@ F*
		LOOP
		i 1+ factorial s>f
		f/
		f-sum f@
		f+
		f-sum f!
	LOOP
	F-SUM F@
;

30 constant NUM_VALS
9 constant MAX_TERMS
fvariable F-X
: table.app
	NUM_VALS 0
	DO
		6.0 NUM_VALS s>f f/
		i s>f f*
		-3.0 f+ f-x f!
		f-x f@ f.
		max_terms 4
		DO
			f-x f@  i dup .  2**x
			2.0 f-x f@ f** f/ f.
		LOOP
		cr
	LOOP
;

0 [if]
w = x*ln2

= 1 + w + w*w/2 + w*w*w/6 + w*w*w*w/24
= 1 + w*(1 + w/2 + w*w/6 + w*w*w/24)
= 1 + w*(1 + w(1/2 + w/6 + w*w/24))
= 1 + w*(1 + w*(1/2 + w*(1/6 + w/24)))
= 2.0(1/2 + w*(1/2 + w*(1/4 + w*(1/12 + w/48)))
[then]

: fast.2**x ( x -- 2**x )
	FLN2.0 f*  F-XL2 f!
F-XL2 f@ f. cr
	                    0.5 5 factorial s>f f/ ." coeff = " fdup f.
	F-XL2 f@ f*         0.5 4 factorial s>f f/ ." coeff = " fdup f. f+
fdup f. cr
	F-XL2 f@ f*         0.5 3 factorial s>f f/ ." coeff = " fdup f. f+
fdup f. cr
	F-XL2 f@ f*         0.5 2 factorial s>f f/ ." coeff = " fdup f. f+
fdup f. cr
	F-XL2 f@ f*         0.5 f+
fdup f. cr
	F-XL2 f@ f*         0.5 f+
fdup f. cr
	2.0 f*
fdup f. cr
;

: gen.dsp.coeffs
	10 1
	DO
		i .
		0.5 i factorial s>f f/
		$ 8000 s>f f* 0.5 f+ f>s
		.hex cr
	LOOP
;
