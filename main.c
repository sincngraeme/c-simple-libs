#define GNU

#include <stdio.h>
#include "dstring.h"
#include "errval.h"

DSTRING_INIT(String);

int main() {

    dstring my_string = UNWRAP(String.new("Hello There!\n"), {
        fprintf(stderr, "Failed to create string!\n");
        return 1;
    });
    printf("My string is: %s\n%lu characters long.\n", 
        UNWRAP(String.str(my_string), {
            fprintf(stderr, "Failed to get string!\n");
            return 1;
        }),
        UNWRAP(String.size(my_string), {
            fprintf(stderr, "Failed to get string size!\n");
            return 1;
        })
    );
    UNWRAP(String.append(&my_string, "General Kenobi\n"), {
        fprintf(stderr, "Failed to append to string!\n");
        String.del(&my_string);
        return 1;
    });
    printf("My string is: %s\n%lu characters long.\n",
        UNWRAP(String.str(my_string), {
            fprintf(stderr, "Failed to get string!\n");
            String.del(&my_string);
            return 1;
        }),
        UNWRAP(String.size(my_string), {
            fprintf(stderr, "Failed to get string size!\n");
            String.del(&my_string);
            return 1;
        })
    );
    printf("Now let's test indexing into the string!\n");
    int index = 0;
    do {
        printf("First character: %c (%d)\n", UNWRAP(String.index(my_string, index), {
            fprintf(stderr, "[1] Failed to get character at index: %d, Out of bounds\n", index); 
            break;
        }), index);
    } while(0);
    index = UNWRAP(String.len(my_string),{
        fprintf(stderr, "[2] Failed to get string size!\n");
        String.del(&my_string);
        return 1;
    });
    do {
        printf("Last character: %c (%d)\n", UNWRAP(String.index(my_string, index), {
            fprintf(stderr, "[3] Failed to get character at index: %d, Out of bounds\n", index); 
            break;
        }), index);
    } while(0);
    index += 2;
    do {
        printf("Out of bounds character: %c (%d)\n", UNWRAP(String.index(my_string, index), {
            fprintf(stderr, "[4] Failed to get character at index: %d, Out of bounds\n", index); 
            break;
        }), index);
    } while(0);

    printf("Printing all the characters one by one to test bounds check\n");
    char ch = -1;
    for(int i = 0; /*ch != '\0'*/; i++) {
        ch = UNWRAP(String.index(my_string, i), {
            fprintf(stderr, "Error: out of bounds!\n");
            break;
        });
        printf("%c @(%d)\n", ch, i);
    }
    do {
        printf("Now test the negative bounds check\n");
        printf("Character at index %d is: %c\n", 
                UNWRAP(String.index(my_string, -1), {
                    fprintf(stderr, "Error: Out of bounds!\n");
                    break;
                }), -1);
    } while(0);

    printf("Now test pushing to the begining of the string\n");
    UNWRAP(String.prepend(&my_string, "Some preamble... "), {
        fprintf(stderr, "Error: Failed to push to string\n");
        String.del(&my_string);
        return 1;
    });
    printf("New String: %s\n", UNWRAP(String.str(my_string), {
        fprintf(stderr, "Error: Failed to access string\n"); 
        String.del(&my_string);
        return 1;
    }));
    do {
        printf("Now test removing from the begining of the string\n");
        char* token = UNWRAP(String.token(&my_string, 4), {
            fprintf(stderr, "Error: Failed to remove token\n");
            break;
        });
        printf("New String: %s\n", UNWRAP(String.str(my_string), {
            fprintf(stderr, "Error: Failed to access string\n"); 
            String.del(&my_string);
            return 1;
        }));
        printf("Token: %s\n", token);
    } while(0);

    String.del(&my_string);
    return 0;
}
