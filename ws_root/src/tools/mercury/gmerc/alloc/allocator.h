

#ifdef __cplusplus
extern "C" {
#endif

// Inside Mac says that C-style functions returning pointers will return them in
//  D0.  By default, Metrowerks returns pointers in A0.  Define our external functions
//  return pointers in D0 so that they will work for calls from the MPW runtime and
//  from routines that expect us to behave in the 'standard' way
#if __MWERKS__
#pragma pointers_in_D0
#endif

void	*AllocateBlock(size_t size);
void	*ReallocateBlock(void *ptr, size_t size);

#if __MWERKS__
#pragma pointers_in_A0		//	reset the standard Metrowerks parameter return
#endif

void	FreeBlock(void *pointer);
void	FreeAllBlocks(void);

#ifdef __cplusplus
}
#endif
