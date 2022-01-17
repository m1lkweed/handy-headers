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
	sig_atomic_t exception;
};

_Thread_local struct exception_frame except_handler = {0};

void _$except_init$(void);
static inline void throw(int id);

#define try ({                                                                           \
	_Thread_local static sigjmp_buf *_$old_exception_frame$, _$new_exception_frame$; \
	_$old_exception_frame$ = except_handler.frame;                                   \
	except_handler.frame = &_$new_exception_frame$;                                  \
	except_handler.exception = 0;                                                    \
	_$except_init$();                                                                \
	if((except_handler.exception = sigsetjmp(_$new_exception_frame$, 0)) == 0){      \
		switch(0)default:case 0: /*lets us call break*/

#define _$EXCEPT_EMPTY$_HELPER(...) = except_handler.exception, ## __VA_ARGS__
#define _$EXCEPT_EMPTY$(default, ...) default _$EXCEPT_EMPTY$_HELPER(__VA_ARGS__)

#define except(e)                                                               \
	except_handler.exception = 0;                                           \
        }else{                                                                  \
		[[maybe_unused]] int _$except_dummy$;                           \
		_$EXCEPT_EMPTY$(_$except_dummy$, e) = except_handler.exception; \
        }                                                                       \
        except_handler.frame = _$old_exception_frame$;                          \
	});                                                                     \
if(except_handler.exception != 0)

#ifdef EXCEPTION_IMPLEMENTATION
#include <stdlib.h> // exit()

_Noreturn void _$exception_handler$(int signum){
	if(except_handler.frame)
		siglongjmp(*except_handler.frame, signum);
	else
		exit(128 + signum);
}

__attribute__((constructor)) void _$except_init$(void){
	struct sigaction old_action, new_action = {
		.sa_handler = _$exception_handler$,
		.sa_flags = SA_NODEFER
	};
	sigaction(SIGILL,  NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGILL,  &new_action, NULL);
	sigaction(SIGABRT, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGABRT, &new_action, NULL);
	sigaction(SIGFPE,  NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGFPE,  &new_action, NULL);
	sigaction(SIGSEGV, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGSEGV, &new_action, NULL);
#ifdef SIGBUS
	sigaction(SIGBUS, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGBUS, &new_action, NULL);
#endif
#ifdef SIGPIPE
	sigaction(SIGPIPE, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGPIPE, &new_action, NULL);
#endif
#ifdef SIGURG
	sigaction(SIGURG, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGURG, &new_action, NULL);
#endif
#ifdef SIGTRAP
	sigaction(SIGTRAP, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGTRAP, &new_action, NULL);
#endif
#ifdef SIGEMT
	sigaction(SIGEMT, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGEMT, &new_action, NULL);
#endif
#ifdef SIGSYS
	sigaction(SIGSYS, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGSYS, &new_action, NULL);
#endif
#ifdef SIGLOST
	sigaction(SIGLOST, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGLOST, &new_action, NULL);
#endif
#ifdef SIGXCPU
	sigaction(SIGXCPU, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGXCPU, &new_action, NULL);
#endif
#ifdef SIGXFSZ
	sigaction(SIGXFSZ, NULL, &old_action);
	if(old_action.sa_handler == SIG_DFL)
		sigaction(SIGXFSZ, &new_action, NULL);
#endif
}

inline void throw(int id){
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
