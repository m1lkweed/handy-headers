// Adds try/throw/except for catching signals
// Original code (c)Patricio Bonsembiante (pbonsembiante@gmail.com)
// Released under the GPLv3+
// Updates (c)m1lkweed 2022 GPLv3+
#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include <setjmp.h>
#include <signal.h>

struct exception_frame {
	sigjmp_buf *frame;
	volatile sig_atomic_t exception;
};

extern _Thread_local char _$exception_stack$[SIGSTKSZ];
extern _Thread_local struct exception_frame except_handler;

void _$except_init$(void);
void throw(int id);

#define try do{                                                                     \
	sigjmp_buf *_$old_exception_frame$, _$new_exception_frame$;                 \
	volatile int _$except_dummy$ = -1;                                          \
	typedef int _$except_no_gotos$[_$except_dummy$];                            \
	_$old_exception_frame$ = except_handler.frame;                              \
	except_handler.frame = &_$new_exception_frame$;                             \
	except_handler.exception = 0;                                               \
	_$except_init$();                                                           \
	if((except_handler.exception = sigsetjmp(_$new_exception_frame$, 0)) == 0){ \
		for(; _$except_dummy$; ++_$except_dummy$) /*lets us call break safely*/

#define _$EXCEPT_EMPTY$_HELPER(...) = except_handler.exception, ## __VA_ARGS__
#define _$EXCEPT_EMPTY$(default, ...) default _$EXCEPT_EMPTY$_HELPER(__VA_ARGS__)

#define except(e)                                                               \
		except_handler.exception = 0;                                   \
	}else{                                                                  \
		_$EXCEPT_EMPTY$(_$except_dummy$, e) = except_handler.exception; \
	}                                                                       \
	except_handler.frame = _$old_exception_frame$;                          \
}while(0);if(except_handler.exception == 0){}else for(struct{_Bool a;int        \
	_$except_no_gotos$[({int n = -1;n;})];} _$inc$ = {};_$inc$.a;++_$inc$.a)

#ifdef EXCEPTION_IMPLEMENTATION

#include <stddef.h>

_Thread_local char _$exception_stack$[SIGSTKSZ];
_Thread_local struct exception_frame except_handler = {0};

_Noreturn void _$exception_handler$(int signum){
	if(except_handler.frame){
		siglongjmp(*except_handler.frame, signum);
	}else{
		//Set handler to SIG_DFL and reraise signal, in case
		//the system wants to print a message or core dump
		struct sigaction old_action, new_action = {
			.sa_handler = SIG_DFL,
			.sa_flags = SA_NODEFER
		};
		sigaction(signum,  NULL, &old_action);
		sigaction(signum,  &new_action, NULL);
		raise(signum);
		sigaction(signum,  &old_action, NULL);
		_Exit(128 + signum);
	}
}

[[gnu::constructor]] void _$except_init$(void){
	stack_t _$exception_stack_wrapper$ = {
		.ss_size = SIGSTKSZ,
		.ss_sp = _$exception_stack$,
		.ss_flags = 0
	};

	struct sigaction old_action, new_action = {
		.sa_handler = _$exception_handler$,
		.sa_flags = SA_NODEFER | SA_ONSTACK
	};

	sigaltstack(&_$exception_stack_wrapper$, NULL);
	sigaction(SIGILL,  NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGILL,  &new_action, NULL);
#ifdef SIGTRAP
	sigaction(SIGTRAP, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGTRAP, &new_action, NULL);
#endif
	sigaction(SIGABRT, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGABRT, &new_action, NULL);
#ifdef SIGBUS
	sigaction(SIGBUS, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGBUS, &new_action, NULL);
#endif
	sigaction(SIGFPE,  NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGFPE,  &new_action, NULL);
	sigaction(SIGSEGV, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGSEGV, &new_action, NULL);
#ifdef SIGPIPE
	sigaction(SIGPIPE, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGPIPE, &new_action, NULL);
#endif
#ifdef SIGSTKFLT
	sigaction(SIGSTKFLT, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGSTKFLT, &new_action, NULL);
#endif
#ifdef SIGURG
	sigaction(SIGURG, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGURG, &new_action, NULL);
#endif
#ifdef SIGXFSZ
	sigaction(SIGXFSZ, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGXFSZ, &new_action, NULL);
#endif
#ifdef SIGSYS
	sigaction(SIGSYS, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGSYS, &new_action, NULL);
#endif
#ifdef SIGEMT
	sigaction(SIGEMT, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGEMT, &new_action, NULL);
#endif
#ifdef SIGLOST
	sigaction(SIGLOST, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGLOST, &new_action, NULL);
#endif
}

[[maybe_unused]] void throw(int id){
	if(id != 0){
		except_handler.exception = id;
		if(except_handler.frame)
			siglongjmp(*except_handler.frame, id);
		_$exception_handler$(id);
	}
}

#endif //EXCEPTION_IMPLEMENTATION
#endif //_EXCEPTION_H_
/*--------------------------END OF TRY / EXCEPT CODE-------------------------*/
