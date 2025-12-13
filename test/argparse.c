#include <stdio.h>
#include "../csl-argparse.h"
#include "../csl-recursed.h"
#include "../csl-match.h"


int main() {
   
    int x = 5;
    printf("Result: %s\n", (
            (x == 1) ? "First":
            (x == 2) ? "Second":
            (x == 3) ? "Third":
            "Last" 
        )
    );
#define TEST(arg) printf("%s\n", #arg);
    int thing1 = 0, thing2 = 0, thing3 = 0;
    FOR_EACH(TEST, thing1, thing2, thing3);
    printf("%d%d%d\n", thing1, thing2, thing3);
    printf("%lu\n", __STDC_VERSION__);
    x = 3;
    char* result = MATCH(x,
        (1, "First"), 
        (2, "Second"), 
        (3, "Third"),
        (x, "Last")
    );
    MATCH(x,
        (1, printf("First\n")),
        (2, printf("Second\n")),
        (3, printf("Third\n")),
        (4, printf("Fourth\n")),
        (x, printf("Default\n"))
    );
    printf("Result: %s\n", result);
    return 0;
}
