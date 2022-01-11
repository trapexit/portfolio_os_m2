		.include	"structmacros.i"
		.include	"PPCMacroequ.i"
		.include	"conditionmacros.i"
		.include	"mercury.i" 

		.include	"M_Sort.i"

		define		lasttimeFLAG	,	31

		.global M_anchorpod
		.global M_userpodptrs
	
/*
 *	PodPipeEntry
 *	Entry point of the pod-based pipeline from C.  Note that since the count
 *	of pod structures is provided, the user list does not need to be
 *	zero-terminated.
 *
 *	INPUT:
 *		GPR3:count of elements in linked list (minimum 1, maximum 2048)
 *		GPR4:pointer to first pod in linked list
 *		GPR5:pointer to a pointer into which a pointer the last pod is placed
 *
 *	OUTPUT:	none
 *
 */			
	DECFN	M_Sort

/*	Save off all the registers that the C code uses.
 *	User input in totalcount and puserobjects remains untouched (GPR 3,4) */
		mflr		0
		bl		M_StartAsmCode

/*	SORT THE OBJECTS */
/*	This is a mergesort (of sorts), to guarantee O(n log n)
 *	performance.  The last pass is special, as it sets
 *	flags to cue the object setup code as to what needs to
 *	be done.
 */

		lea		puserpodptrs,M_userpodptrs

/*	We sort the pods in chunks of up to 128, the most that will fit in the
 *	4K byte data cache.  As we sort them, we create a list of pointers
 *	to the first element in each chunk.
 */

/*	As a first step, determine "chunkcount", the number of these chunks */
		li		chunkbits,0
		neg		mptemp0,totalcount
.Lsetchunkbitsloop:		
		addi		chunkbits,chunkbits,1
		sraw		mptemp1,mptemp0,chunkbits
		cmpi		0,mptemp1,-128
		blt-		.Lsetchunkbitsloop		
		li		chunkcount,1
		slw		chunkcount,chunkcount,chunkbits

/*	Now, a double loop.  The outer loop is over the chunks, the inner
 *	loop is performed (log size of chunk) times
 */
		crclr		lasttimeFLAG

		li		chunkloop,0
		lea		poldchunklastpod,M_anchorpod
		stw		puserobjects,Pod.pnext_off(poldchunklastpod)

.Louterlooptop:		
/*	calculate size of the chunk (which is totalcount/chunkcount, maybe +1)
 *	((totalcount*(chunkloop+1))>>chunkbits)-((totalcount*chunkloop)>>chunkbits)
 */
		mullw		mptemp0,totalcount,chunkloop
		add		mptemp1,mptemp0,totalcount
		sraw		mptemp0,mptemp0,chunkbits
		sraw		mptemp1,mptemp1,chunkbits
		sub		chunksize,mptemp1,mptemp0
		slwi		mptemp0,chunkloop,2
		stwx		poldchunklastpod,mptemp0,puserpodptrs

		li		groupsize,1					/* sort sublists of 1 at first */
.Linnerlooptop:			
		mr.		remcount,chunksize
		mr		pprevpod,poldchunklastpod
		lwz		ptr1,Pod.pnext_off(poldchunklastpod)

		bnel		MergesortPass	/* chunksize 0 not called, 1 returns properly */
	
		add		groupsize,groupsize,groupsize
		cmp		0,chunksize,groupsize
		bgt		.Linnerlooptop			

		mr		poldchunklastpod,pprevpod

 		addi		chunkloop,chunkloop,1
	 	cmp		0,chunkloop,chunkcount
	 	bne+		.Louterlooptop		
 
/*	Final sorts, also a double loop.  Each of the chunks is merged with
 *	its neighbors in the first pass, then pairs of chunks are merged, etc.
 */
		li		remcount,0				/* so remcount < groupcount */
		li		chunkstep,1
.Louterlooptop2:			
		add		mptemp0,chunkstep,chunkstep
		cmp		0,mptemp0,chunkcount
		li		chunkloop,0	
/*	--		*/
		crmove		lasttimeFLAG,cr0eq

.Linnerlooptop2:		
/*	calculate start address and size of the two groups of chunks */
		add		mptemp1,chunkloop,chunkstep

		slwi		pprevpod,chunkloop,2
		slwi		ptr1,mptemp1,2

		lwzx		pprevpod,pprevpod,puserpodptrs
		lwzx		ptr1,ptr1,puserpodptrs

		lwz		ptr0,Pod.pnext_off(pprevpod)
		lwz		ptr1,Pod.pnext_off(ptr1)

/*	mptemp0 = (totalcount * chunkloop) >> chunkbits
 *	mptemp1 = (totalcount * (chunkloop+chunkstep)) >> chunkbits
 *	count1 = (totalcount * (chunkloop+ 2*chunkstep)) >> chunkbits
 *		
 *	count0 = mptemp1 - mptemp0;
 *	count1 = count1 - mptemp1;
 */
		mullw		mptemp0,totalcount,chunkloop
		add		count1,mptemp1,chunkstep
		mullw		mptemp1,mptemp1,totalcount
		sraw		mptemp0,mptemp0,chunkbits
		mullw		count1,count1,totalcount
		sraw		mptemp1,mptemp1,chunkbits
		sraw		count1,count1,chunkbits
		sub		count0,mptemp1,mptemp0
		sub		count1,count1,mptemp1

		bl		MergesortPassEntry2

/*	store the new pprevpod to puserpodptrs	*/
		add		mptemp1,chunkloop,chunkstep
		add		mptemp1,mptemp1,chunkstep
		slwi		mptemp1,mptemp1,2
		stwx		pprevpod,mptemp1,puserpodptrs

		add		chunkloop,chunkloop,chunkstep
		add		chunkloop,chunkloop,chunkstep
		cmp		0,chunkloop,chunkcount
		bne+		.Linnerlooptop2		

		add		chunkstep,chunkstep,chunkstep
		cmp		0,chunkstep,chunkcount
		bne+		.Louterlooptop2			

/*	Make the final list zero-terminated */
		li		mptemp0,0
		stw		mptemp0,Pod.pnext_off(pprevpod)
		cmpi		0,5,0

/*	Load the first pod and jump to the loop over pods */
		lwz		3,0(puserpodptrs)
		lwz		3,Pod.pnext_off(3)
		beq		skipstore
		stw		pprevpod,0(5)
skipstore:	
		b		M_EndAsmCode

/*
 *	MergesortPass
 *	Perform single pass of a mergesort on "remcount" elements of the
 *	pod linked list.  During this pass, a succession of pairs of sublists 
 *  is merged, where each sublist in the pair is groupsize long (except
 *	for the last sublist, which may be shorter).  The final result is
 *	a new linked list, that starts with the pod at "prevptr" and ends
 *	with the former end structure, be it defined or undefined [there is
 *	no requirement that the user linked list be terminated].
 *
 *	The pods are sorted lowest first -- meaning that the nextptr elements
 *	are rearranged so that they are in the proper order.  "Lowest" in this
 *	context means check pcase, ptexture, pgeometry, pmatrix and plights
 *	and select the lowest on the basis of the first to differ.  This causes
 *	all pods with the same pcase to be sorted together; within the same
 *	pcase, all pods with the same ptexture are sorted together, and so on.
 *
 *	On the final pass, there is some special processing to set flags bits
 *	based on the differences between one pod and the next in the final sorted
 *  list.  Also, count0, count1 and ptr0 are supplied at the call, and a 
 *	zero remcount is input.  "groupsize" is not used.  The effect of this
 *	all is two sort the specified two sublists, and not to continue
 *	with any more sublists.
 *	
 *	INPUT:	pprevpod -- place the first element of the sorted list here
 *			ptr1 -- pointer to the head of the pod list
 *			remcount -- number of elements to sort
 *			groupsize -- the number of elements in each of the
 *				two sublists that will be merged.
 *			lasttimeFLAG -- set if this is the final call
 *	OUTPUT:	pprevpod -- last element of sorted list
 *			ptr1 -- first object after sort
 */		
MergesortPass:	

/*	This store is only meaningful at the start of subsequent pairs of lists,
 *	but causes no harm the first time around.
 */
		stw		ptr1,Pod.pnext_off(pprevpod)

		cmp		0,remcount,groupsize
		bgt+		.Ldontexit

/*	If the amount of structures is insufficient to allow the creation of
 *	two lists, just tie in pprevpod to the structures, and return with
 *	ptr1 containing the first value after the sort.
 */
		cmpi		0,remcount,0
		mtctr		remcount
		beqlr-

.Ltooshortinput:			
		mr		pprevpod,ptr1
		lwz		ptr1,Pod.pnext_off(pprevpod)
		bdnz+		.Ltooshortinput			
		blr
.Ldontexit:			
/*	New first sublist starts at entry after old second sublist */
		mr		ptr0,ptr1

/*	Follow the pointer links forward groupsize times from ptr0 to find ptr1 */
		mtctr		groupsize
.Lfirstlooptop:					
		lwz		ptr1,Pod.pnext_off(ptr1)
		bdnz+		.Lfirstlooptop					

/*	"remcount" contains the number of remaining pods in the area we are
 *	sorting.  Subtract out the pods we passed above.
 */
		sub		remcount,remcount,groupsize

/*	Set count1 to the smaller of groupsize and remcount */
		cmp		0,remcount,groupsize
		mr		count0,groupsize
		mr		count1,groupsize
		bge		.Lcont		
		mr		count1,remcount
.Lcont:		
		sub		remcount,remcount,count1

MergesortPassEntry2:	
/*	Prepare for the merge sort of the two sublists */
		cmpi		0,count0,0
		cmpi		1,count1,0

		lwz		case0,Pod.pcase_off(ptr0)
		lwz		texture0,Pod.ptexture_off(ptr0)
		lwz		geometry0,Pod.pgeometry_off(ptr0)
		lwz		matrix0,Pod.pmatrix_off(ptr0)
		lwz		lights0,Pod.plights_off(ptr0)

		lwz		case1,Pod.pcase_off(ptr1)
		lwz		texture1,Pod.ptexture_off(ptr1)
		lwz		geometry1,Pod.pgeometry_off(ptr1)
		lwz		matrix1,Pod.pmatrix_off(ptr1)
		lwz		lights1,Pod.plights_off(ptr1)

/*	Sort the two sublists.  After each compare, the lower entry (based
 *	on five in-order comparisons) is added to the list -- i.e., the
 *	pprevpod's nextptr is set to point to the lowest element, and the
 *	lowest element becomes the new pprevpod.
 */
.Lsorttwosublists:			
		cmpl		2,case0,case1
		bt		cr0eq,.Lstore1extracheck				/* is case1 == 0 as well? */
		bt		cr1eq,.Lstore0		
		cmpl		3,texture0,texture1
		bt		cr2lt,.Lstore0		
		bt		cr2gt,.Lstore1	
		cmpl		2,geometry0,geometry1
		bt		cr3lt,.Lstore0		
		bt		cr3gt,.Lstore1	
		cmpl		3,matrix0,matrix1
		bt		cr2lt,.Lstore0		
		bt		cr2gt,.Lstore1	
		cmpl		2,lights0,lights1
		bt		cr3lt,.Lstore0		
		bt		cr3gt,.Lstore1	
		bt		cr2gt,.Lstore1	
.Lstore0:		
		stw		ptr0,Pod.pnext_off(pprevpod)
		bt		lasttimeFLAG,.Llasttimesub0		
.Llasttimesub0ret:		
		mr		pprevpod,ptr0
		lwz		ptr0,Pod.pnext_off(ptr0)
		addic.		count0,count0,-1			/* set CR0 */
		beq-		.Lsorttwosublists			
		lwz		case0,Pod.pcase_off(ptr0)
		lwz		texture0,Pod.ptexture_off(ptr0)
		lwz		geometry0,Pod.pgeometry_off(ptr0)
		lwz		matrix0,Pod.pmatrix_off(ptr0)
		lwz		lights0,Pod.plights_off(ptr0)
		b		.Lsorttwosublists			
.Llasttimesub0:		
		lwz		mptemp0,Pod.pcase_off(pprevpod)
		lwz		mptemp1,Pod.ptexture_off(pprevpod)
		cmpl		2,mptemp0,case0
		cmpl		3,mptemp1,texture0
		crmove		samecaseFLAG,cr2eq
		crmove		sametextureFLAG,cr3eq
		lwz		mptemp0,Pod.flags_off(ptr0)
		mfcr		mptemp1
		rlwimi		mptemp0,mptemp1,0,samecaseFLAG,sametextureFLAG
		stw		mptemp0,Pod.flags_off(ptr0)
		b		.Llasttimesub0ret		
.Lstore1extracheck:		
		bt		cr1eq,MergesortPass			/* done with this pair of lists */
.Lstore1:	
		stw		ptr1,Pod.pnext_off(pprevpod)
		bt		lasttimeFLAG,.Llasttimesub1		
.Llasttimesub1ret:		
		mr		pprevpod,ptr1
		lwz		ptr1,Pod.pnext_off(ptr1)
		addi		count1,count1,-1
		cmpi		1,count1,0

/*	Since the list isn't terminated, don't load from ptr1 unless it's valid */
		bt-		cr1eq,.Lsorttwosublists			
		lwz		case1,Pod.pcase_off(ptr1)
		lwz		texture1,Pod.ptexture_off(ptr1)
		lwz		geometry1,Pod.pgeometry_off(ptr1)
		lwz		matrix1,Pod.pmatrix_off(ptr1)
		lwz		lights1,Pod.plights_off(ptr1)
		b		.Lsorttwosublists			
.Llasttimesub1:		
		lwz		mptemp0,Pod.pcase_off(pprevpod)
		lwz		mptemp1,Pod.ptexture_off(pprevpod)
		cmpl		2,mptemp0,case1
		cmpl		3,mptemp1,texture1
		crmove		samecaseFLAG,cr2eq
		crmove		sametextureFLAG,cr3eq
		lwz		mptemp0,Pod.flags_off(ptr1)
		mfcr		mptemp1
		rlwimi		mptemp0,mptemp1,0,samecaseFLAG,sametextureFLAG
		stw		mptemp0,Pod.flags_off(ptr1)
		b		.Llasttimesub1ret		

/*	END */
	
