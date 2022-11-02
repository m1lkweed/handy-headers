#include <stdio.h>
#define FUNCTIONS_IMPLEMENTATION
#define EXCEPTION_IMPLEMENTATION
#include "functions.h"
#include "exception.h"

patchable void foo(void){
	puts("in foo");
}

void bar(void){
	puts("in bar");
}

closeable int close_me(int a){
	static int b = 0;
	return b += a;
}

[[noreturn]] void my_handler(int signum){
	if(except_handler.frame)
		siglongjmp(*except_handler.frame, signum);
	exit(1);
}

int tramp_foo(int a){
	printf("main() returned %d\n", a);
	return 0;
}

int foo(){
	return injection_trampoline(tramp_foo);
}

int main(){
	foo();
	if(is_patchable(foo))
		hotpatch(foo, bar);
	foo();
	original_function(foo)();
	if(is_patched(foo))
		hotpatch(foo, NULL);
	foo();
	int e;
	signal(SIGTERM, my_handler);
	try{
		raise(SIGTERM);
	}except(e){
		printf("Caught a %d: %s\n", e, strsignal(e));
	}

	int (*closure)(void) = closure_create(close_me, 1, 5); //increment by 5 each call
	for(int i = 0; i < 10; ++i)
		printf("%d\n", closure());
	inject(foo);
	return 5;
}
