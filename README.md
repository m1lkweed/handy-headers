# Handy Headers
GCC is required for all headers
## [exception.h](https://github.com/m1lkweed/handy-headers/blob/main/exception.h)
### `try`/`except` blocks in C
Works with actual signals
```c
int main(){
	int e;
	try{ //brackets optional
		__asm("ud2");
	}except(e){ //e is an optional argument, must be a modifiable lvalue if used
		printf("Caught a %d: %s\n", e, strsignal(e));
	}
}
```
`throw(code)`: throws an exception without raising a signal, so it should be faster than `raise()`. Both will work for custom handlers automatically. `SIGINT` and some other signals are intentionally not caught, though they can be manually handled by adding the following:
```c
void my_handler(int signum){
	if(except_handler.frame) //global
		longjmp(*except_handler.frame, signum); //siglongjmp also works
	exit(1);
}
...
signal(SIGINT, my_handler); //sigaction is preferred
```
Signal handlers are never overwritten unless they are `SIG_DFL`.
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
`hotpatch(target, replacement)` replaces all calls to `target` with calls to `replacement`.

`is_patchable(function)` returns true if `function` can safely be patched.

`is_patched(function)` will return the address of the replacing function if `function` is patched, else NULL.

`original_function(function)` returns a callable pointer to the original code of a patched function.

**Safety**

Unless `ALLOW_UNSAFE_HOTPATCH` is defined, all calls to `hotpatch()` will ensure that `target` is patchable and that `replacement` and `target` have identical signatures.

### Closures:
```c
closeable int close_me(int a){ //required on systems that do not use the SYSV ABI by default
	static int b = 0;
	return b += a;
}

int main(){
	int (*closure)(void) = closure_create(close_me, 1, 5); //increment by 5 each call
	for(int i = 0; i < 10; ++i)
		printf("%d\n", closure());
	closure_destroy(closure); //optional
}
```
Floats and variadics are not supported, closeable functions take advantage of the System V ABI.

`closure_create(function, nargs, data)` creates a closure around `function`, which has `nargs` arguments (max. 6). `data` can be any type but should match the final parameter of `function`.
### Lambdas:
```c
int(*foo)(int) = lambda(int, (int a), {printf("%d\n", a); return 5;});
foo(3); //prints 3
```
`lambda`s are functions that don't have a name. They are well-suited for replacing a patchable function (only with optimization flags) and can be turned into a closure. Note that trying to read or modify non-static local variables from the parent function is UB and will cause the stack to be marked as executable.
###Injection:
```c
// We don't have __usercall yet but this function is essentially
// int double_return_value(int value@<eax>){
// 	return value + value;
// }
[[gnu::naked]] int double_return_value(){
	asm("addl	%%eax, %%eax\n\t"
	    "retq"
	   :
	);
}

// int double_return_value(int value@<eax>){
// 	return value + 3;
// }
[[gnu::naked]] int add_3_to_ret(){
	asm("addl	$3, %%eax\n\t"
	    "retq"
	   :
	);
}

int main(){
	inject(double_return_value);
	inject(add_3_to_ret);
	return 5; // returns 16;
}
```
`inject(function)` hijacks the stack, making the current function return to your newly-injected function before returning to its caller. injected functions are reached in reverse order, so the last-called function is entered first. Mark all functions that call inject with `[[gnu::noinline]]` and compile with optimizations enabled for reliable results.
```c
int main(){
	inject(double_return_value);
	inject(add_3_to_ret);
	// 0 -> 16
	// 1 -> 8
	// 2 -> 5
	// 3+   UB
	bypass_injections(5, 2); // 5
}
```
`bypass_injection(value)` skips the most recent injection, so it is never reached or called. `bypass_injections(value, count)` skip the `count` most recent injections. It is erroneous to cell either function without calling `inject()` first, or to use a `count` larger than the number of calls to `inject`.
