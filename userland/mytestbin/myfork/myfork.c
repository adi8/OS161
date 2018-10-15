#include <stdio.h>
#include <unistd.h>

int
main()
{
        int pid = fork();
        if (pid == -1) {
                printf("Fork failed\n");
        }
        else if (pid == 0) {
                printf("Hello from the child\n");
        }
        else {
                printf("Hello from parent\n");
        }

        return 0;
}
