#ifndef _EXCEPTION_H_
#error Don't use this file directly! Use `#include "exception.h"' instead.
#else
// (c) M1lkweed, 2022-2023
// Released as GPLv3+

// Differences from normal setjmp:
// 1.) volatile is not required to prevent clobbering
// 2.) sizeof(xjmp_buf) is 8
// 3.) xlongjmp can be used more than once per buffer
// 4.) reading the return value of xsetjmp is not UB
// 5.) saves __m128 and __m256 variables
// 6.) all of the points also apply to the sig*jmp family

// The following code uses tricks to avoid stack execution.
// Still, GCC gets confused and marks the stack as executable.
// Add `-znoexecstack` to prevent this.

// Even with all optimizations disabled, GCC will hoist code and
// variables inside nested functions to avoid using an executable
// stack. GCC only tries to make the stack executable if you both:
// 1.) Let the address of the nested function escape the current
//     function (passing it to a called function is fine, passing
//     it to a parent or storing it globally is not), and
// 2.) Capture any local context from outside the nested function
//     itself, whether it's a parent's variable or a goto label
// Both of these must be violated at the same time to cause an
// executable stack.
#include <stddef.h>
#include <stdint.h>

#ifndef XSETJMP_SKIP_SIGNALS
#include <signal.h>
#endif

#ifndef XSETJMP_SKIP_FENV
#include <fenv.h>
#define XFENVGET fenv_t fp_env; fegetenv(&fp_env);
#define XFENVSET fesetenv(&fp_env);
#else
#define XFENVGET
#define XFENVSET
#endif

typedef struct{
	const uint16_t mov1;
	const uint32_t addr;
	const uint16_t mov2;
	const void * const chain;
} __attribute__((packed)) thunk_struct;

#define NESTED_CHAIN(p) ({            \
	thunk_struct *__t = (void*)p; \
	__t->chain;                   \
})

#define NESTED_ADDR(p) ({                 \
	__auto_type __p = (p);            \
	thunk_struct *__t = (void*)__p;   \
	(typeof(__p))(intptr_t)__t->addr; \
})

#define NESTED_UPGRADE(self, ptr, args) ({                    \
	if(self != ptr)                                       \
		__builtin_call_with_static_chain(             \
			NESTED_ADDR((typeof(self)*)ptr) args, \
			NESTED_CHAIN(ptr)                     \
		);                                            \
})

typedef struct{
	void(*fun)(void*, int);
}xjmp_buf[1], xsigjmp_buf[1];

#define xsetjmp(env) ({                                 \
	__label__ trgt;                                 \
	int __xsetjmp_ret = 0;                          \
	XFENVGET                                        \
	void __jmp(void *self, int r){                  \
		NESTED_UPGRADE(__jmp, self, (self, r)); \
		__xsetjmp_ret = r ?: 1;                 \
		goto trgt;                              \
	}                                               \
	(env)[0].fun = __jmp;                           \
	trgt:;                                          \
	XFENVSET                                        \
	__xsetjmp_ret;                                  \
})

#define xlongjmp(env, r) ({                         \
	NESTED_ADDR((env)[0].fun)((env)[0].fun, r); \
})

#ifndef XSETJMP_SKIP_SIGNALS
#define xsigsetjmp(env, save_sigmask_p) ({              \
	__label__ trgt;                                 \
	int __xsetjmp_ret = 0;                          \
	XFENVGET                                        \
	sigset_t set = {};                              \
	if(save_sigmask_p)                              \
		sigprocmask(SIG_BLOCK, NULL, &set);     \
	void __jmp(void *self, int r){                  \
		NESTED_UPGRADE(__jmp, self, (self, r)); \
		__xsetjmp_ret = r ?: 1;                 \
		goto trgt;                              \
	}                                               \
	(env)[0].fun = __jmp;                           \
	trgt:;                                          \
	if(save_sigmask_p)                              \
		sigprocmask(SIG_SETMASK, &set, NULL);   \
	XFENVSET                                        \
	__xsetjmp_ret;                                  \
})

#define xsiglongjmp(env, r) xlongjmp(env, r)

#endif

int(xsetjmp)(xjmp_buf buf){
	return xsetjmp(buf);
}

#ifndef XSETJMP_SKIP_SIGNALS
int(xsigsetjmp)(xsigjmp_buf buf, int save_sigmask_p){
	return xsigsetjmp(buf, save_sigmask_p);
}
#endif

void(xlongjmp)(xjmp_buf buf, int value){
	xlongjmp(buf, value);
}

#ifndef XSETJMP_SKIP_SIGNALS
void(xsiglongjmp)(xsigjmp_buf buf, int value){
	xlongjmp(buf, value);
}
#endif
#endif
