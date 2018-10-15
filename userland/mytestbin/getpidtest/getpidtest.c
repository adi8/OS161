#include <unistd.h>
#include <stdio.h>

int 
main(void) 
{
        printf("My pid is %d\n", getpid());
        return 0;
}
