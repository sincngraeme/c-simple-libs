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

## `csl-errval.h `ISO C23 Standard 

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
