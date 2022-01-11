/*  @(#) utils.h 96/09/23 1.43 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include "predefines.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef __3DO_DEBUGGER__
#ifndef _MW
	#include <fcntl.h>
#endif /* _MW */
#include <time.h>
#ifndef LOAD_FOR_MAC        
#include <unistd.h>         
#include <sys/stat.h>   
#include <sys/time.h>       
#elif !defined(__CD_EMULATION__)                   
#include "gluelib.h"  /* get missing unix functions */
#endif /* LOAD_FOR_MAC */
#else
#include "allocator.h"
#endif /* __3DO_DEBUGGER__ */

#include "loaderty.h"
#include "debug.h"
#ifdef macintosh
#include <Memory.h>
#endif

int simplestrcmp(const char* s, const char* t);

#ifdef _SUN4
#define mystrcmp simplestrcmp
#else
#define mystrcmp strcmp
#endif

#define MAXVSTR 8
#define X_FIELD(buf,T,OFF)      (*((T*)&(buf[OFF])))
#define ALIGN(x,y) ((((uint32)x) + ((y)-1)) & ~((y)-1) )
//#define SWAP(x) (swap(x))
#define min(x,y) (x<y ? x : y)
#define max(x,y) (x>y ? x : y)
#define NEW_LINE '\n'
#if _MSDOS || defined(macintosh)
	#define BINARY_READ_MODE "rb"
#else
	#define BINARY_READ_MODE "r"
#endif
//This linker figures out the path separator from the 
//makefile mmh 09/06/96
#define MAC_SEP ':'
#define DOS_SEP '\\'
#define UNIX_SEP '/'


#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif
#define _STRINGZ(x) #x
#define STRINGZ(x) _STRINGZ(x)
 
//==========================================================================
// misc utilities
// efficient c->p & p->c string funcs
void cname(char* cstr,StringPtr pstr);
void pname(StringPtr pstr,char* cstr);
char* cnamestr(StringPtr pstr);
StringPtr pnamestr(char* cstr);
int getenvnum(char* s);

#ifdef __3DO_DEBUGGER__ 
#include "UProgressBar.h"
#else
class TProgressBar {
 public:
	TProgressBar(void);
	~TProgressBar(void);
	OSErr InitProgressDialog(ConstStr255Param title, ConstStr255Param statusString);
	void EndProgressDialog(void);
	void ChangeTitle(ConstStr255Param title);
	void ChangeStatus(ConstStr255Param statusString);
	Boolean UpdateProgress(uint32 complete, uint32 total);
	void ClearProgress(void);
	};
#endif
//====================================================================
// memory

class State;
class Heap;
class Heap {
	State* _state;
	static int _max_ptrs;
		struct _heap_ptr {
			void* _ptr;
			char* _file;
			uint32 _line;
			} *_ptrs;
		int _ptr_ind;
public:
	//news & deletes for specific heap allocation
	//(not implemented yet)
	void* heap_new(void* x, const char* file=0,uint32 line=0);
	void heap_delete(void* x, const char* file=0,uint32 line=0);
	Heap(State* state=0);
	//void set_state(State* state) { _state = state; }
	Boolean add_ptr(void* x, const char* file=0,uint32 line=0);
	Boolean rmv_ptr(void* x, const char* file=0,uint32 line=0);
	Boolean full();
	void heap_check();
	~Heap();
	};

extern Heap* global_heap;
extern Heap* global_tmp_heap;
extern Heap* global_objheap;
#if defined(macintosh) && defined(DEBUG)
	extern Heap* global_handles;
	extern Heap* global_ptrs;
#endif

//overrides
#ifdef DEBUG
	//override global new & delete
	//INLINE void* operator new(size_t s);
	INLINE void* operator new(size_t s,char*file,uint32 line);
	//INLINE void operator delete(void* x,size_t s);
	#if defined(macintosh) && defined(DEBUG)
		//can't override DisposePtr && DisposeHandle???
		INLINE Handle NewHandle_dbg(size_t s,char*f,uint32 l);
		INLINE void DisposeHandle_dbg(Handle x,char*f,uint32 l);
		INLINE Ptr NewPtr_dbg(size_t x,char*f,uint32 l);
		INLINE void DisposePtr_dbg(Ptr x,char*f,uint32 l);
		INLINE Ptr NewPtrClear_dbg(size_t x,char* f, int l);
	#endif
#endif

//====================================================================
// heap inlines

#ifndef __UTILS_INLINES_H__
#if !defined(USE_DUMP_FILE) || defined(__UTILS_CP__)
#define __UTILS_INLINES_H__
	#if defined(macintosh) && defined(DEBUG)
		//can't override DisposePtr && DisposeHandle???
		INLINE Handle NewHandle_dbg(size_t s,char* f,uint32 l) {
			Handle x = ::NewHandle(s);
			global_heap->add_ptr(x,f,l); 
			return x;
			};
		INLINE void DisposeHandle_dbg(Handle x,char* f,uint32 l) {
			global_heap->rmv_ptr(x,f,l); 
			::DisposeHandle(x);
			};
		INLINE Ptr NewPtr_dbg(size_t x, char* f,uint32 l) {
			Ptr p = NewPtr(x);
			global_ptrs->add_ptr((void*)p,f,l);
			return p;
			}
		INLINE void DisposePtr_dbg(Ptr x, char* f,uint32 l) {
			global_ptrs->rmv_ptr((void*)x,f,l);
			DisposePtr(x);
			}
		INLINE Ptr NewPtrClear_dbg(size_t x,char* f, int l)  {
			Ptr p = NewPtrClear(x);
			global_ptrs->heap_new((void*)p,f,l);
			return p;
			}
	#endif
//override global new & delete
#ifndef _MW_BUG_
INLINE void* operator new(size_t s) {
	DBG_(PICKY,("new(x%X)\n",s));
	#if defined(_MW_BUG_) && defined(macintosh)
		void* x = (void*) NewPtr(s);
	#else
		void* x = malloc(s);
	#endif
	#ifdef DEBUG
		global_heap->add_ptr(x); 
	#endif
	return x;
	};
#endif  /* !_MW_BUG_ */
INLINE void* operator new(size_t s, char* file, uint32 line) {
	DBG_(PICKY,("new(x%X,%s,%d)\n",s,file,line));
	#if defined(_MW_BUG_) && defined(macintosh)
		void* x = (void*) NewPtr(s);
	#else
		void* x = malloc(s);
	#endif
	#ifdef DEBUG
		global_heap->add_ptr(x,file,line); 
	#endif
	return x;
	};

#ifndef _MW_BUG_
INLINE void operator delete(void* x,size_t s) {
	DBG_(PICKY,("delete(x%X,x%X)\n",x,s));
	#ifdef DEBUG
		global_heap->rmv_ptr(x); 
	#endif
	#ifdef _MW_BUG_
		DisposePtr((Ptr)x);
	#else
		free(x);
	#endif
	};
#endif /* !_MW_BUG_ */
	

#endif /* !defined(USE_DUMP_FILE) || defined(__UTILS_CP__) */
#endif /* __UTILS_INLINES_H__ */

//====================================================================
//defines for heap

#ifdef DEBUG
	#define HEAP_NEW(heap,x) (heap)->heap_new((void*)(new x),__FILE__,__LINE__)
	#define HEAP_DELETE(heap,x) { (heap)->heap_delete((void*)(x),__FILE__,__LINE__); delete x; x=0; }
	#define HEAP_DELETE_ARRAY(heap,x) { (heap)->heap_delete((void*)(x),__FILE__,__LINE__); delete [] x; x=0; }
	#define HEAP_CHECK(heap) (heap)->heap_check();
	//#define NEW(x) new(__FILE__,__LINE__) x
	//#define NEW new(__FILE__,__LINE__)
	#define NEW(x) HEAP_NEW(global_objheap,x)	//MW's new is broken :-(
	#define DELETE(x) HEAP_DELETE(global_objheap,x)
	#define DELETE_ARRAY(x) HEAP_DELETE_ARRAY(global_objheap,x)
	#define MALLOC(x) (global_heap)->heap_new((void*)malloc(x),__FILE__,__LINE__)
	#define FREE(x) { (global_heap)->heap_delete((void*)(x),__FILE__,__LINE__); free(x); }
#else
	#define HEAP_NEW(heap,x) (heap)->heap_new((void*)(new x))
	#define HEAP_DELETE(heap,x) { (heap)->heap_delete((void*)(x)); delete x; }
	#define HEAP_DELETE_ARRAY(heap,x) { (heap)->heap_delete((void*)(x)); delete [] x; }
	#define HEAP_CHECK(heap)
	#define NEW(x) new x
	#define DELETE(x) delete x
	#define DELETE_ARRAY(x) delete [] x
#ifdef __3DO_DEBUGGER__
	#define MALLOC(x) AllocateBlock(x)
	#define FREE(x) FreeBlock(x)
#else
	#define MALLOC(x) malloc(x)
	#define FREE(x) free(x)
#endif
#endif

#ifdef macintosh
//override
	#define HEAP_NEW_HANDLE(heap,x) (heap)->heap_new((void*)(NewHandle(x)),__FILE__,__LINE__)
	#define HEAP_DISPOSE_HANDLE(heap,x) { (heap)->heap_delete((void*)(x),__FILE__,__LINE__); DisposeHandle(x); }
	#define HEAP_NEW_PTR(heap,x) (heap)->heap_new((void*)(NewPtr(x)),__FILE__,__LINE__)
	#define HEAP_NEW_PTR_CLEAR(heap,x) (heap)->heap_new((void*)(NewPtrClear(x)),__FILE__,__LINE__)
	#define HEAP_DISPOSE_PTR(heap,x) { (heap)->heap_delete((void*)(x),__FILE__,__LINE__); DisposePtr(x); }
	#if 0	//defined(DEBUG) // debugger has strangeness with Ptrs & Resources 
		#define NewPtrClear(x)  NewPtrClear_dbg(x,__FILE__,__LINE__)
		#define NewPtr(x)  NewPtr_dbg(x,__FILE__,__LINE__)
		//#define NEW_HANDLE(x) NewHandle(x,__FILE__,__LINE__)
		//#define DISPOSE_HANDLE(x) DisposeHandle_(x,__FILE__,__LINE__)
		//#define NEW_PTR(x) NewPtr(x,__FILE__,__LINE__)
		//#define DISPOSE_PTR(x) DisposePtr_(x,__FILE__,__LINE__)
		#define NEW_HANDLE(x) (Handle) HEAP_NEW_HANDLE(global_handles,x)
		#define DISPOSE_HANDLE(x) HEAP_DISPOSE_HANDLE(global_handles,x)
		#define NEW_PTR(x) (Ptr) HEAP_NEW_PTR(global_ptrs,x)
		#define NEW_PTR_CLEAR(x) (Ptr) HEAP_NEW_PTR_CLEAR(global_ptrs,x)
		#define DISPOSE_PTR(x) HEAP_DISPOSE_PTR(global_ptrs,x)
	#else
		#define NEW_HANDLE(x) NewHandle(x)
		#define DISPOSE_HANDLE(x) DisposeHandle(x)
		#define NEW_PTR(x) NewPtr(x)
		#define DISPOSE_PTR(x) DisposePtr(x)
	#endif
#endif
#ifdef macintosh
	#define CHECK_HEAP() { HEAP_CHECK(global_heap); HEAP_CHECK(global_objheap); HEAP_CHECK(global_handles); HEAP_CHECK(global_ptrs); }
#else
	#define CHECK_HEAP() { HEAP_CHECK(global_heap); HEAP_CHECK(global_objheap); }
#endif


//==========================================================================
// Endian - class for determining host endianness and whether to swap bytes
#define BIG_ENDIAN false
#define LTL_ENDIAN true

//note: endianness assumes binary is big endian by default -
//if both host and binary are little_endian, shouldn't swap
//call set_endianness to set binary endianness
class Endian {
    Boolean _host_is_little_endian;
    Boolean _host_swap;
    union { char c; short i; } _candi;
public:
    Endian(Boolean target_is_little_endian=BIG_ENDIAN) {
        _candi.i = 0; _candi.c = 1;
        _host_is_little_endian = (Boolean) (_candi.i == 1);
        _host_swap = (Boolean) (_host_is_little_endian != target_is_little_endian);
        }
    Boolean swap_needed() { return _host_swap; }
    uint16 swap(uint16 l);
    int16 swap(int16 l) { return (int16) swap((uint16)l); }
    uint32 swap(uint32 l);
    int32 swap(int32 l) { return (int32) swap((uint32)l); }
    void set_endianness(Boolean target_is_little_endian) {
        _host_swap = (Boolean) (_host_is_little_endian != target_is_little_endian);
        }
    //swap in place
	void swapit(uint16 &a) { a = swap(a); }
	void swapit(int16 &a) { a = swap(a); }
	void swapit(int32 &a) { a = swap(a); }
	void swapit(uint32 &a) { a = swap(a); }
	//swap if needed
#ifdef macintosh
	// We know that swapping is not required on Macintosh, so this
	// is an optimization.
	uint32 swapfix(uint32 x) { return x;}
	uint16 swapfix(uint16 x) { return x;}
	int32 swapfix(int32 x) { return x;}
	int16 swapfix(int16 x) { return x;}
#else
	uint32 swapfix(uint32 x) { return (swap_needed() ? swap(x) : x);}
	uint16 swapfix(uint16 x) { return (swap_needed() ? swap(x) : x);}
	int32 swapfix(int32 x) { return (swap_needed() ? swap(x) : x);}
	int16 swapfix(int16 x) { return (swap_needed() ? swap(x) : x);}
#endif
    };
    
//==========================================================================
//defines for class Strstuffs
class Strstuffs {
public:
    static char* dup_str(const char *s);
	static char* strbuf();
    static char* vfmt_str(const char *fmt, va_list ap);
    static char* fmt_str(const char *fmt, ...);
    static char* str(const char *pstr);
    static char* str(int i, const char *pstr);
    };

//==========================================================================
//defines for class BufEaters
//		advances buf, modifies byte_size, and returns requested type
class BufEaters {
	unsigned char* _buf; 
	unsigned char* _start; 
	long _buf_size; 
	Endian* _endian;
public:
	BufEaters(Endian* e,uint32 b_size,unsigned char* b) {
		_start = b;
		_endian = e;
		_buf = b; 
		_buf_size=b_size;
		}
	unsigned char* ptr() { return _buf; }
	unsigned char* buf() {  return _start; }
	unsigned char eat_bite();
	uint16 eat_uint16();
	uint32 eat_uint32();
	uint16 eat_val(uint16) { return eat_uint16(); }
	uint32 eat_val(uint32) { return eat_uint32(); }
	int16 eat_val(int16) { return (int16)eat_uint16(); }
	int32 eat_val(int32) { return (int32)eat_uint32(); }
	//uint16 operator uint16() { return eat_uint16(); }
	//uint32 operator uint32() { return eat_uint32(); }
	char* eat_str();
	unsigned char* eat_nbites(uint32 n);
	uint32 bites_left() const { return _buf_size; }
	uint32 pick(uint32 x,uint32 off);
	uint16 pick(uint16 x,uint32 off);
	int32 pick(int32 x,uint32 off) { return (int32)pick((uint32)x,off); }
	int16 pick(int16 x,uint32 off) { return (int16)pick((uint16)x,off); }
	unsigned char pick(char x,uint32 off);
	//and, in case we ate something we weren't supposed to...
	void puke(char*b);
	void puke(unsigned char*b,uint32 n);
	void puke(uint32 v);
	void puke(uint16 v);
	};

//other buf eaters for when we don't feel like creating a new class...
#define GET_UINT16(v,buf,i) { 	\
	memcpy(&v,buf+i,2); v=swap((uint16)v); i+=2; 	\
	}
#define GET_UINT32(v,buf,i) { 	\
	memcpy(&v,buf+i,4); v=swap(v); i+=4; 	\
	}

//==========================================================================
//simple list implementation
class LList {
	uint32* _newobj;
	int _maxnum;
	int _num;
public:
LList(int num) {
	_newobj = new uint32[num];
	_maxnum = num;
	_num = 0;
	memset(_newobj,0,num*sizeof(uint32));
	}
~LList() {
	delete _newobj;
	}
void add(uint32 objind) {
	_newobj[_num]=objind;	//add this offset to list
	_num++;
	}
Boolean find(uint32 off) {
	for (int j=0; j<_num; j++) {
		if (_newobj[j]==off)
			return true;
		}
	return false;
	}
Boolean full() {
	if (_num>=_maxnum)
		return true;
	return false;
	}
int num() {
	return _num;
	}
	};
//==========================================================================
//simple stack implementation
#define CHUNK_SIZE  32  //number of elements in a chunk (must be power of 2)
#define MAX_NCHUNKS 20  //max number of chunks
#define MAX_STACK_SIZE CHUNK_SIZE*MAX_NCHUNKS   //max number of elems
#define CHUNK_SELECTOR 5    //2**CHUNK_SELECTOR=CHUNK_SIZE
#define ELEM_MASK 31    //CHUNK_SIZE-1

#ifdef __3DO_DEBUGGER__
//	dkk - Implementation of stack for 3DODebug. Doesn't have to be different, however I
//	only tested changes in the debugger. Biggest difference is that debugger version allocates
//	all the space it will need in the constructor, rather than allocating and freeing small
//	chunks as the stack grows and shrinks. If the memory allocation fails in the debugger, the
//	growzone proc will catch it.
class Stack { 
#ifdef macintosh		
//MrCpp has bug with inheritting protected members from protected base classes
public:
#else
protected:
#endif
	struct _elem { 
		uint32 i1; 
		uint32 i2; 
		};
	_elem* fStackBuffer;
	int _top;	//top index = one past number of elements in stack
	Boolean full() { return (Boolean) (_top >= MAX_STACK_SIZE); }
public:
	//SOMEDAY: generalize this
	Stack(void);
	~Stack();
	uint32 num() { return _top; }
	Boolean empty();
	void top(uint32& o,uint32& v);
	void top(uint32& o) { uint32 v; top(o,v); }
	void push(uint32 o,uint32 v=0);
	void push(uint32 o,void* v) { push(o,(uint32)v); }
	void pop(uint32& o,uint32& v);
	void pop(uint32& o) { uint32 v; pop(o,v); }
	void pop(uint32& o,void*& v) { uint32 _v=(uint32)v; pop(o,_v); v=(void*)_v; }
	};
#else
class Stack { 
#ifdef macintosh		
//MrCpp has bug with inheritting protected members from protected base classes
public:
#else
protected:
#endif
	struct _elem { 
		uint32 i1; 
		uint32 i2; 
		};
	Heap* _heap;
	int _nchunks;
	_elem* _schunk[MAX_NCHUNKS];
	int _top;	//top index = one past number of elements in stack
	int _n;	//maximum number of allocated slots in stack
	Boolean new_chunk();
	Boolean delete_chunk();
	_elem* top_elem() { 
		return &_schunk[(_top-1)>>CHUNK_SELECTOR][(_top-1)&ELEM_MASK];
		};
	Boolean full() { return (Boolean) (_top>=_n); }
public:
	//SOMEDAY: generalize this
	Stack(Heap* heap=global_heap);
	~Stack();
	uint32 num() { return _top; }
	Boolean empty();
	void top(uint32& o,uint32& v);
	void top(uint32& o) { uint32 v; top(o,v); }
	void push(uint32 o,uint32 v=0);
	void push(uint32 o,void* v) { push(o,(uint32)v); }
	void pop(uint32& o,uint32& v);
	void pop(uint32& o) { uint32 v; pop(o,v); }
	void pop(uint32& o,void*& v) { uint32 _v=(uint32)v; pop(o,_v); v=(void*)_v; }
	};
#endif

class Queue {
	struct _elem { 
		uint32 _i1; 
		uint32 _i2; 
	    _elem *_next;
		};
    struct _chunk {
	    _elem _elem_pool[CHUNK_SIZE];
	    _chunk *_next;
		};
	_chunk *_chunk_head;
	_elem *_free_pool;
	_elem *_queue_tail;
	uint32 _num;
	_elem *new_elem(uint32 o, uint32 v);
	void free_elem(_elem *e);
public:
	Queue();
	~Queue();
	uint32 num() { return _num; }
	Boolean empty() { return (Boolean)(_num == 0); }
	void bot(uint32& o,uint32& v);
	void bot(uint32& o) { uint32 v; bot(o,v); }
	void add(uint32 o,uint32 v=0);
	void add(uint32 o,void* v) { add(o,(uint32)v); }
	void rmv(uint32& o,uint32& v);
	void rmv(uint32& o) { uint32 v; rmv(o,v); }
	void rmv(uint32& o,void*& v) { uint32 _v=(uint32)v; rmv(o,_v); v=(void*)_v; }
	};

		
//==========================================================================
//simple Hash class for strings
class Heap;
//typedef int HashOff;	//optional hash offset for hashing function
enum HashOff {
	hoff_global=0
	};	//optional hash offset for hashing function

class Hash {
	struct _hent {	//hash entry
		_hent* _chain;	//chain for this entry
		const char* _name;	
		void* _ptr;
		_hent(_hent*& chain, const char* name, void* ptr);
		};
	Heap* _heap;
	int _hsize;
	_hent** _htable;	//table of ptrs to _hent
	int _hash_ind;
	_hent* _hash_ptr;
	void delete_chain(_hent* h);
public:
	Hash(int size, Heap* heap=global_heap);
	~Hash();
	//n below is for optional add to hash function
	//to force different hash for same name (as long as n<_hsize)
	uint32 compute_hash(const char* name, HashOff n = hoff_global);
	void add_hash(const char* name,void* ptr,HashOff n=hoff_global);
	void* set_hash(const char* name,void* ptr,HashOff n=hoff_global);
	void* get_hash(const char* name,HashOff n=hoff_global);
	void* first_hash(); //itterate thru hashed items
	void* next_hash();
	void rmv_hash();
	};

//====================================================================
// state
#define MAX_FNAME 30
class State {
	int    _state;
	uint32 _line;
	char  _file[MAX_FNAME+1];
public:
	State() {
		_state = 0;
		_line = 0;
		*_file = 0;
		}
	void set_state(int state, const char* file=0, uint32 line=0);
	int state(char*& fname, uint32& line);
	int state() { return _state; }
	};

#ifndef __3DO_DEBUGGER__
//====================================================================
// misc i/o funcs

FILE* open_(const char* fname="stdout");
FILE* openb_(const char* fname);
void close_(FILE*& fp);
Boolean exists(const char* fname);
const char* rmv_path(const char* fname);
void expand_at_files(int *argc, char ***argv);
#include <signal.h>
#ifdef macintosh
extern "C" {
	typedef void (*XcptHandler)(int);
	}
#else
	typedef void (*XcptHandler)(int sig, int code, struct sigcontext *scp, char* addr);
#endif
void register_xcpt_handler(XcptHandler);

//====================================================================
// misc conversion funcs

uint32 ch2val(const char* cval);	//cnv from char input to number
uint32 str2time(char *ts);
char* time2str(time_t secs);
void busy_cursor();

#ifdef macintosh
//====================================================================
// spin cursor funcs for Mac to support command period
// (Thanks to John McMullen for these :-)
#include <events.h>

Boolean KeyBit(KeyMap keyMap, char keyCode);
Boolean UserAbort();
class BusyCursor {
public:
	BusyCursor();
	void Spin();
	};

#endif /* macintosh */
#endif /* __3DO_DEBUGGER__ */

//Path separator guessing class mmh
class PathString {
	char separator;
	const char *pathname;
public:
	PathString();
	PathString(const char* p) {pathname = p;}
	void test ();
	char get_separator() {return separator;}
};
#endif /* __UTILS_H__ */

