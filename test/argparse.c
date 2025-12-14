#include <stdio.h>
#include "../csl-argparse.h"
#include "../csl-templates.h"

ARGPARSE_FLAGS(
    (SAY_HI, 's'),
    (SAY_HELLO, 'S'),
    (NERD_OUT, 'n')
);

ARGPARSE_OPTARGS(
    (custom_message, "custom-message"),   
    (custom_number, "custom-number"),   
);

ARGPARSE_POSITIONALS(3,
    (PROGRAM, 0),           
    (FILE_NAME, 1),           
    (OUTPUT_FILE, 2),
);

int main(int argc, char** argv) {
    
    return 0;
}

