#include <types.h>
#include <cdefs.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <syscall.h>

/*
 * getpid() - return the pid of current process.
 */
int
sys_getpid(int *retval)
{
        *retval = curproc->pid;
        return 0;
}
