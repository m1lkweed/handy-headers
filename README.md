# Handy Headers
GCC/clang is required for all headers
## [exception.h](https://github.com/m1lkweed/handy-headers/blob/main/exception.h)
### `try`/`except` blocks in C
Works with actual signals
```c
int main(){
	int e;
	try{ //brackets optional
		__asm("ud2");
	}except(e){ //e must be a modifiable lvalue
		printf("Caught a %d: %s\n", e, strsignal(e));
	}
}
```
`throw()`: prevents values from killing the program (such as `SIGKILL`), though `raise()` works for most cases. Both will work for custom handlers automatically. `SIGINT` and some other signals are intentionally not caught, though they can be manually handled by adding the following:
```c
void my_handler(int signum){
	if(except_handler.frame) //global
		siglongjmp(*except_handler.frame, signum);
	exit(1);
}
...
signal(SIGINT, my_handler); //sigaction is preferred
```
Signal handlers are never overwritten unless they are `SIG_DFL`
## [functions.h](https://github.com/m1lkweed/handy-headers/blob/main/functions.h)
### Hotpatching:
Redefining functions at runtime
```c
patchable void foo(void){ //patchable is required
	puts("foo");
}

void bar(void){
	puts("bar");
}

int main(){
	foo(); //prints foo
	hotpatch(foo, bar);
	foo(); //prints bar
	hotpatch(foo, NULL);
	foo(); //prints foo, NULL resets the patch
}
```
### Closures:
```c
closeable int close_me(int a){ //closeable is required on systems that do not use the SYSV ABI by default
	static int b = 0;
	return b += a;
}

int main(){
	int (*closure)(void) = closure_create(close_me, 1, 1);
	for(int i = 0; i < 10; ++i)
		printf("%d\n", closure());
	closure_destroy(closure); //optional
}
```
Floats and variadics are supported, closeable functions take advantage of the System V ABI.

`closure_create(function, nargs, data)` creates a closure around `function`, which has `nargs` arguments. `data` can be any type but should be the final parameter of `function`.
### Lambdas:
```c
int(*foo)(int) = lambda(int, (int a), {printf("%d\n", a); return 5;});
foo(3); //prints 3
```
`lambda`s work well with both closures and hotpatches
