/*******************************************************************************
* Name:             csl-argparse.h                                             *
* Description:      Cursed macro magic to recursively parse VA lists           *
* By:               Nigel Sinclair                                             *
* Email:            sincngraeme@gmail.com                                      *
* Github:           https://github.com/sincngraeme/                            *
*******************************************************************************/

// For some reason this needs expansion
#define PARENS ()

// Defines the recursion depth of the macro expansion
#define DEFER(...)   DEFER14(__VA_ARGS__)
#define DEFER14(...) DEFER13(__VA_ARGS__)
#define DEFER13(...) DEFER12(__VA_ARGS__)
#define DEFER12(...) DEFER11(__VA_ARGS__)
#define DEFER11(...) DEFER10(__VA_ARGS__)
#define DEFER10(...) DEFER9(__VA_ARGS__)
#define DEFER9(...)  DEFER8(__VA_ARGS__)
#define DEFER8(...)  DEFER7(__VA_ARGS__)
#define DEFER7(...)  DEFER6(__VA_ARGS__)
#define DEFER6(...)  DEFER5(__VA_ARGS__)
#define DEFER5(...)  DEFER4(__VA_ARGS__)
#define DEFER4(...)  DEFER3(__VA_ARGS__)
#define DEFER3(...)  DEFER2(__VA_ARGS__)
#define DEFER2(...)  DEFER1(__VA_ARGS__)
#define DEFER1(...)  __VA_ARGS__

/* This is the top level one that will acually be called, note that defer passes
 * through the full "stack" of nested macros */
#define FOR_EACH(macro, ...) __VA_OPT__(DEFER(_FOR_EACH(macro, __VA_ARGS__)))
/* This macro removes the first argument of the VA list repeatedly and calls
 * the macro on it */
#define _FOR_EACH(macro, first, ...)                    \
    macro(first)                                        \
    __VA_OPT__(_FOR_EACH_ PARENS (macro, __VA_ARGS__))
/* Another necessary abstraction to prevent the prepocessor from doing the safe thing */
#define _FOR_EACH_() _FOR_EACH
