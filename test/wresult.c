#include <stdio.h>
#include "../csl-errval.h"

DERIVE_WRESULT(int, BAD_THING1, BAD_THING2);
DERIVE_WRESULT(char);

WRESULT(int) fails() {
    return WRESULT_ERR(int, 0);
}

WRESULT(int) fails_with_code() {
    return WRESULT_ERR_CODED(int, 0, BAD_THING1);
}

WRESULT(int) succeeds() {
    return WRESULT_OK(int, 10);
}

int main() {
    printf("PASS: %d\n", UNWRAP( succeeds(), { 
        return 1; 
    }));
    printf("FAIL: %d\n", UNWRAP( fails_with_code(), { 
        fprintf(stderr, "Function fails_with_code returned error %d\n", wresult.code);
    }));
    printf("FAIL: %d\n", UNWRAP( fails(), { 
        return 1; 
    }));
}
