#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <current.h>
#include <copyinout.h>
#include <syscall.h>
#include <execv.h>

/*
 * Copy arguments array from userspace to kernel space.
 *
 */
int
copy_args(char **dest, char **src, int len)
{
        int i;
        size_t sz;
        for (i = 0; i < len; i++) {
                // Extract size of the string to store in dest
                sz = strlen(*(src + i)) + 1;
                
                // Allocate memory for the actual argument string
                // in dest
                *(dest + i) = kmalloc(sz);
        
                char *ret = strcpy(*(dest + i), *(src + i));
                if (ret == NULL) {
                        return ENOMEM;
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
                size_t actual;
                int res = copyoutstr((const char *)*(src + i), *sp, sz, &actual);
                if (res) {
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
                return res;
        }

        return 0;
}
