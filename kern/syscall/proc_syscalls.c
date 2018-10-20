#include <types.h>
#include <kern/errno.h>
#include <limits.h>
#include <cdefs.h>
#include <lib.h>
#include <mips/trapframe.h>
#include <addrspace.h>
#include <current.h>
#include <thread.h>
#include <proc.h>
#include <proclist.h>
#include <syscall.h>

/*
 * Return the pid of current process.
 */
int
sys_getpid(int *retval)
{
        *retval = curproc->pid;
        return 0;
}

/*
 * Enter user mode for a newly forked process.
 */
void
enter_forked_process(struct trapframe *tf)
{
        // Return value for child is 0
        tf->tf_v0   = 0;
        // Indicate a successful return
        tf->tf_a3   = 0;
        // Increment pc so that fork is not called again
        tf->tf_epc += 4;

        struct trapframe child_tf;
        bzero(&child_tf, sizeof(child_tf));
        memcpy(&child_tf, tf, sizeof(child_tf));
                
        kfree(tf);

        as_activate();
        mips_usermode(&child_tf);
}

/*
 * Create a copy of the existing process
 */
int
sys_fork(struct trapframe *proc_tf, int *retval)
{
        /* Ref to current process */
        struct proc *proc = curproc;
        
        if (proc->p_child.pl_count <= MAX_CHILDREN) {
                /* Copy trap frame of current process */
                struct trapframe *child_tf = kmalloc(sizeof(*proc_tf));
                if (child_tf == NULL) {
                        return ENOMEM;
                }
                memcpy(child_tf, proc_tf, sizeof(*proc_tf));

                /* Copy proc structure of current process */
                struct proc **child_proc =
                        kmalloc(sizeof(struct addrspace*));
                if (child_proc == NULL) {
                        kfree(child_tf);
                        return ENOMEM;
                }
                int err = proc_fork(child_proc);
                if (err) {
                        kfree(child_tf);
                        return err; 
                }

                /* Adding the child proc in parent childrens list */
                proclist_addhead(&proc->p_child, *child_proc);        
                
                /* Assign a ppid to child proc */
                (*child_proc)->ppid = proc->pid;
                
                /* Copy addrspace of current process */
                struct addrspace *proc_addrspace = proc_getas();
                struct addrspace **child_addrspace = 
                        kmalloc(sizeof(struct addrspace*)); 
                if (child_addrspace == NULL) {
                        proc_destroy(*child_proc);
                        kfree(child_proc);
                        kfree(child_tf);
                        return ENOMEM;
                }
                err = as_copy(proc_addrspace, child_addrspace);
                if (err) {
                        proc_destroy(*child_proc);
                        kfree(child_proc);
                        kfree(child_tf);
                        return err;
                }
                (*child_proc)->p_addrspace = *child_addrspace;

                /* Copy current thread */
                err = thread_fork("child", *child_proc,
                                  enter_forked_process,
                                  child_tf, 0);
                if (err) {
                        kfree(child_addrspace);
                        proc_destroy(*child_proc);
                        kfree(child_proc);
                        kfree(child_tf);
                        return err;
                }
                

                /* Return value of current process */
                *retval = (*child_proc)->pid;
        }
        else {
                kprintf("Too many processes\n");
                return EMPROC;
        }
        
        return 0;
}

/*
 * Terminate the current process and return exit code to
 * parent.
 *
 * Does not return.
 */
void
sys__exit(int exit_code)
{
        struct thread *curt = curthread;
        struct proc *proc   = curt->t_proc;

        // Set exit code and status for this process
        proc->exit_code   = exit_code;
        proc->exit_status = true;
        
        // Remove current thread from process
        proc_remthread(curt);
        
        // Detach all proc in our own child list if any
        if (!proclist_isempty(&proc->p_child)) {
                struct proc *itervar;
                spinlock_acquire(&proc->p_lock);
                // Iterate over all children and remove them from list
                while ((itervar = (proclist_remhead(&proc->p_child))) != NULL) {
                        spinlock_acquire(&itervar->p_lock);
                        // Mark child as orphan
                        itervar->ppid = -1;

                        // Destroy the child process that has
                        // already exited
                        if (itervar->exit_status && 
                            itervar->wait_count == 0) {
                                spinlock_release(&itervar->p_lock);  
                                proc_destroy(itervar); 
                        }
                        else {
                                spinlock_release(&itervar->p_lock);
                        }
                }
                spinlock_release(&proc->p_lock);
        }
        
        // Singal all processes waiting for this process
        lock_acquire(proc->p_wait_lock);
        cv_broadcast(proc->p_wait_cv, proc->p_wait_lock);

        // Destroy proc if no other proc waiting and has no parent
        if (proc->ppid == -1 && proc->wait_count == 0) {
                proc_destroy(proc); 
        }
        
        // Exit the thread
        thread_exit();
}
