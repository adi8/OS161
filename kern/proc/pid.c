#include <types.h>
#include <kern/errno.h>
#include <array.h>
#include <limits.h>
#include <synch.h>
#include <pid.h>

/*
 * Initialize the pid generating system.
 */
void
pid_bootstrap(void)
{
        counter  = 0;
        pid_gen  = PID_MIN;

        pid_gen_lock = lock_create("pid_gen_lock");
        if (pid_gen_lock == NULL) {
                panic("lock_create for counter_lock failed\n");
        }

        for (int i=0; i<PID_MAX-2; i++) {
                pid_arr[i] = 0;
        }

}

/*
 * Retrieve a pid.
 */
int
pid_retrieve(pid_t *ret)
{
        /* 
         * Check if maximum number of processes reached
         * PID_MAX - 2 because 0 and 1 pid not used
         */
        if (counter < PID_MAX - 2) {
                lock_acquire(pid_gen_lock);

                // Loop to find available pid
                do {
                        /* 
                         * When pid_gen wraps around it should 
                         * start at PID_MIN and not 0.
                         */
                        pid_gen = (pid_gen == PID_MAX ) ? 
                                      PID_MIN : pid_gen + 1;
                } while (pid_arr[pid_gen]);
                
                // Set generated pid to mark as assigned
                pid_arr[pid_gen] = 1;

                // Increment number of pid assigned in system              
                counter++;

                // Assign generated value to process.
                *ret = pid_gen;

                lock_release(pid_gen_lock);
        }
        else {
                *ret = -1;
                kprintf("Too many processes in the system");
                return ENPROC;
        }


        return 0;
}

/*
 * Reclaim a pid by removing pid from list.
 */
int
pid_reclaim(pid_t pid)
{
        lock_acquire(pid_gen_lock);

        /*
         * Should validate pid but as user proc cannot call 
         * this, I am trusting the pid does exist
         */

        // Unset pid to mark as unassigned
        pid_arr[pid] = 0; 

        // Decrement number of pid assigned in system
        counter--; 

        lock_release(pid_gen_lock);
        return 0;
}
