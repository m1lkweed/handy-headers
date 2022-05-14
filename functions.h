// Adds lambdas, closures, and patchable functions
// Original code (c)Chris Wellons (wellons@nullprogram.com)
// Released into the public domain via The Unlicense
// Updates (c)m1lkweed 2022 GPLv3+
#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include <stdlib.h> // size_t
#include <stdbool.h>

#if !defined(__x86_64__)
#error Requires x86_64
#endif

#ifdef ALLOW_UNSAFE_HOTPATCH
#define UNSAFE_HOTPATCH_CHECK 0
#else
#define UNSAFE_HOTPATCH_CHECK 1
#endif // ALLOW_UNSAFE_HOTPATCH

#if !defined(__unix__) && !defined(SYSV_ABI)
#warning Requires System V ABI, use 'closeable' on closeable functions to ensure compatability. Use -DSYSV_ABI to hide this warning
#endif

#define lambda(lambda$_ret, lambda$_args, lambda$_body) ({lambda$_ret lambda$__anon$ lambda$_args lambda$_body &lambda$__anon$;})

#define patchable [[gnu::ms_hook_prologue, gnu::aligned(8), gnu::noipa]]
#define closeable [[gnu::sysv_abi]]
// Replace a patchable function with another function
int hotpatch(void * restrict target, void * restrict replacement);
// If function is patched, return the address of the replacement, else NULL
void *is_patched(void *function);
// Returns true if function is patchable, useful if ALLOW_UNSAFE_HOTPATCH is not defined
[[gnu::pure]] bool is_patchable(void *function);
// Returns a callable address of the original patched function
[[gnu::nonnull, gnu::pure]] const void *original_function(void *function);
// Creates and returns a closure around f, a function with nargs parameters, and binds userdata to the last argument
void *closure_create(void * restrict f, size_t nargs, void * restrict userdata);
// Destroys a closure
void closure_destroy(void *closure);

#ifdef FUNCTIONS_IMPLEMENTATION
#include <stdint.h>
#include <stddef.h>
#include <string.h>   // memcpy()
#include <unistd.h>   // getpagesize()
#include <sys/mman.h> // mprotect()

#ifdef _REENTRANT
#include <threads.h>

static mtx_t hotpatch_mutex;
static once_flag hotpatch_mutex_flag;

static mtx_t closure_mutex;
static once_flag closure_mutex_flag;

[[gnu::visibility("internal")]] static inline void _$void_hotpatch_mtx_init$(void){
	mtx_init(&hotpatch_mutex, mtx_plain);
}

[[gnu::visibility("internal")]] static inline void _$void_closure_mtx_init$(void){
	mtx_init(&closure_mutex, mtx_plain);
}
#endif

patchable static void _$hotpatch_dummy_func$(void){}

int hotpatch(void * restrict target, void * restrict replacement){
#ifdef _REENTRANT
	call_once(&hotpatch_mutex_flag, _$void_hotpatch_mtx_init$);
#endif
	if(target == NULL)
		return -1; // prevent NULL dereference otherwise
	uint8_t *check = target;
	uint64_t * check8 = target;
	uint64_t * dummy8 = (void*)_$hotpatch_dummy_func$;
	if(UNSAFE_HOTPATCH_CHECK && (check[0] != 0xe9 && check8[0] != dummy8[0])) // not a jmp or empty patchable, don't patch
		return -1;
	if(_Alignof(target) < 8) // not aligned, don't patch
		return -1;
	void *page = (void*)((uintptr_t)target & ~0xfff);
	int pagesize = getpagesize();
#ifdef _REENTRANT
	mtx_lock(&hotpatch_mutex);
#endif
	if(mprotect(page, pagesize, PROT_WRITE | PROT_EXEC) != 0){
#ifdef _REENTRANT
		mtx_unlock(&hotpatch_mutex);
#endif
		return -1;
	}
	if(target == replacement || replacement == NULL){ // first case would cause an endless loop, second would dereference NULL
		memcpy(target, _$hotpatch_dummy_func$, 8);
	}else{
		uint32_t rel = (char*)replacement - (char*)target - 5;
		union {
			uint8_t bytes[8];
			uint64_t value;
		} instruction = {{0xe9, (rel >> 0) & 0xFF, (rel >> 8) & 0xFF, (rel >> 16) & 0xFF, (rel >> 24) & 0xFF}};
		*(uint64_t*)target = instruction.value;
	}
	if(mprotect(page, pagesize, PROT_EXEC) != 0){
#ifdef _REENTRANT
		mtx_unlock(&hotpatch_mutex);
#endif
		return -1;
	}
#ifdef _REENTRANT
	mtx_unlock(&hotpatch_mutex);
#endif
	return 0;
}

void *is_patched(void *function){
	uint8_t *check = function;
	if(!is_patchable(function))
		return NULL;
	if(!__builtin_memcmp((void*)_$hotpatch_dummy_func$, function, 8)){
		return NULL;
	}else{
		uintptr_t address = (uintptr_t)function;
		uint32_t offset = (check[4] << 24) + (check[3] << 16) + (check[2] << 8) + check[1];
		address += offset;
		return (void*)address + 5;
	}
}

[[gnu::pure]] bool is_patchable(void *function){
	uint8_t *check = function;
	uint64_t *check8 = function;
	uint64_t *dummy8 = (void*)_$hotpatch_dummy_func$;
	return ((_Alignof(function) >= 8) && (check[0] == 0xe9 || check8[0] == dummy8[0]));
}

[[gnu::nonnull, gnu::pure]] const void *original_function(void *func){
	if(is_patchable(func))
		return &((uint8_t*)func)[8];
	return func;
}

static inline void _$closure_set_data$(void * restrict closure, void * restrict data){
	void **p = closure;
	p[-2] = data;
}

static inline void _$closure_set_function$(void * restrict closure, void * restrict f){
	void **p = closure;
	p[-1] = f;
}

static const unsigned char _$closure_thunk$[6][13] = {
	{0x48, 0x8b, 0x3d, 0xe9, 0xff, 0xff, 0xff, 0xff, 0x25, 0xeb, 0xff, 0xff, 0xff},	// dec eax; mov edi, DWORD PTR ds:0xffffffe9;jmp DWORD PTR ds:0xffffffeb
	{0x48, 0x8b, 0x35, 0xe9, 0xff, 0xff, 0xff, 0xff, 0x25, 0xeb, 0xff, 0xff, 0xff},	// dec eax; mov esi, DWORD PTR ds:0xffffffe9;jmp DWORD PTR ds:0xffffffeb
	{0x48, 0x8b, 0x15, 0xe9, 0xff, 0xff, 0xff, 0xff, 0x25, 0xeb, 0xff, 0xff, 0xff},	// dec eax; mov edx, DWORD PTR ds:0xffffffe9;jmp DWORD PTR ds:0xffffffeb
	{0x48, 0x8b, 0x0d, 0xe9, 0xff, 0xff, 0xff, 0xff, 0x25, 0xeb, 0xff, 0xff, 0xff},	// dec eax; mov ecx, DWORD PTR ds:0xffffffe9;jmp DWORD PTR ds:0xffffffeb
	{0x4c, 0x8b, 0x05, 0xe9, 0xff, 0xff, 0xff, 0xff, 0x25, 0xeb, 0xff, 0xff, 0xff},	// dec esp; mov eax, DWORD PTR ds:0xffffffe9;jmp DWORD PTR ds:0xffffffeb
	{0x4c, 0x8b, 0x0d, 0xe9, 0xff, 0xff, 0xff, 0xff, 0x25, 0xeb, 0xff, 0xff, 0xff}	// dec esp; mov ecx, DWORD PTR ds:0xffffffe9;jmp DWORD PTR ds:0xffffffeb
};

void *closure_create(void * restrict f, size_t nargs, void *userdata){
#ifdef _REENTRANT
	call_once(&closure_mutex_flag, _$void_closure_mtx_init$);
#endif
	long page_size = getpagesize();
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_ANONYMOUS | MAP_PRIVATE;
	char *p;
#ifdef _REENTRANT
	mtx_lock(&closure_mutex);
#endif
	if((p = mmap(0, page_size * 2, prot, flags, -1, 0)) == MAP_FAILED){
#ifdef _REENTRANT
		mtx_unlock(&closure_mutex);
#endif
		return NULL;
	}
	void *closure = p + page_size;
	memcpy(closure, _$closure_thunk$[nargs - 1], sizeof(_$closure_thunk$[0]));
	if(mprotect(closure, page_size, PROT_READ | PROT_EXEC) != 0){
#ifdef _REENTRANT
		mtx_unlock(&closure_mutex);
#endif
		return NULL;
	}
	_$closure_set_function$(closure, f);
	_$closure_set_data$(closure, userdata);
#ifdef _REENTRANT
	mtx_unlock(&closure_mutex);
#endif
	return closure;
}

void closure_destroy(void *closure){
	long page_size = getpagesize();
#ifdef _REENTRANT
	mtx_lock(&closure_mutex);
#endif
	munmap((char*)closure - page_size, page_size * 2);
#ifdef _REENTRANT
	mtx_unlock(&closure_mutex);
#endif
}

#endif // FUNCTIONS_IMPLEMENTATION

#define closure_create(f, nargs, userdata) ({static typeof(userdata) $ = 0; $ = userdata; (closure_create)((void*)f, nargs, (void*)*(typeof(uintptr_t)*)&$);}) /*cast to avoid warnings*/
#define original_function(f) ((typeof(&f))original_function(f))
#define is_patched(f) ((typeof(&f))is_patched(f))

#ifndef ALLOW_UNSAFE_HOTPATCH
#define HOTPATCH_TYPECMP(x, y) (_Generic((y), typeof(NULL):1, default:(_Generic((*y), typeof(&x):1, default:0))))
#define hotpatch(x, y) (HOTPATCH_TYPECMP(x, y)?hotpatch(x, y):-1)
#endif

#endif // _FUNCTIONS_H_
/*--------------------- --END OF EXTRA FUNCTIONS CODE------------------------*/
