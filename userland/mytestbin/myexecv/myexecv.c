#include <unistd.h>
#include <stdio.h>

int main() 
{
        printf("Printed from myexecv\n");
        char *const args[] = {(char *)"add", "1", "2", NULL};
        execv("/testbin/add", args);
        return 0;
}
