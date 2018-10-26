#include <unistd.h>
#include <stdio.h>

int
main(int argc, char **argv) {
        printf("Args given is %d\n", argc);
        for (int i = 0; i < argc; i++) {
                printf("Arg %d: %s\n", i, *(argv+i));
        }
        return 0;
}
