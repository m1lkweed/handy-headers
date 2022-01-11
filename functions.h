// Adds lambdas, closures, and patchable functions
// Original code (c)Chris Wellons (wellons@nullprogram.com)
// Released into the public domain via The Unlicense
// Updates (c)m1lkweed 2022 GPLv3+
#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include <stdlib.h> // size_t

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

#if defined(__GNUC__) && !defined(__clang__)
#define lambda(lambda$_ret, lambda$_args, lambda$_body)\
({lambda$_ret lambda$__anon$ lambda$_args lambda$_body &lambda$__anon$;})
#else
#warning Lambdas are not supported by your compiler
#endif

#define patchable __attribute__((ms_hook_prologue, aligned(8), noinline, noclone))
#define closeable __attribute__((sysv_abi))

int hotpatch(void * restrict target, void * restrict replacement);
void *closure_create(void * restrict f, size_t nargs, void * restrict userdata);
void closure_destroy(void *closure);

#ifdef FUNCTIONS_IMPLEMENTATION
#include <stdint.h>
#include <string.h>   // memcpy()
#include <unistd.h>   // getpagesize()
#include <sys/mman.h> // mprotect()

#ifdef _REENTRANT
#include <threads.h>

static mtx_t hotpatch_mutex;
static once_flag hotpatch_mutex_flag;

static mtx_t closure_mutex;
static once_flag closure_mutex_flag;

static inline void _$void_hotpatch_mtx_init$(void){
	mtx_init(&hotpatch_mutex, mtx_plain);
}

static inline void _$void_closure_mtx_init$(void){
	mtx_init(&closure_mutex, mtx_plain);
}
#endif

patchable static void _$hotpatch_dummy_func$(void){}

int hotpatch(void * restrict target, void * restrict replacement){
#ifdef _REENTRANT
	call_once(&hotpatch_mutex_flag, _$void_hotpatch_mtx_init$);
#endif
	if(target == replacement || target == NULL)
		return -1; // would cause an endless loop or NULL dereference otherwise
	uint8_t *check = target;
	uint64_t * check8 = target;
	uint64_t * dummy8 = (void*)_$hotpatch_dummy_func$;
	if(UNSAFE_HOTPATCH_CHECK && (check[0] != 0xe9 && check8[0] != dummy8[0])) // not a jmp or empty patchable, don't patch
		return -1;
	if(_Alignof(target) < 8) // not aligned, don't patch
		return -1;
	void *page = (void *)((uintptr_t)target & ~0xfff);
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
	if(replacement == NULL){
		memcpy(target, _$hotpatch_dummy_func$, 8);
	}else{
		uint32_t rel = (char *)replacement - (char *)target - 5;
		union {
			uint8_t bytes[8];
			uint64_t value;
		} instruction = {{0xe9, (rel >> 0) & 0xFF, (rel >> 8) & 0xFF, (rel >> 16) & 0xFF, (rel >> 24) & 0xFF}};
		*(uint64_t *)target = instruction.value;
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

static inline void _$closure_set_data$(void * restrict closure, void * restrict data){
	void **p = closure;
	p[-2] = data;
}

static inline void _$closure_set_function$(void * restrict closure, void * restrict f){
	void **p = closure;
	p[-1] = f;
}

static const unsigned char _$closure_thunk$[6][13] = {
	{0x48, 0x8b, 0x3d, 0xe9, 0xff, 0xff, 0xff, 0xff, 0x25, 0xeb, 0xff, 0xff, 0xff},
	{0x48, 0x8b, 0x35, 0xe9, 0xff, 0xff, 0xff, 0xff, 0x25, 0xeb, 0xff, 0xff, 0xff},
	{0x48, 0x8b, 0x15, 0xe9, 0xff, 0xff, 0xff, 0xff, 0x25, 0xeb, 0xff, 0xff, 0xff},
	{0x48, 0x8b, 0x0d, 0xe9, 0xff, 0xff, 0xff, 0xff, 0x25, 0xeb, 0xff, 0xff, 0xff},
	{0x4C, 0x8b, 0x05, 0xe9, 0xff, 0xff, 0xff, 0xff, 0x25, 0xeb, 0xff, 0xff, 0xff},
	{0x4C, 0x8b, 0x0d, 0xe9, 0xff, 0xff, 0xff, 0xff, 0x25, 0xeb, 0xff, 0xff, 0xff}
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
	munmap((char *)closure - page_size, page_size * 2);
#ifdef _REENTRANT
	mtx_unlock(&closure_mutex);
#endif
}

#endif // FUNCTIONS_IMPLEMENTATION

#define closure_create(f, nargs, userdata) closure_create((void *)f, nargs, (void *)userdata) /*cast to avoid warnings*/

#endif // _FUNCTIONS_H_
/*--------------------- --END OF EXTRA FUNCTIONS CODE------------------------*/
