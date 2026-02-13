# C Simple Libs

A collection of simple c libraries.

## Introduction

This library is a collection of header-only libraries for the purpose of adding some of the basic features of
higher level languages without the bloat. In order to do this, I have relied on heavy use of macros. If you are a
c-macro hater, this is likely not the library for you. I realize that they can be problematic, but are somewhat
necessary to achieve advanced features while remaining easy to use, lightweight and without requiring the use of
assembly.

## Portability Disclaimer

While portability is a consideration, it is not my main concern as these libraries are mainly for my personal
use, and are in many cases not suitable for production code. Most libraries require C23, with some relying on GNU
compiler extensions, such as `csl-errval.h` (some of these will have a "compatibility mode" which provides
similar functionality but typically with some trade-offs in either performance, ease-of use, or safety).

Given that several libraries rely on GNU compiler extensions, I recommend either [clang](https://clang.llvm.org/) or [GCC](https://gcc.gnu.org/), as they have both implemented most of these features. 

For use on Windows, your best bet is still to use these options, just be aware that certain niche problems exist
with their use on Windows. I cannot speak to these libraries compatibility with MSVC.

## Contents

- [csl-errval (GNU Extended)](#csl-errval (GNU Extended)) 
- [csl-errval (ISO C23)](#csl-errval ISO C23 Standard) 
- [csl-dstring](#csl-dstring.h)
- [csl-match](#csl-match.h)
- [csl-recursed](#csl-recursed.h)
- [csl-templates](#csl-templates.h)
- [csl-smrtptrs](#csl-smrtptrs.h)

## csl-errval (GNU Extended)

`csl-errval.h` is intended to provide Rust inspired error value wrapping. The basic idea is to use macros (of
course) to wrap the usual return type of the function in a "result" type. This type is a struct which contains an
enum that indicates the error code, and whatever the "base type" specified by the user is. 

There are two "modes" for this library. If the source is compiled in an environment supporting the GNU compiler
extension *statement expressions*, the following macros are exposed. Otherwise see section ...

### `RESULT(T)`

A function-like macro that takes a type (`T`) as a parameter and expands to the mangled type for the result struct.

```c
RESULT(int) // Expands to --> int_result_t
```

This is the type that you will implement any functions that you want to use `csl-errval` error-checking on.
Because this type is actually a struct which contains the base type (`int` in the above example), `T_result_t`
must be derived first. See `DERIVE_RESULT_DIRECT` and  `DERIVE_RESULT_INDIRECT` for more information.

>[!note]
>Because the inputted type is expanded to `T_result_t`, this is the type that your LSP, and compiler will see. Be
>aware of this when debugging.

### `DERIVE_RESULT_DIRECT(T)`

Another function-like macro that takes a type (`T`) as its parameter and expands to the necessary `result`
structure based on the inputted type. 

```c
DERIVE_RESULT_DIRECT(int); // Expands to the definition of the result struct containing the base type
```

>[!note]
>The `DIRECT` part of the name indicates that this is not to be used for types with any level of indirection
>(pointers). The `*` breaks the type mangling, and a separate macro `DERIVE_RESULT_INDIRECT(T)` is needed.

### `DERIVE_RESULT_INDIRECT(T)`

Essentially the same as `DERIVE_RESULT_DIRECT(T)`, except intended for use with pointers The main difference is
you must specify the base type, then give an alias for the base type to prevent breaking the type mangling with
`*`. The alias must be used as the type with `RESULT()`, `ERR()`, and `OK()`.

```c
// Expands the same as `DERIVE_RESULT_DIRECT`, but prevents the * from breaking the type mangling
DERIVE_RESULT_INDIRECT(char*, p_char); 
```

>[!note]
>The alias does not need to match this format, it was left ambiguous because there are many different styles.

### `ERR(T, value)`

This is a function-like macro which expands to a compound literal of the result type for the inputted base type
(`T`). If you do not know about c compound literals, I highly recommend reading up on them because they were a
very nice quality-of-life addition to the language, and are somewhat necessary for use with this library with any complex type. This is what will be used as a return from any function which implements the `RESULT(T)` type in the case of an error.

```c
RESULT(int) always_fails() {
    return ERR(int, 0); // expands to --> (int_result_t){ .code = _ERR, .value = 0}
}
```

>[!note]
>While `value` is the value that will be returned as the base type, due to the control flow within the `UNWRAP`
>macro, the result of the expression is never evaluated meaning it's value is arbitrary in the case of an error
>and is only there to prevent compilation errors.

### `OK(T, value)`

Function-like macro which is equivalent to `ERR(T, value)` except sets the error code to `_OK`. This means that
the value will be evaluated in the resulting expression when the function returns. 
```c
RESULT(int) always_succeeds() {
    // expands to --> (int_result_t){ .code = _OK, .value = 1} --> value is accessible to resulting expression
    return OK(int, 1); 
}
```

### `UNWRAP(func(), handler)`

This is where the magic happens. Without getting into the nitty-gritty of how it works, `UNWRAP` checks the
result of the code contained in the `RESULT(T)` structure that is returned from the wrapped function. If the code
was `_OK`, then the LHS of the expression is evaluated. If not, handler is run, allowing the LHS to be skipped.

```c
// Example adapted from csl-dstring.h
size_t size = UNWRAP(get_size(self), {
        return ERR(size_t, 0);
});
```

As you can see, given that the handler (the second argument) is evaluated when the function errors, the outer
function will return before the LHS can be evaluated.

>[!warning]
>If you do not use a control statement in the handler, the LHS will be evaluated. This is allowed for recoverable
>errors where you still want to know the value returned by the function, but for non recoverable errors, a
>control statement such as:
>- `return`
>- `break`
>- `goto`
>Must be used to prevent the evaluation of garbage values. This will prevent NULL pointer dereferences.

>[!note]
>The `{}` in the second argument (the handler) are optional, but I find it makes the scope of the handler clearer, so I add them

## csl-errval ISO C23 Standard 

This is the version of `csl-errval.h` you will get when compiling in an environment which does not support GNU
statement expressions. Everything is the same except for the `UNWRAP` macro, and the addition of some other
supporting macros. 

### `UNWRAP(func())`

This version of unwrap does not support inline handlers due to the absence of statement expressions. Instead it
saves the state of the stack frame before the execution, and restores it when there is an error. 

>[!warning]
>This has some significant memory concerns due to the stack unwinding, so BE CAREFUL how you use this version. Because of this there is no way to enable this feature if statement expressions are available. If you can use the other version, do not use this one.

Briefly, the concerns with this method are that memory cleanup for any manually managed resources may be skipped
in the stack unwinding. Wherever possible, do not have function-local heap allocated memory, and ensure that ALL
MANUALLY MANAGED MEMORY IS CLEANED UP BEFORE YOU RETURN WITH AN ERROR. Under the hood this version uses `setjmp`
and `longjmp` to handle the stack unwinding. This explanation is intentionally vague because you need to read up
on how these functions work and understand the ramifications of improper usage before using this version. I also
recommend reading the source and understanding the precise flow control that occurs. YOU HAVE BEEN WARNED.

Ominous warnings aside, here is how it is used.

```c
TRY { // Set the jump point
    UNWRAP(always_fails());
    printf("You will never see me!\n");
} CATCH( // Jump to here when code is 1
    fprintf(stderr, "Something broke!\n")
    return -1;
);
```

This version of `UNWRAP` only takes one argument, the wrapped function that implements `RESULT(T)`. If the
function fails, `longjmp` is called and the handler within `CATCH` is run. You may have multiple `UNWRAP`s within
a `TRY` block, and the first one to error will break out and skip any following code. However, due to the safety
concerns with dynamic memory between `TRY` and `CATCH`, it is encouraged that you keep these sections small. As
stated before READ THE DOCS on `setjmp` and `longjmp` before using this feature.

## csl-dstring

Dynamic String implementation.

## csl-match 

Rust-like match expressions.

>[!note]
>All this really is is a convenient wrapper for nested ternary operations. Be advised that there is an idiomatic
>way to achieve this:
>
>```c
>int x = (y == 1) ? 0:
>        (y == 2) ? 10:
>        (y == 3) ? 11:
>        (y == 4) ? 20:
>        /*Default*/30;
>```

The wrapper `MATCH` is used as follows:

```c
int x = 3;
char* result = MATCH(x, "Default",
    (1, "First"), 
    (2, "Second"), 
    (3, "Third"),
    (4, "Last")
);
```

The first argument is the value to match against, the second is the default option. I know that in most
languages (as with the c `switch` statement), default is the last one, and is often optional, but due to
limitations in the macro expansion, it was simplest to make it the first argument. Personally, I think this is a
good thing because it forces the programmer to think about having a safe default first, which is necessary in an
expression.

The last argument is a list of lists in a sense. This uses unfamiliar syntax, as they are surrounded by brackets.
This is because they become the parameter list of a function-like macro. I highly recommend reading the docs on
`csl-recursed.h` and even the source because this is a very interesting corner of c.

Each item in the list must be a two item list itself surrounded in `()`. The first item of each list entry is compared
to the first argument one by one until there is a match, and the second argument of each list entry is the result of the match expression if the left side matches the first argument.

Either item in each list entry can be a function, as long as the return type matches the type of the expression
as a whole.

```c
// One 
int x = MATCH(true, 30,
    (returns_true(), 10),
    (returns_false(),  20)
);

// Or both
MATCH(true, false,
    (returns_false(), printf("False\n")),
    (returns_true(),  printf("True\n"))
);
```

>[!note]
>In situations where you want to run a function but either it does not return a value or you simply do not care
>about the return, you may use `MATCH` as a statement instead of an expression, as seen in the example above.

If you wish to have multiple lines of code running as the result of a `MATCH` expression, I recommend using a
switch case instead, as you cannot use a MATCH as an expression in the context. However, if this is truly what
you want to do, this can be achieved with a statement expression

```c
MATCH(true, printf("Default\n"),
    (returns_false(), printf("False\n")),
    (returns_true(), ({ 
                       printf("True\n");
                       printf("Second Line\n");
                    })),
    (returns_false(), printf("This is never reached\n"))
);
/* stdout:
True
Second Line

*/
```

### csl-recursed

Provides a macro iterator for performing any macro on a list of arguments. This uses recursion and is evaluated
by the preprocessor (worry not fellow recursion haters). Heavy use of macro magic to achieve this. `FOREACH(macro, ...)`. This is used by a number of other libraries included here. 

- `csl-match.h`
- `csl-templates.h`

```c
#define PRINT_THIS(x) printf("%d\n", x);
FOR_EACH(PRINT_THIS, 1, 2, 3, 4, 5, 6);

/* Expands to...
printf("%d\n", 1);
printf("%d\n", 2);
printf("%d\n", 3);
printf("%d\n", 4);
printf("%d\n", 5);
printf("%d\n", 6);
*/
```

This is a fairly contrived example, but you can see how it may be useful for reducing boilerplate repetitions of
something. For more advanced usage, a few other macros can be used to allow for multiple arguments to the macro
that is being repeated. 

To do this, you need to do some annoying boilerplate. This is so the macro gets expanded with parentheses properly for the next preprocessor expansion pass, and to force the next pass to happen. Here is an example from my in-progress argument parsing module.

```c
#define _ARGPARSE_ADD_FLAG(name, flag) name = flag,
#define ARGPARSE_ADD_FLAG() _ARGPARSE_ADD_FLAG
#define ARGPARSE_GET_FLAG(arg) ARGPARSE_ADD_FLAG PARENS arg
/* Constructs the flags enum to store all the boolean options */
#define ARGPARSE_FLAGS(...)                         \
    typedef enum {                                  \
        FOR_EACH(ARGPARSE_GET_FLAG, __VA_ARGS__)    \
    } argparse_flags;                               \
```

The user then calls `ARGPARSE_FLAGS()` like so.

```c
ARGPARSE_FLAGS(
    (SILENT, s),
    (VERBOSE, v),
    /* ... */
)
```

If you need to get more precise control over the recursion, use the `FOR_EACH` definition as an example. `DEFER`
is the secret sauce to getting recursive expansion passes.

>[!note]
>The max recursion depth of `FOR_EACH` is 256. If you need more (which I certainly hope you don't) you can patch
>the source in the `csl-recursed.h` module (specifically the nested `DEFER` definitions). I am choosing to not have a builtin way to do this because recursive
>macros are cursed enough and 256 expansion passes is already kind-of ridiculous.

>[!warning]
>I am not an expert on compilers or the c preprocessor, but I imagine this can cause your compile times to explode
>with heavy usage.

>[!note]
>I am not the inventor of this preprocessor abuse, numerous people have written about this topic, but special
>thanks to:
> - David Maziéres (he has a great [ blog post ](https://www.scs.stanford.edu/~dm/blog/va-opt.html) about this and gives a non-contrived example of usage in C++)
> - Jonathan Heathcote (another good [ blog post ](http://jhnet.co.uk/articles/cpp_magic) about this topic)
>
>Both of these examples use C++, but the principles are the same.

## csl-templates

This module allows for function template generation, and type inference on generated functions.

Here is an example of when you might want a template function.

```c
int returns_int(int arg) {
    return ++arg;
}
float returns_float(float arg) {
    return ++arg;
}
int main() {
    printf("%d", returns_int(10));          // 11
    printf("%f", returns_float(10.0));      // 11.00000
    return 0;
}
```

Since these functions are nearly identical, differing only in type, a template would be useful to avoid
maintaining two almost identical functions. Here is an example template for this case.

```c
#define TEMPLATE(T)                 \
T returns_##T(T arg) {              \
   return ++arg;                    \
}
GENERATE(int, float);
#undef TEMPLATE // Necessary if you want to have multiple templated functions

int main() {
    // These are generated functions
    printf("%d", returns_int(10));          // 11
    printf("%f", returns_float(10.0));      // 11.00000
    return 0;
}
```

The T variable in the `TEMPLATE` macro is substituted for each type in the type list passed to `GENERATE`, which
calls `TEMPLATE` for each item in the list. The result is as follows.

```c
int returns_int(int arg) {
    return ++arg;
}
float returns_float(float arg) {
    return ++arg;
}
```

Identical to our first version!

However, we still need to remember the names of each of the generated functions. Wouldn't it be nice if we could
have one function that does both? We can, using a function-like macro (I did say it would come up a lot). 

If we update our current example to use type inference we get this:

```c
#define TEMPLATE(T)                 \
T my_function_##T(T arg) {          \
   return ++arg;                    \
}
#define my_function(T) INFER(my_function, T, int, float) // This is where the magic happens
GENERATE(int, float);
#undef TEMPLATE // Necessary if you want to have multiple templated functions

int main() {
    // These are generated functions with type inference
    printf("%d", my_function(10));      // 11
    printf("%f", my_function(10.0));    // 11.00000
    return 0;
}
```

The type inference line:

```c
#define my_function(T) INFER(my_function, T, int, float)
```

I should mention, this is not *where* the type inference actually happens. It happens whenever you call
`my_function`. This is because we have defined a function-like macro which wraps all of the generated functions
under one name. This is thanks to the `_Generic` macro. I also recommend reading up on this feature if you don't
know about it.

The important thing to know is that we must supply the name of the function (which should match the name of the
function-like macro we are defining), and pass the parameter of the macro to the second parameter of `INFER`.
Then we pass the type list that we used in `GENERATE`. Now when we call `my_function` the c preprocessor
replaces it with the correctly typed version.

And that is how you get templating and type inference in plain-old-c. Do not be afraid of macros! 

### Caveats

The type inference implementation breaks down with multiple arguments. This will be fixed later on, likely
requiring a second `INFER` macro specifically for this.

#### Multi-type templating

This is possible using another macro: `MULTIGEN` as follows:

```c
#define TEMPLATE(T, U)                 \
U function_##T##_to_##U(T my_variable) {     \
   my_variable += 1;                \
   return my_variable;              \
}
MULTIGEN((int , float), (int, double));
#undef TEMPLATE

int main() {
    printf("Integer: %f\n", function_int_to_float(10));
    printf("Float: %f\n", function_int_to_double(10));
    return 0;
}
```

As you can see above, the example template function above generates two functions. 
- One that accepts an integer and returns a float
- One that accepts an integer and returns a double

>[!warning]
>`MULTIGEN` is incompatible with `INFER`. With the current implementation you cannot use type inference on
>templates with multiple templated types.

>[!note]
>You may also have arguments that are not templated, but this is also incompatible with `INFER` at the present.
>
>```c
>#define TEMPLATE(T)                                 \
>T function_##T(T my_variable, char unused) {        \
>   my_variable += 1;                                \
>   return my_variable;                              \
>}
>GENERATE(int, double);
>#undef TEMPLATE
>
>int main() {
>    char c = 'c';
>    printf("Integer: %f\n", function_int_to_float(10));
>    printf("Double: %f\n", function_int_to_double(10));
>    printf("Integer: %d\n", function_int(10, c));
>    printf("Double: %f\n", function_double(10, c));
>    return 0;
>}
>```

## csl-smrtptrs.h

This module provides `C++`-like smart pointers to C! There are multiple pointer types.

1. `smrtptr_unique(T)`
2. `smrtptr_strong(T)`
3. `smrtptr_strong_atomic(T)`

If you are familiar with `C++` smart pointers these names should be familiar. They function very similarly, except this being C there are a few more chances for undefined behaviour, and they have slightly different names (`smrtptr_strong_atomic` is equivalent to `C++` shared pointers, `smrtptr_strong` is the same but without atomic reference counting).

Caveat: This module depends on a GNU specific attribute (`__attribute((cleanup(<function>)))`). This means that only clang and gcc compilers are supported. Without this feature, the cleanup functions will not run when a pointer goes out of scope. 

## Usage

The basic pattern for this module is this:
1. Forward declare the `SMRTPTR_IMPLEMENTATION` macro
2. Include `"csl-smrtptrs.h"`
3. Create a pointer of a given smart pointer templated type using the base type you want to point to.
4. (If using a shared pointer make copies)
5. The cleanup function will run when the pointer goes out of scope, and free the memory (if certain conditions are met)

### `smrtptr_unique(T)`

Unique pointers are used for when the data is not being shared, and only needs to exist within one scope; exactly one object owns the data. 

```c
{ /* This scope defines the lifetime */
    smrtptr_unique(int) ptr1 = smrtptr_make_unique(int, malloc(sizeof(int)), free);
    deref_smrtptr(ptr1) = 1;
    printf("ptr1: %d\n", *ptr1.ptr);
} /* cleanup function is called here and frees the memory */
```

In the example above `smrtptr_unique(int)` expands to:

```c
__attribute__((cleanup(smrtptr_free_unique))) int_smrtptr_unique
```

This cleanup function must be defined before the unique pointer can be used. To do this you must define an "X-macro" of the name "`SMRTPTR_UNIQUE_TYPE_LIST`".

```c
#define SMRTPTR_UNIQUE_TYPE_LIST \
    SMRTPTR_DERIVE_UNIQUE(int) \
    SMRTPTR_DERIVE_UNIQUE(FILE)
```

This will derive the struct definition which will contain the given type. Now we can use unique pointers to integers and file handles.

All smart pointers are compound types. That means that
you cannot directly assign them to a pointer. To
access the pointer use the field `ptr`, or one of the
helper macros

```c
smrtptr_unique(FILE) fp = smrtptr_make_unique(FILE, fopen("test.txt", "a"), close_file);

*fp.ptr = 10;
// Or
deref_smrtptr(fp);

fputs("Test", fp.ptr);
// Or
fputs("Test", ref_smrtptr(fp));
```

>[!note]
>the "ref" and "deref" macros expand to the exact same
>thing so don't use them if you prefer. If you use
>them you won't touch the fields you shouldn't.

## Unique Pointer Allocation

All pointer types follow a similar pattern. To use a smart pointer you must have a pointer to some allocated data which requires manual cleanup (not on the stack), and a cleanup function pointer which is responsible for freeing the allocated data at the end of the scope. This function pointer must be of the type `void (*)(void*)`; `free` for example. 

The pointer to the allocated data is void and therefore can accept any type.

```c
smrtptr_unique(int) ptr = smrtptr_make_unique(int,
malloc(sizeof(int)), free);
```

Cleanup functions which do not have this type must be wrapped in a usable function. Here is an example for file handles (`FILE*`).

```c
void close_file(void* void_fp) {
    FILE* fp = void_fp;
    if(fclose(fp) == EOF) {
        fprintf(stderr, "ERROR: Failed to close file\n");
        exit(EOF);
    }
}
```

Now that we have a wrapping cleanup function we can
use a unique pointer with `FILE*`.

```c
{
    // File IO
    smrtptr_unique(FILE) fp = smrtptr_make_unique(FILE, fopen("test.txt", "a"), close_file);
    if(smrtptr_errno) return smrtptr_errno;
    if(fp.ptr == NULL) {
        fprintf(stderr, "ERROR: Failed to open file\n");
        exit(-1);
    } else {
        printf("File successfully opened!\n");
        if(fputs("Hello there!\n", fp.ptr) == 0) {
            fprintf(stderr, "ERROR: Failed to write to file\n");
            return -1;
        };
        printf("File successfully written to!\n");
    }
}
/* smrtptr_free_unique() is called which in turn
calls the cleanup function pointer for the given type */
```

Assuming the file could be opened, this will write a new line to the file which says "Hello there!". `fclose` is implicitly called via `close_file` using the cleanup attribute when each scope ends.

### Foot-guns

1. Only one unique pointer to the same data can exist. Failure to follow this rule will result in a double free. If you need multiple references, used a strong pointer.
2. Because this is `C` there is nothing to stop you from simply handing out raw pointers to the same data. This is even sometimes the right choice because of `C`'s limitations. Generally, this should only be done when passing by reference to a function that DOES NOT TAKE OWNERSHIP. Do not manually free unless you choose to disable automatic cleanup, which I will explain later.

## `smrtptr_strong(T)`

Strong pointers are for when the ownership of the data must be shared. Unique pointers are not suitable for this because the data is freed as soon as the first `smrtptr_unique(T)` goes out of scope, leading to use-after-frees. This is undefined behaviour. In `C++` an attempt to copy a `unique_ptr` results in a compiler error. That is not the case here because `C` does not have copy constructors or operator overloading. It is therefore up to the programmer to ensure they do not attempt to copy a unique pointer, or assign it directly to a raw pointer.

Strong pointers are like unique pointers, except the "make" function also allocates a control block which contains reference counts and the cleanup function pointer.

```c
typedef struct {
    T *ptr;
    shared_ptr_ctrlblk *ctrl;
} T##_smrtptr_strong;
```

The basics of how `smrtptr_strong` works is as follows:
- Creation of a pointer is handled with `smrtptr_make_strong` (this initializes the reference counts and allocates the control block).
- Cloning of a pointer is handled with `smrtptr_copy_strong` (this increments the reference count for the corresponding reference type).
- The freeing function is called at the end of each scope and only frees the data once the shared reference count goes to 0.
    - The control block is freed once the weak reference count is also 0.

>[!warning]
>`shared_ptr_ctrlblk` does not use atomic reference counting and is therefor not thread safe. For thread safety you must use `smrtptr_strong_atomic`

```c
/* Allocate space for an integer on the heap and store it in a smrtptr_strong to an integer */
smrtptr_strong(int) ptr2 = smrtptr_make_strong(int, malloc(sizeof(int)), free);
```

This is not the same as copying. With copying the smart pointer has already been allocated. Attempting to copy a raw pointer is invalid.

### `smrtptr_copy_strong(ptr, PTR_TYPE)`

`smrtptr_copy_strong` takes 3 arguments:
1. The base type that the strong pointer will contain (must match the base type of the original)
1. The `smrtptr` you wish to copy.
2. The type of that pointer. (Valid values are `SHARED_PTR` and `WEAK_PTR`). 

```c
smrtptr_strong(int) ptr_copy = smrtptr_copy_strong(int, ptr, SMRTPTR_STRONG);
```

In the above example, `ptr` is a shared pointer, and we are creating another shared pointer, `ptr_copy`, which references the same data and shares ownership with `ptr`. Thus, if one goes out of scope, it will free the data only if it is the only remaining owning reference (`smrtptr_strong`) to that data.

However, sometimes we do not want to have our reference take ownership of the data such as to prevent cyclical references which prevent the data from being freed. In this case, a `smrtptr_weak` must be used.

`smrtptr_copy_strong` can also create a `weak_ptr` to the data, if passed `WEAK_PTR` as the pointer type. 

```c
smrtptr_weak(int) ptr_copy = smrtptr_copy_strong(ptr, WEAK_PTR);
```

In this case, `ptr` is still a strong pointer, so a weak (non-owning) reference to the same data is created and stored in `ptr_copy`, and `ptr` still has ownership of the data. Since `ptr_copy` is a non-owning reference, if we want to access the data, we should first check if it still exists. DO NOT USE THIS FOR ATOMIC VERSIONS. There is a separate function.

```c
if(is_ptr_dead(ptr3)) {
    printf("ptr3: %d\n", deref_smrtptr(ptr3)); /* Reading */
}
```

Therefore, we do not try to access the data unless it is still valid. Attempting to access the data stored in a weak pointer without this check could result in use after free.

## `smrtptr_strong_atomic(T)`

Atomic strong pointers are for the same purposes as regular strong pointers but using atomic reference counting,
meaning thread safety. 

>[!warning]
>You are still responsible for ensuring thread safety for the pointed-to data by means of a mutex for example.
>Atomic reference counting only ensures that reference counts are synchronized between threads.

Usage of strong pointers is exactly the same, just know that there is atomic reference counting under the hood. 

>[!note]
>You may be wondering "why not just always use the thread-safe version?". You can, there is just a performance
>cost. This is the downside of `C++` shared pointers, and why `Rust` gives the option (`Arc` vs `Rc`).

## `smrtptr_weak_atomic(T)`

This is where the atomic implementation differs from the non-atomic one. We cannot simply check if the pointer is
dead and *then* perform operations on the data. The weak pointer does not own the data, so another thread may free
it at any point, including right after we check if it is still alive. Instead, we must conditionally promote the
weak pointer to a shared pointer if the data is still alive. This is done via `smrtptr_lock_weak_atomic`.

It is a macro which takes the following arguments:

1. `T`: The type of the shared/weak pointers
2. `strong`: The strong pointer to copy the weak pointer into
3. `weak`: The weak pointer to conditionally promote

And it looks something like this:

```c
{ 
    smrtptr_strong_atomic(int) ptr11;
    if( smrtptr_lock_weak_atomic(int, ptr11, ptr9) ) {
        printf("ptr11: %d\n", deref_smrtptr(ptr11));
    }
}
```

The outer scope here is necessary because we want to constrain the scope of the promoted strong pointer to where
we know it is valid. This is an ugly and unfortunate limitation of `C` syntax.

What happens under the hood is `smrtptr_lock_weak_atomic` checks if the weak pointer is still alive and if so
increments the strong count, synchronised between threads thanks to atomic operations. You can then use the strong
pointer within that scope safely knowing that the lifetime of the data is guaranteed to be at least as long as the
current scope.

Failing to lock the weak pointer first will result in use-after-frees and other annoying issues.
