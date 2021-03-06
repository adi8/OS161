#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vnode.h>
#include <vfs.h>
#include <copyinout.h>
#include <syscall.h>
#include <execv.h>

/*
 * Copy arguments array within kernel.
 */
int
kcopy_args(char **dest, char **src, int len)
{
        int i;
        size_t sz;
        char *ret;

        for (i = 0; i < len; i++) {
                // Extract size of the string to store in dest
                sz = strlen(*(src + i)) + 1;
                
                // Allocate memory for the actual argument string
                // in dest
                *(dest + i) = kmalloc(sz);
                if (*(dest + i) == NULL) {
                        return ENOMEM;
                }
        
                ret = strcpy(*(dest + i), *(src + i));
                if (ret == NULL) {
                        return ENOMEM;
                }
        }

        *(dest + i) = NULL;

        return 0;
}

/*
 * Copy arguments array from userspace to kernel space.
 *
 */
int
copy_args(char **dest, char **src, int len)
{
        int i, result;
        size_t sz;
        for (i = 0; i < len; i++) {
                // Extract size of the string to store in dest
                sz = strlen(*(src + i)) + 1;
                
                // Allocate memory for the actual argument string
                // in dest
                *(dest + i) = kmalloc(sz);
                if (*(dest + i) == NULL) {
                        return ENOMEM;
                }
        
                //char *ret = strcpy(*(dest + i), *(src + i));
                result = copyinstr((const_userptr_t)*(src + i), *(dest + i), sz, NULL);
                if (result) {
                        return result;
                }
        }

        *(dest + i) = NULL;

        return 0;
}

/*
 * Load arguments into user address space stack
 */
int
load_args(userptr_t *sp, char **src, int len)
{
        /* 
         * This is required to track pointers to argument strings in 
         * userspace. Then pointer array will be created with same ptr 
         * values in userspace stack.
         */
        char **argv_ptrs = kmalloc((len + 1) * sizeof(char*));

        int i;
        for (i = len - 1; i >= 0; i--) {
                // Calculate size in bytes required for arg string
                size_t sz = strlen(*(src + i)) + 1;

                // Decrement sp according to size as stack is
                // top to bottom.
                *sp -= sz;

                // Store pointer in argv_ptrs
                *(argv_ptrs + i) = (char *) *sp;

                // Copy string into user stack
                bzero(*sp, sz);
                int res = copyoutstr((const char *)*(src + i), *sp, sz, NULL);
                if (res) {
                        kfree(argv_ptrs);
                        return res;
                }
        }
        // Mark last array element as null
        *(argv_ptrs + len) = NULL;

        // Size of argv_ptrs array
        int array_sz = (len + 1) * sizeof(char *);
        
        // Decrement sp according to size of pointer array to be created
        // in userspace stack
        *sp -= array_sz;

        // Check if stack pointer is aligned. Aligned means that sp should 
        // be divisible by 8
        int mod = (vaddr_t) *sp % 8;
        if (mod) {
                *sp -= mod;
        }

        // Copy argv_ptrs into userspace stack
        bzero(*sp, array_sz);
        int res = copyout(argv_ptrs, *sp, array_sz);
        if (res) {
                kfree(argv_ptrs);
                return res;
        }

        /* Done with argv_ptrs */
        kfree(argv_ptrs);

        return 0;
}

/*
 * Return the Count of number of arguments in given array
 */
int
count_args(char **args, int *ret)
{
        // Check if valid args pointer passed
        char **kargs = kmalloc(sizeof(char **));
        int result = copyin((const_userptr_t)args, (void *)kargs,
                            sizeof(char **)); 
        if (result) {
                kfree(kargs);
                return result;
        }

        // Check the number of args passed
        int i = 0;
        for (i = 0; *(args+i) != NULL && i < MAXEXECVARGS; i++) {
                // Check if pointers in args are valid
                char *ptr = *(args + i);
                char *kptr = kmalloc(sizeof(char *));
                result = copyin((const_userptr_t)ptr, (void *)kptr,
                                sizeof(char));
                if (result) {
                        kfree(kptr);
                        return result;
                }

                /* Done with kptr */
                kfree(kptr);
        }
        if (i == MAXEXECVARGS) {
                return E2BIG;
        }
        
        /* Done with kargs */
        kfree(kargs);

        *ret = i;
        return 0;
}

/*
 * Load and execute given program.
 */
int
sys_execv(const char *program, char **args)
{
        int result;

        /* Check if program name and args passed */
        if (program == NULL || args == NULL) {
                return EFAULT;
        }

        /* Check if pointer given is valid */
        size_t actual;
        char progname[128];
        result = copyinstr((const_userptr_t)program, progname,
                           128, &actual);
        if (result) {
                return result;
        }

        // If only '\0' copied return
        if (actual == 1) {
                return EINVAL;
        }

        /* Count number of args */
        int argc;
        result = count_args(args, &argc);
        if (result) {
                return result;
        }
        
        /* Copy args into kernel space */
        char **args_copy = kmalloc((argc + 1) * sizeof(char *));
        result = copy_args(args_copy, args, argc);
        if (result) {
                return result;
        }

        /* Open new program file */
        struct vnode *v;
        result = vfs_open(progname, O_RDONLY, 0, &v);
        if (result) {
                return result;
        }

        /* Remove current address space from current proc and destroy */
        KASSERT(proc_getas() != NULL);
        struct addrspace *as = proc_setas(NULL);
        as_deactivate();
        as_destroy(as);

        /* Create a new address space */
        as = as_create();
        if (as == NULL) {
                vfs_close(v);
                return ENOMEM;
        }
         
        /* Switch to it and activate it */
        proc_setas(as);
        as_activate();

        /* Load the executable */
        vaddr_t entrypoint;
        result = load_elf(v, &entrypoint);
        if (result) {
                vfs_close(v);
                return result;
        }

        /* Done with the file now */
        vfs_close(v);

        /* Define the user stack in the new address space */
        vaddr_t stackptr;
        result = as_define_stack(as, &stackptr);
        if (result) {
                return result;
        }

        /* Load user arguments into programs addresss space */
        userptr_t sp = (userptr_t) stackptr;
        result = load_args(&sp, args_copy, argc);
        if (result) {
                return result;
        }

        /* Done with args_copy */
        kfree(args_copy);

        userptr_t argv = sp;
        stackptr = (vaddr_t) sp;

	/* Warp to user mode. */
	enter_new_process(argc /*argc*/, argv /*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);

	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
        
}
