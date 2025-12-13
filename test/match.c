#include <stdio.h>
#include <stdbool.h>
#include "../csl-argparse.h"
#include "../csl-recursed.h"
#include "../csl-match.h"


bool returns_true() {
    return true;
}
bool returns_false() {
    return false;
}

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
    char* result = MATCH(x, "Default",
        (1, "First"), 
        (2, "Second"), 
        (3, "Third"),
        (4, "Last")
    );
    printf("Result: %s\n", result);
    MATCH(x, printf("Default\n"),
        (1, printf("First\n")),
        (2, printf("Second\n")),
        (3, printf("Third\n")),
        (4, printf("Fourth\n")),
        (5, printf("Fourth\n")),
        (6, printf("Fourth\n")),
        (7, printf("Fourth\n")),
        (7, printf("Fourth\n")),
        (5, printf("Fifth\n"))
    );
    MATCH(true, printf("Defualt"),
        (false, printf("First\n")),
        (false, printf("Second\n")),
        (false,  printf("Third\n")),
        (true, printf("Fourth\n"))
    );
    MATCH(true, false,
        (returns_false(), printf("False\n")),
        (returns_true(),  printf("True\n"))
    );
    x = MATCH(true, false,
        (returns_false(), 10),
        (returns_true(),  20)
    );
    printf("x: %d\n", x);
    x = MATCH(true, 30,
        (returns_false(), 10),
        (returns_false(),  20)
    );
    printf("x: %d\n", x);
    return 0;
}
