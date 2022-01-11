/*  @(#) utils.cpp 96/09/23 1.44 */

/*
	File:		utils.cpp

	Written by:	John R. McMullen

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by The 3DO Company.

	Change History (most recent first):

		<9+>	96/03/13	JRM		Fix xcpthandler stuff for Macintosh.
		 <7>	96/03/11	JRM		Merged Danny's and my changes.

	To Do:
*/

#ifndef USE_DUMP_FILE
#include "predefines.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#endif /* USE_DUMP_FILE */

#ifndef __3DO_DEBUGGER__
#include <time.h>
#ifndef LOAD_FOR_MAC
//#include <unistd.h> 
#include <sys/stat.h>
#ifdef _MSDOS
#include <sys/timeb.h>
#else
#include <sys/time.h> 
#endif
#else
#include "gluelib.h"  /* get missing unix functions */
#endif
#include "parse.h"
#endif /* __3DO_DEBUGGER__ */

#include "utils.h"
#define DEBUG_DEFS
#include "debug.h"

#ifdef macintosh
#include <Files.h>
#endif
#ifndef _MSDOS
#pragma segment utils
#endif
//==========================================================================
// misc utilities
// efficient c->p & p->c string funcs
void cname(char* cstr,StringPtr pstr) {
	DBG_ASSERT(cstr && pstr);
	cstr[0]=0;
	if (pstr && pstr[0]) {
		memcpy(cstr,pstr+1,(unsigned char)pstr[0]);
		cstr[(unsigned char)pstr[0]]=0;
		}
	}
void pname(StringPtr pstr,char* cstr) {
	DBG_ASSERT(cstr && pstr);
	pstr[0]=0;
	if (cstr) {
		pstr[0] = (char)strlen(cstr);
		memcpy(pstr+1,cstr,(unsigned char)pstr[0]);
		}
	}
char* cnamestr(StringPtr pstr) {
	DBG_ASSERT(pstr);
	char* cstr=0;
	if (pstr && pstr[0]) {
		cstr = Strstuffs::strbuf();
    	memcpy(cstr,(char*)pstr+1,(unsigned char)pstr[0]+1);
    	cstr[(unsigned char)pstr[0]]=0;
    	}
    return cstr;
    }
StringPtr pnamestr(char* cstr) {
	DBG_ASSERT(cstr);
	StringPtr pstr=0;
	if (cstr) {
		pstr = (StringPtr)Strstuffs::strbuf();
		pstr[0] = (char)strlen(cstr);
		memcpy(pstr+1,cstr,(unsigned char)pstr[0]);
		}
    return pstr;
	}

#ifdef _GCC	//gcc doesn't supply strtoul
uint32 strtoul(const char* s, char** e, int b) {
	uint32 l=0;
	DBG_ASSERT(e==0);	//I don't implement e
	for (const char* p=s; p && *p; p++) {
		if (*p>='0'&&*p<='9') 
			l = l*b + (*p-'0');
		else if (*p>='a'&&*p<='f') 
			l = l*b + (*p-'a'+10);
		else if (*p>='A'&&*p<='F') 
			l = l*b + (*p-'A'+10);
		else break;	//ran into some other token
		}
	return l;
	}
#endif
//get environment value
int getenvnum(char* envstring) {
    char *s = getenv(envstring);
	int s_int;
	if (s!=0) {
		s_int = atoi(s);
		return s_int;
		}
	return 0;
    }

//-------------------------------------------
int simplestrcmp(const char* s, const char* t)
// K&R (ANSI edition) p.106
//-------------------------------------------
{
	for (; *s == *t; s++, t++)
	{
		if (*s == '\0')
			return 0;
	}
	return *s - *t;
} // simplestrcmp


#ifndef __3DO_DEBUGGER__
//stubs for class TProgressBar - may want to add unix/dos equivalents ..??
enum {
	kTitleItemNumber = 1,
	kStatusItemNumber = 2,
	kSpinCursorRsrcID = 1003,
	kProgressDialog = 1005
	};
TProgressBar::TProgressBar() { }
TProgressBar::~TProgressBar() { }
OSErr TProgressBar::InitProgressDialog(ConstStr255Param /*title*/, ConstStr255Param /*statusString*/)
	{ return 0; }
void TProgressBar::EndProgressDialog() { }
void TProgressBar::ChangeTitle(ConstStr255Param /*title*/)
	{}
void TProgressBar::ChangeStatus(ConstStr255Param /*statusString*/) { }
Boolean TProgressBar::UpdateProgress(uint32 /*complete*/, uint32 /*total*/)
	{ return true; }
void TProgressBar::ClearProgress()
	{ }
#endif // __3DO_DEBUGGER__

//==========================================================================
// Endian - class for determining host endianness and whether to swap bytes
unsigned long Endian::swap(unsigned long l) {
    if (swap_needed()){
        union {
            unsigned char b4[4];
            unsigned long l;
            }u,v;
        u.l = l;
        v.b4[0] = u.b4[3];
        v.b4[1] = u.b4[2];
        v.b4[2] = u.b4[1];
        v.b4[3] = u.b4[0];
        l = v.l;
        }
    return l;
    }

unsigned short Endian::swap(unsigned short l) {
    if (swap_needed()) {
        union {
            unsigned char b2[2];
            unsigned short l;
            }u,v;
        u.l = l;
        v.b2[0] = u.b2[1];
        v.b2[1] = u.b2[0];
        l = v.l;
        }
    return l;
    }
// inlined instead...
#if 0
void Endian::swapit(uint16 &a) {
    a = (((a>>8)&0xff)|((a<<8)&0xff00));
    }
void Endian::swapit(int16 &a) {
    uint16 a0 = a;
    swap(a0);
    a = a0;
    }
void Endian::swapit(int32 &a) {
    uint32 a0 = a;
    swap(a0);
    a = a0;
    }
void Endian::swapit(uint32 &a) {
    a = ((a>>24)&0xff)|
        ((a>>8)&0xff00)|
        ((a<<8)&0xff0000)|
        ((a<<24)&0xff000000);
    }
#endif

//==========================================================================
//defines for class Strstuffs
char *Strstuffs::dup_str(const char *s) {
	//for development, can keep track of pointers to insure that all are freed
    char *t = (char*)MALLOC(strlen(s)+1);
    if (!t) { 
    	DBG_ERR(("outamem! can't create dup_str streing!\n")); 
    	return 0; 
    	}
    strcpy(t,s);
    return t;
    }
char* Strstuffs::strbuf() {
    // print to circular buffer so that we can rely on the return
    //  value not to be clobbered by calls to vstr up to 4 times.
    static char cbuf[MAXVSTR][1024];
    static int i = 0;
    char *p = cbuf[i];
    i++; i %= MAXVSTR;
    return p;
    }
char* Strstuffs::vfmt_str(const char *fmt, va_list ap) {
	char *p = strbuf();
    vsprintf(p,fmt,ap);
    return p;
    }
char* Strstuffs::fmt_str(const char *fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
	char *s = strbuf();
    vsprintf(s,fmt,ap);
    va_end(ap);
    return s;
    }
char* Strstuffs::str(const char *pstr) {
	char *s = strbuf();
    strcpy(s,pstr);
    return s;
    }
char* Strstuffs::str(int i, const char *pstr) {
	char *s = strbuf();
    strncpy(s,pstr,i);
    s[i] = 0;
    return s;
    }
    
//==========================================================================
//defines for class BufEaters
//		advances buf, modifies byte_size, and returns requested type

//for eating 1 bite out of the buffer
unsigned char BufEaters::eat_bite() {
	DBG_ASSERT(_buf && _buf_size>=1);
	if (!(_buf && _buf_size>=1)) {
		_buf_size=0; return 0; 
		}
	unsigned char x = *_buf;
	_buf_size-=1; _buf+=1;
	return x;
	}
//for eating bites out of the buffer
uint16 BufEaters::eat_uint16() {
	DBG_ASSERT(_buf && _buf_size>=2);
	if (!(_buf && _buf_size>=2)) {
		_buf_size=0; return EOF; 
		}
	uint16 x;
	memcpy(&x,_buf,2);
	x = _endian->swap(x);
	_buf_size-=2; _buf+=2;
	return x;
	}
uint32 BufEaters::eat_uint32() {
	DBG_ASSERT(_buf && _buf_size>=4);
	if (!(_buf && _buf_size>=4)) { 
		_buf_size=0; return 0; 
		}
	uint32 x;
	memcpy(&x,_buf,4);
	x = _endian->swap(x);
	_buf_size-=4; _buf+=4;
	return x;
	}
unsigned char* BufEaters::eat_nbites(uint32 n) {
	DBG_ASSERT(_buf && _buf_size>=n);
	if (!(_buf && _buf_size>=n)) { 
		_buf_size=0; return 0; 
		}
	unsigned char* m = _buf;
	_buf_size-=n; _buf+=n;
	return m;
	}
char* BufEaters::eat_str() {
	DBG_ASSERT(_buf && _buf_size>0);
	if (!(_buf && _buf_size>=0)) { 
		_buf_size=0; return 0; 
		}
	char* s = (char*)_buf;
	uint32 slen = strlen(s)+1;
	DBG_ASSERT(_buf_size>=slen);
	if (!(_buf_size>=slen)) { 
		_buf_size=0; return 0; 
		}
	_buf_size -= slen;
	_buf += slen;
	return s;
	}
//for picking stuff out of the buffer without advancing ptrs
uint32 BufEaters::pick(uint32 x,uint32 off) {
	DBG_ASSERT(_buf && _buf_size>=off+4);
	if (!(_buf && _buf_size>=off+4)) { 
		return 0; 
		}
	x = _endian->swap(*(uint32*)(_buf+off));
	return x;
	}
uint16 BufEaters::pick(uint16 x,uint32 off) {
	DBG_ASSERT(_buf && _buf_size>=off+2);
	if (!(_buf && _buf_size>=off+2)) { 
		return 0; 
		}
	x = _endian->swap(*(uint16*)(_buf+off));
	return x;
	}
unsigned char BufEaters::pick(char x,uint32 off) {
	DBG_ASSERT(_buf && _buf_size>=off+1);
	if (!(_buf && _buf_size>=off+1)) { 
		return 0; 
		}
	x = *(_buf+off);
	return (unsigned char)x;	//in case gets assigned to something bigger than char
	}
//for adding stuff back to buffer if we really weren't supposed to eat it
void BufEaters::puke(uint32 v) { 
	DBG_ASSERT(_buf && _buf-4>=_start);
	if (!(_buf && _buf-4>=_start)) { return; }
	_buf_size+=4; _buf-=4; 
	_endian->swapit(v); memcpy(_buf,&v,4); 
	}	
void BufEaters::puke(uint16 v) {
	DBG_ASSERT(_buf && _buf-2>=_start);
	if (!(_buf && _buf-2>=_start)) { return; }
	_buf_size+=2; _buf-=2; 
	_endian->swapit(v); 
	_buf[0]=(v&0xff00)>>16; _buf[1]=v&0xff; 
	}	
void BufEaters::puke(unsigned char*b,uint32 n) { 
	DBG_ASSERT(_buf && _buf-n>=_start);
	if (!(_buf && _buf-n>=_start)) { return; }
	_buf_size+=n; _buf-=n; 
	memcpy(_buf,b,n);
	}	
void BufEaters::puke(char*b) { 
	DBG_ASSERT(b);
	int n = strlen(b)+1;	//include 0
	DBG_ASSERT(_buf && _buf-n>=_start);
	if (!(_buf && _buf-n>=_start)) { return; }
	_buf_size+=n; _buf-=n; 
	memcpy(_buf,b,n);
	}	

#ifdef __3DO_DEBUGGER__
//==========================================================================
//simple Stack class
Stack::Stack(void)
{
	_top=0; //always one past last element in stack
	fStackBuffer = (_elem *) MALLOC(MAX_STACK_SIZE * sizeof(_elem));

//	dkk - For now, we'll get put into MacsBug because the growzone proc will get hit if
//	the allocation fails, but this allocation failure should somehow get reported.
//	if (fStackBuffer == nil)
//		DebugStr("\pStack Allocation failed");
}


Stack::~Stack()
{ 
	if (fStackBuffer)
		FREE((Ptr) fStackBuffer);
}


Boolean Stack::empty()
{
	return (Boolean) (_top <= 0);
}


void Stack::push(uint32 o,uint32 v)
{
//	dkk - This error should somehow get reported, rather then just returning.
	if (full())
		return;
//		DebugStr("\pStack Overflow");
		
	Stack::_elem* e = &fStackBuffer[_top++];
	e->i1 = o;
	e->i2 = v; 
}


void Stack::top(uint32& o,uint32& v)
{ 
	if (empty())
	{
		o=0; v=0;
		return;
	}
	Stack::_elem* e = &fStackBuffer[_top - 1];
	o = e->i1;
	v = e->i2; 
}


void Stack::pop(uint32& o,uint32& v)
{ 
	top(o,v);
	_top--;
}
#else
//==========================================================================
//simple Stack class
Stack::Stack(Heap* heap) {
	_heap = heap;
	_top=0; //always one past last element in stack
	_n=0; 	//maximum number of stack elements allocated
			//_n is the same as _nchunks*CHUNK_SIZE
	_nchunks=0; //number of chuncks allocated
			//current stack chunk will be _chunks[_nchunks-1];
	}
Stack::~Stack() { 
	if (!empty()) {
		DBG_WARN(("deleting non-empty stack! %d elements left!\n",_top));
		}
	DBG_ASSERT(empty());
	for (int i=0; i<_nchunks; i++) {
		DBG_ASSERT(_schunk[i]);
		DELETE_ARRAY(_schunk[i]);
		}
	}
Boolean Stack::empty() { return (Boolean) (_top<=0); }
Boolean Stack::new_chunk() {
	if (_nchunks+1>=MAX_NCHUNKS) {
		DBG_ERR(("stack size greater than that allowed!\n"));
		return false;
		}
	_elem* s;
	s = (_elem*)NEW(_elem[CHUNK_SIZE]);
	if (!s) {
		DBG_ERR(("# Unable to alloc stack chunk!\n"));
		return false;
		}
	_schunk[_nchunks++] = s;
	_n+=CHUNK_SIZE;
	return true;
	}
Boolean Stack::delete_chunk() {
	if (_nchunks<=0) {
		DBG_ERR(("can't delete chunk - no chunks!\n"));
		return false;
		}
	DBG_ASSERT(_schunk[_nchunks-1]);
	if (!_schunk[_nchunks-1]) {
		DBG_ERR(("can't delete chunk - stack empty!\n"));
		return false;
		}
	--_nchunks;
	DELETE_ARRAY(_schunk[_nchunks]);
	_n-=CHUNK_SIZE;
	return true;
	}
void Stack::push(uint32 o,uint32 v) { 
		if (full()) {
			if (_top>=MAX_STACK_SIZE) {
				DBG_ERR(("stack elements exceeds stack size!\n"));
				return;
				}
			if (!new_chunk()) {
				DBG_ERR(("# Unable to create more memory to grow stack!\n"));
				return;
				}
			}
		_top++;
		Stack::_elem* e = top_elem();
		e->i1=o; e->i2=v; 
		}
void Stack::top(uint32& o,uint32& v) { 
		if (empty()) {
			DBG_ERR(("stack underflow!\n"));
			o=0; v=0;
			return;
			}
		Stack::_elem* e = top_elem();
		o=e->i1; v=e->i2; 
		}
void Stack::pop(uint32& o,uint32& v) { 
	//if have two chunks too big, delete top chunk for space 
		if (_top>>CHUNK_SELECTOR<_nchunks-1) {
			if (!delete_chunk()) return;	//delete top chunk
			}
		top(o,v);
		_top--;
		}
#endif	// __3DO_DEBUGGER__


Queue::Queue() {
	_num = 0;
	_chunk_head = 0;
	_queue_tail = 0;
	_free_pool = 0;
	}

Queue::~Queue() { 
	if (!empty()) {
		DBG_WARN(("deleting non-empty stack! %d elements left!\n",_num));
		}
	DBG_ASSERT(empty());
	while (_chunk_head) {
		_chunk *t = _chunk_head;
		_chunk_head = _chunk_head->_next;
		DELETE(t);
		}
	}

Queue::_elem* Queue::new_elem(uint32 o, uint32 v) {
    _elem *t = 0;
	int i;
    if (!_free_pool) {
	    _chunk *nc = NEW(_chunk);
// dkk 96/03/10 - Needlessly inefficient way of linking free blocks together.
//	    for (i=0; i < CHUNK_SIZE; i++) {
//			// add newly allocated chunk of elem to _free_pool
//			nc->_elem_pool[i]._next = _free_pool;
//		    _free_pool = &(nc->_elem_pool[i]);
//			}
	    _elem* elemPtr = &nc->_elem_pool[0];
		for (i=0; i < CHUNK_SIZE - 1; i++, ++elemPtr)
	    {
			// add newly allocated chunk of elem to _free_pool
			elemPtr->_next = elemPtr + 1;
		}
		_free_pool = nc->_elem_pool;
		elemPtr->_next = 0;
		
		nc->_next = _chunk_head;
		_chunk_head = nc;
	    }
	t = _free_pool;
	_free_pool = _free_pool->_next;
	t->_i1 = o; t->_i2 = v;
	return t;
    }

void Queue::free_elem(_elem *e) {
	// put back to free pool, delete the chunk at the end will free it.
	e->_next = _free_pool;
	_free_pool = e;
	}

void Queue::add(uint32 o,uint32 v) {
	_elem *t = new_elem(o,v);
	if (!_queue_tail) {
		_queue_tail = t;
		}
	t->_next = _queue_tail->_next;
	_queue_tail->_next = t;
	_queue_tail = t;
	_num++;
	}

void Queue::bot(uint32& o,uint32& v) { 
		if (empty()) {
			DBG_ERR(("queue underflow!\n"));
			o=0; v=0;
			return;
			}
		o=_queue_tail->_next->_i1; v=_queue_tail->_next->_i2; 
		}

void Queue::rmv(uint32& o,uint32& v) { 
		bot(o,v);
		_num--;
		if (_num) {
			// remove head _elem.
			_elem *t = _queue_tail->_next;
			_queue_tail->_next = t->_next;
			// put in _free_pool
			free_elem(t);
			}
		else {
			free_elem(_queue_tail);
			_queue_tail = 0;
			}
		}

//==========================================================================
//simple Hash class for strings
Hash::_hent::_hent(_hent*& chain, const char* name, void* ptr)
:	_ptr(ptr)
,	_name(name)
,	_chain(chain)
{
	chain = this;
}

Hash::Hash(int size, Heap* heap) {
		DBG_ASSERT(heap);
		DBG_ASSERT(size>0);
		_heap=heap;
		_hsize=size;
		_hash_ind=0;  
		_hash_ptr=0;
		_htable=(_hent**)HEAP_NEW(_heap,_hent*[size]);
		if (!_htable) {
			DBG_ERR(("# Unable to create hash table!\n"));
			return;
			}
		memset(_htable,0,size*(sizeof(_hent*)));
		}

//-------------------------------------------
Hash::~Hash()
//-------------------------------------------
{
	if (_htable)
	{
		for (int i=0; i<_hsize; i++)
		{
			if (_htable[i])
				delete_chain(_htable[i]);
		}
		HEAP_DELETE(_heap,_htable);
	}
} // Hash::~Hash()

//-------------------------------------------
void Hash::delete_chain(_hent* h)
//-------------------------------------------
{
	//	if (h->_chain) delete_chain(h->_chain);
	// NO! recursion is slow and causes stack overflow.
	_hent* current = h;
	while (current)
	{
		_hent* next = current->_chain;
		HEAP_DELETE(_heap,current);
		current = next;
	}
} // Hash::delete_chain

//-------------------------------------------
inline uint32 Hash::compute_hash(const char* name, HashOff n)
//-------------------------------------------
{
		DBG_ASSERT(name);
		register uint32 h = (int)n;
		char c;
		do {
			h += (c = *name++);
		} while (c)
//JRM	for (int i=0; i < strlen(name); i++) 
//JRM		h += name[i];
//JRM	h += (int)n;
		DBG_ASSERT(n < _hsize);	//the hash offset must be less than the hash size!
		h %= _hsize;
		return h;
} // Hash::compute_hash

//-------------------------------------------
void Hash::add_hash(const char* name, void* ptr, HashOff n)
//-------------------------------------------
{
	DBG_ASSERT(name);
	int h=compute_hash(name, n);
	if (!(HEAP_NEW(_heap, _hent(_htable[h], name, ptr))))
	{
		DBG_ERR(("# Unable to add name to hash table!\n"));
		return;
	}
}

//-------------------------------------------
void* Hash::set_hash(const char* name, void* ptr, HashOff n)
//-------------------------------------------
{
	DBG_ASSERT(name);
	int h=compute_hash(name,n);
	for (_hent* hent = _htable[h]; hent; hent=hent->_chain)
	{
		if (!mystrcmp(name, hent->_name))
		{
				void* save_ptr = hent->_ptr;
				hent->_ptr=ptr;
				return save_ptr;
				}
			}
		return 0;
}

//-------------------------------------------
void* Hash::get_hash(const char* name, HashOff n)
//-------------------------------------------
{
	DBG_ASSERT(name);
	int h=compute_hash(name,n);
	for (_hent* hent = _htable[h]; hent; hent = hent->_chain)
	{
		if (!mystrcmp(name, hent->_name))
			return hent->_ptr;
	}
	return 0;
}

//-------------------------------------------
void* Hash::first_hash()
//-------------------------------------------
{
	_hash_ind = 0;
	_hash_ptr = _htable[0];
	if (_hash_ptr && _hash_ptr->_ptr)
		return _hash_ptr->_ptr;
	else
		return next_hash();
}

void* Hash::next_hash() { 
	while (_hash_ind<_hsize && (!_hash_ptr || !_hash_ptr->_ptr)) {
		//advance ptrs
		if (_hash_ptr && _hash_ptr->_chain)
			_hash_ptr=_hash_ptr->_chain;	//get next element in this hash cell
		else {							//get next hash cell
			_hash_ind++;
			if (_hash_ind==_hsize) return 0;
			_hash_ptr = _htable[_hash_ind];
			}
		}
	if (_hash_ptr && _hash_ptr->_ptr) 
		return _hash_ptr->_ptr;
	else
		return 0;
	}
void Hash::rmv_hash() {   //rmv the reference to the current itteration
	_hash_ptr->_ptr=0;
	}   

		
//====================================================================
// memory

#define HEAP_FATAL_ERR 		0x1000
#define HEAP_MALLOC_ERR 		(4 | HEAP_FATAL_ERR)	 //call to malloc failed
static Heap _global_heap;		//for mallocs/frees
static Heap _global_tmp_heap;	//for mallocs/frees who's lifetimes are short
static Heap _global_objheap;	//for news/deletes
Heap* global_heap=&_global_heap;
Heap* global_tmp_heap=&_global_tmp_heap;
Heap* global_objheap=&_global_objheap;
#if defined(macintosh) && defined(DEBUG)
	static Heap _global_handles;	//for NewHandles/DisposeHandles
	static Heap _global_ptrs;		//for NewPtrs/DisposePtrs
	Heap* global_handles=&_global_handles;
	Heap* global_ptrs=&_global_ptrs;
#endif
#ifdef NewPtr
#undef NewPtr
#endif

#define _USEHEAP
#ifdef DEBUG
    #define MAX_PTRS 4000
#else
    #define MAX_PTRS 1
#endif

int Heap::_max_ptrs = MAX_PTRS;
#ifndef _USEHEAP
	static _heap_ptr _ptrs_array[MAX_PTRS];
#endif

Heap::Heap(State* state) {
	DBG_ASSERT(this);
	#ifdef DEBUG
		_ptr_ind=0;
		#ifdef _USEHEAP
			#ifdef macintosh
				_ptrs = (_heap_ptr*) NewPtr(sizeof(_heap_ptr)*MAX_PTRS);
			#else
				_ptrs = (_heap_ptr*) malloc(sizeof(_heap_ptr)*MAX_PTRS);
			#endif
			DBG_ASSERT(_ptrs);
		#else
			_ptrs = _ptrs_array;
		#endif
	#endif
	if (this) {
		_state = state;
		}
	else {
		DBG_ERR(("Heap class not yet created!\n"));
		if (state) state->set_state(HEAP_MALLOC_ERR);
		}
	}
Heap::~Heap() {
#ifdef DEBUG
	DBG_ASSERT(this);
	if (!this) {
		DBG_ERR(("invalid Heap class!\n"));
		return;
		}
	heap_check();
	#ifdef _USEHEAP
		#ifdef macintosh
			if (_ptrs) DisposePtr((Ptr)_ptrs);
		#else
			if (_ptrs) free(_ptrs);
		#endif
	#endif
#endif
	}
#ifdef DEBUG
void Heap::heap_check() {
	DBG_ENT("heap_check");
	DBG_ASSERT(this);
	if (!this) {
		DBG_ERR(("invalid Heap class!\n"));
		return;
		}
	DBG_ASSERT(_ptrs);
	if (!_ptrs) return; 
	if (_ptr_ind) {
		DBG_ERR(("HEAP: MEMORY LEAK. %d ptrs left in ptr list!!\n",_ptr_ind));
		for (int i=0; i<_ptr_ind; i++) {
			DBG_ERR(("HEAP: ptr x%X in file %s at line %d\n",
				_ptrs[i]._ptr,_ptrs[i]._file,_ptrs[i]._line));
			}
		}
	}

Boolean Heap::full() {
	if (!_ptrs || _ptr_ind>=_max_ptrs) {
		#ifdef _USEHEAP
		DBG_WARN(("HEAP: attempting to add more space for ptr list!!\n"));
		#ifdef macintosh
			DisposePtr(_ptrs);
			_ptrs = (_heap_ptr*) NewPtr(sizeof(_heap_ptr)*(_max_ptrs+MAX_PTRS));
		#else
			_ptrs = (_heap_ptr*) realloc(_ptrs,sizeof(_heap_ptr)*(_max_ptrs+MAX_PTRS));
		#endif
		if (_ptrs) {
			_max_ptrs += MAX_PTRS;
			return false;
			}
		#endif /* _USEHEAP */
		return true;
		}
	return false;
	}
Boolean Heap::add_ptr(void* x, const char* file,uint32 line) {
	DBG_ASSERT(this);
	if (!this) return false; 
	DBG_ASSERT(_ptrs);
	if (full()) {
		DBG_ERR(("HEAP: no more space in ptr list!!\n"));
		DBG_ERR(("HEAP: error occurred in file %s at line %d\n",file?file:"???",line));
		DBG_ASSERT(0);
		return false; 
		}
	_ptrs[_ptr_ind]._ptr = x;
	_ptrs[_ptr_ind]._file = file;
	_ptrs[_ptr_ind]._line = line;
	DBG_(PICKY,("HEAP: Added ptr x%X in file %s at line %d\n",
			_ptrs[_ptr_ind]._ptr,_ptrs[_ptr_ind]._file,_ptrs[_ptr_ind]._line));
	_ptr_ind++;
	return true;
	}
Boolean Heap::rmv_ptr(void* x,char*file,uint32 line) {
	DBG_ASSERT(this);
	if (!this) return false; 
	DBG_ASSERT(_ptrs);
	if (!_ptrs) return false; 
	for (int i=0; i<_ptr_ind; i++) {
		if (_ptrs[i]._ptr==x) {
			DBG_(PICKY,("HEAP: Removed ptr x%X in file %s at line %d\n",
				_ptrs[i]._ptr,_ptrs[i]._file,_ptrs[i]._line));
			_ptr_ind--;
			_ptrs[i]._ptr=_ptrs[_ptr_ind]._ptr;
			_ptrs[i]._file=_ptrs[_ptr_ind]._file;
			_ptrs[i]._line=_ptrs[_ptr_ind]._line;
			return true;
			}
		}
	DBG_ERR(("HEAP: ptr x%X not in ptr list!!\n",x));
	DBG_ERR(("HEAP: error occurred in file %s at line %d\n",file?file:"???",line));
	DBG_ASSERT(0);
	return false;
	}
#endif/* DEBUG */

void* Heap::heap_new(void* x, const char* file, uint32 line)
{ 
	DBG_ASSERT(this);
	if (!x)
	{
		DBG_ERR(("HEAP: HEAP_MALLOC_ERR.  no more memory\n")); 
		DBG_ERR(("HEAP: error occurred in file %s at line %d\n",file,line));
		if (_state) _state->set_state(HEAP_MALLOC_ERR, file, line); 
		DBG_ASSERT(0);
		return 0;
		}
	#ifdef DEBUG
		add_ptr(x,file,line);
	#endif
	return x; 
}

void Heap::heap_delete(void* x, const char* file,uint32 line) {
	DBG_ASSERT(this);
	#ifdef DEBUG
		if (!x || !rmv_ptr(x,file,line)) 
	#else
		if (!x) 
	#endif
		{
		DBG_ERR(("HEAP: se_malloc_err occurred.  attempt to free ptr x%X not in heap\n",x)); 
		DBG_ERR(("HEAP: error occurred in file %s at line %d\n",file,line));
		if (_state) _state->set_state(HEAP_MALLOC_ERR,file,line); 
		DBG_ASSERT(0);
		return;
		}
	return; 
	}
	
//====================================================================
// state

int State::state(char*& fname, uint32& line) {
	fname= _file;
	line = _line;
	return _state;
	}
		
void State::set_state(int state, const char* file, uint32 line) {
	_state = state;
	if (file) {
		int len = min(strlen(file),MAX_FNAME);
		strncpy(_file,file,len);
		_file[len] = 0;
		}
	else *_file=0;
	_line = line;
    }

//====================================================================
// misc i/o funcs

#ifndef __3DO_DEBUGGER__
FILE* open_(const char* fname) {
	FILE* fp=0;
    DBG(("open_(%s)\n",fname));
	if (!mystrcmp(fname,"stdout"))
		fp=stdout;
	else if (!mystrcmp(fname,"stderr"))
		fp=stderr;
	else if ((fp=fopen(fname,"w+"))==NULL) {
    	DBG_ERR(("ERROR!  unable to create %s!\n",fname));
    	return 0;
    	}
    return fp;
    }
    
FILE* openb_(const char* fname) {
	FILE* fp=0;
    DBG(("openb_(%s)\n",fname));
	if ((fp=fopen(fname,"w+b"))==NULL) {
    	DBG_ERR(("ERROR!  unable to create %s!\n",fname));
    	return 0;
    	}
    return fp;
    }
    
void close_(FILE*& fp) {
    if (fp && fp!=stdout && fp!=stderr) {
    	fclose(fp);
    	fp = 0;
    	}
	}

Boolean exists(const char* fname) {
	DBG_ENT("exists");
#ifdef macintosh
	FInfo fndrInfo; 
	return (getfinfo(fname, 0, &fndrInfo) == noErr);
#else
	FILE* fp;
	if ((fp=fopen(fname,"rb"))!=NULL) {
		fclose(fp);
		return true;
		}
	return false;
#endif
	}
//tests for path separator
//mmh 9/06/1996 No need for ifdefs, now the linker will test for path 
//separator first
//====================================================================
void PathString::test()
{
	int i;

	for (i=0;pathname;i++)
	{
		separator = pathname[i];
		if((separator == UNIX_SEP) || (separator == DOS_SEP))
			break;
		else if ((separator == MAC_SEP) && (pathname[i+1]!=DOS_SEP))
		    break;
			 
	}
}


//====================================================================
const char* rmv_path(const char* fname) {
	const char* p=0;
	PathString our_path(fname);
	our_path.test();
	if (fname) {
		p = strrchr(fname,our_path.get_separator());
		if (p) p++;
		else
//mmh removed the cygwin stuff that unixified pathnames 09/06/96
        p=fname;
		}
	return p;
	}

//================================================================
//expand_at_files & misc conv funcs

//Don't need this for debugger, but it's
//a useful tool - would rather put in a library
//as a separate object
#define COMMENT '#'
enum GWStates { 
	gw_skip_white_space,
	gw_skip_comment,
	gw_get_word
	};
static char *getword(FILE *fp) {
    static char buf[256];
    GWStates state = gw_skip_white_space;
    int c;
	char *p;
    p = buf;
    while (c = fgetc(fp), c != EOF) {
		switch (state) {
		    case gw_skip_white_space:
				if (c == COMMENT)
					state = gw_skip_comment;
				else if (!(Parse::iswhite(c))) {
				    *p++ = c;
				    state = gw_get_word;
				    }
			    break;
		    case gw_get_word:
				if (Parse::iswhite(c)) {
					*p = 0;
					return buf;
				    }
				else if (c == COMMENT) {
					state = gw_skip_comment;
					*p = 0;
					}
				else 
				    *p++ = c;
				break;
		    case gw_skip_comment:
				if (c == NEW_LINE) {
				    if (p != buf)
						return buf;
					else
						state = gw_skip_white_space;
				    }
				break;
			}
	    }
    if (c == EOF && p != buf) {
		*p = 0; return buf;
		}
	return 0;
	}

void expand_at_files(int *argc, char ***argv) {
    int iac = *argc;
    char **iav = *argv;
    int i;
    Boolean has_atfile = false;
	for (i = 1; i < iac ; i++) {
	    if (iav[i][0] == '@') {
		     has_atfile = true;
		     break;
		     }
		}
    if (!has_atfile) return;
    char **nav = new char*[1024];	// hack: max of 1024 arguments.
    int nac = 0;
    for (i=0; i < iac; i++) {
	    if (iav[i][0] == '@') {
		    FILE *fp = fopen(&(iav[i][1]),"r");
		    if (fp == 0) {
				fprintf(stderr, "warning: can't open at file\n");
			    continue;
			    }
		    char *s;
			while (s = getword(fp), s) {
				int l = strlen(s);
				char *t = new char[l+1];
			    strcpy(t,s);
				nav[nac++] = t;		//copy args from at file
				}
			fclose(fp);
			}
	    else {
		    nav[nac++] = iav[i];	//copy command line args
		    }
	    }
    nav[nac] = 0;
    *argv = nav;
    *argc = nac;
    }
//====================================================================
//catch xcpts and signals
#include <signal.h>
//generic xcpt handler
//we'll do this in c so we don't have to deal with destructors in case of memory problems
//parms:	(we don't use them now, but might want to later)
//			mac only takes sig :-(
//	sig is the signal number
//	code is a parameter  of  certain  signals  that  provides  additional  detail
//	scp is a pointer to the sigcontext structure (defined in <signal.h>),
//		used to restore the context from before the signal
//	addr is additional address information
//	(see sigvec(2))
static XcptHandler user_handler=0;
#ifdef macintosh
extern "C" void xcpt_handler(int sig);
extern "C" void xcpt_handler(int sig)
{
	signal(SIGINT,xcpt_handler); //if cntl c inside of handler, catch it
	if (user_handler) user_handler(sig);	//invoke user's handler if one is present
    exit(1);
}
#else
static void xcpt_handler(int sig, int code, struct sigcontext *scp, char* addr)
{
//static void xcpt_handler(int /*sig*/) {
//	signal(SIGINT,xcpt_handler); //if cntl c inside of handler, catch it
    signal(SIGINT,(void (*)(int))xcpt_handler);
	if (user_handler) user_handler(sig, code, scp, addr);	//invoke user's handler if one is present
    exit(1);
}
#endif
void register_xcpt_handler(XcptHandler handler) {
/*	#if defined(_SUN4) || defined(_MSDOS) 	//what about mac??
	user_handler = handler;
    signal(SIGINT,xcpt_handler);	//catch interrupts
    signal(SIGILL,xcpt_handler);	//catch illegal instruction (not reset when caught)
    signal(SIGFPE,xcpt_handler);	//catch floating point exception */
  /*  signal(SIGBUS,xcpt_handler);	//catch bus error 
    signal(SIGSEGV,xcpt_handler);	//catch segmentation violation
	#endif*/
	#if defined(_SUN4) || defined(_MSDOS) 	//what about mac??
	user_handler = handler;
	signal(SIGINT,(void (*)(int))xcpt_handler);	//catch interrupts
	signal(SIGILL,(void (*)(int))xcpt_handler);	//catch illegal instruction (not reset when caught)
	signal(SIGFPE,(void (*)(int))xcpt_handler);	//catch floating point exception */
#ifndef _MSDOS
	signal(SIGBUS,(void (*)(int))xcpt_handler);	//catch bus error
#endif
	signal(SIGSEGV,(void (*)(int))xcpt_handler);	//catch segmentation violation
#endif
	}

//====================================================================
// misc conversion funcs


//convert char string to value (eg. "0x01aB1", "360")
uint32 ch2val(const char* cval) {
    char *p;
    uint32 val;
    if (!cval) return 0;
    if ((p=strchr(cval,'x'),p) || (p=strchr(cval,'X'),p)) {
        p++;
        val=strtoul(p, 0, 16);
        }   
    else
        val=strtoul(cval, 0, 10);
    return val;
    }

//convert char string to time in seconds
//	valid strings:
//		MM/DD/YY hh:mm:ss 
//		MM/DD/YY,hh:mm:ss 
//		now
static uint32 mdays[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

extern "C" int gettimeofday(timeval* tp, timezone *tzp);
uint32 str2time(char *ts) {
    int32 month, date, year, hrs, mins, secs;
    time_t dummy;
    int32 corr;

	if (!mystrcmp(ts,"now")) {
		//date=today time=now
#ifdef _MSDOS
		time_t t=0;
//FIXME.
		//time(&t); 
		//struct tm *tv = _localtime(&t);
		return t;
		//return (tv.tm_sec - 23*365*24*60*60 - 6*24*60*60);
#else
		struct timeval tv;
		if (gettimeofday(&tv, (struct timezone *)NULL) < 0)
		{
			DBG_ERR(("Error in getting time-of-day\n"));
			return 0;
		}
		return (tv.tv_sec - 23*365*24*60*60 - 6*24*60*60);
#endif
		}

    //sanity checks 
    if (sscanf(ts,"%u/%u/%u %u:%u:%u",&month,&date,&year,&hrs,&mins,&secs) != 6
    	&& sscanf(ts,"%u/%u/%u,%u:%u:%u",&month,&date,&year,&hrs,&mins,&secs) != 6) {
		DBG_ERR(("invalid time %s\n",ts));
		return 0;
		}
    year -= 100*(year/100);	/* get last two digits of the year */
    if (year < 92)
		return 0;
    if ((month < 1) || (month >12))
		return 0;
    --month;	/* zero-based */
    mdays[1] = (year & 3) ? 28 : 29;	/* set no. of days for Feb */
    if ((date < 1) || (date > mdays[month]))
		return 0;
    if ((hrs > 24) || (mins >= 60) || (secs >= 60))
		return 0;
    secs += mins*60 + hrs*60*60 + (date-1)*24*60*60;

    while (--month >= 0)
		secs += mdays[month]*24*60*60;

    year -= 93;	/* zero-based no. of years */
    if (year < 0)
		secs -= 366*24*60*60;	/* This must be year '92 */
    else
		secs += year*365*24*60*60 + (year/4)*24*60*60;

    dummy = secs + (23*365+6)*24*60*60;	/* secs till first day of '70 */

#ifdef	__mips
    if (localtime(&dummy)->tm_isdst > 0)
		corr = altzone;
    else
		corr = timezone;
#elif defined(sparc)	/* __mips */
    corr = -localtime(&dummy)->tm_gmtoff;/* convert to GMT */
#else
	corr = 0;
#endif	/* __mips/sparc */

    secs += corr;
    if (secs < 0)
		return 0;

    return (uint32)secs;
	}


char* time2str(time_t secs) {
    /* secs is the number of seconds since 01/01/93 00:00:00 GMT */
    static char	  prbuf[18] = { '\0', };
    struct tm *lt;

    secs += ((23*365+6)*24*60*60);
    lt = localtime(&secs);

    sprintf(prbuf,"%02d/%02d/%02d %02d:%02d:%02d", lt->tm_mon+1, lt->tm_mday,
			lt->tm_year, lt->tm_hour, lt->tm_min, lt->tm_sec);
    return prbuf;
}


//================================================================
//code to support command-period:
#if defined(macintosh) && !defined (_MW_APP_)

#include <stdlib.h>
#include <CursorCtl.h>

Boolean KeyBit(KeyMap keyMap, char keyCode) {
	char* keyMapPtr = (char*)keyMap;
	char keyMapByte = keyMapPtr[keyCode / 8];
	return ((keyMapByte & (1 << (keyCode % 8))) != 0);
	} // KeyBit

Boolean UserAbort() {
	const char	kAbortKeyCode = 47;	// US "Period" key code.	
	const char	kCmdKeyCode = 55;	
	KeyMap	theKeyMap;	
	GetKeys(theKeyMap);
	
	return KeyBit(theKeyMap, kCmdKeyCode)
		&& KeyBit(theKeyMap, kAbortKeyCode);
	} // UserAbort()

BusyCursor::BusyCursor() {
	InitCursorCtl(0);
	//atexit(exithandler);	//for installing handlers - maybe later...
	}

void BusyCursor::Spin() {
	if (UserAbort()) // check first, the built-in SpinCursor version has nasty bug
		exit(-9); // conventional status value for user abort
	static unsigned char callCount = 0;
	// Spin every 256 calls
	if (!++callCount)
		SpinCursor(16);
	}
#endif /* macintosh */

void busy_cursor() {
#if defined(macintosh) && !defined(_MW_APP_)
	static BusyCursor busybee;
	busybee.Spin();
#else
	//may want this later...
	//fprintf(stdout,".");
#endif
	}

#endif /* __3DO_DEBUGGER__ */

