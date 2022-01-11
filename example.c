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

_Noreturn void my_handler(int signum){
	if(except_handler.frame)
		siglongjmp(*except_handler.frame, signum);
	exit(1);
}

int main(){
	foo();
	hotpatch(foo, bar);
	foo();
	hotpatch(foo, NULL);
	foo();
	int e;
	signal(SIGTERM, my_handler);
	try{
		raise(SIGTERM);
	}except(e){
		printf("Caught a %d: %s\n", e, strsignal(e));
	}

	int (*closure)(void) = closure_create(close_me, 1, 1);
	for(int i = 0; i < 10; ++i)
		printf("%d\n", closure());
}
