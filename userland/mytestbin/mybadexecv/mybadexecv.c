#include <unistd.h>
#include <stdio.h>

int main() 
{
        printf("Printed from myexecv\n");
        char *const args[] = {(char *)"add", "1", "2", NULL};
        int ret = execv(NULL, args);
        printf("Retvale: %d\n", ret);
        return 0;
}
