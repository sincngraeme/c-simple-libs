#include "../csl-argparse.h"
#include "../csl-templates.h"

int main(int argc, char** argv) {
    ARGPARSE_PARSE(
        /* Positional Params */
        (PROGRAM, FILE_NAME, OUTPUT_FILE),
        /* Flags */
        (
            (SAY_HI, 's'),
            (SAY_HELLO, 'S'),
            (NERD_OUT, 'n')
        ),
        /* Optional Arguments */
        (
            (CUSTOM_MESSAGE, "custom-message"),   
            (CUSTOM_NUMBER, "custom-number"),   
        )
    );
    for(int i = 0; i < argparse.npositionals; i++) {
        if(argparse.positionals[i] != NULL) printf("Positional %s\n", argparse.positionals[i]);
    }
    for(int i = 0; i < argparse.noptargs; i++) {
        if(argparse.optargs[i] != NULL) printf("Positional %s\n", argparse.optargs[i][2]);
    }
    for(int i = 0; i < argparse.nflags; i++) {
        if(argparse.flags[i] != '\0') printf("Positional %c\n", argparse.flags[i]);
    }
    return 0;

}
