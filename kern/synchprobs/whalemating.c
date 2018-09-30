/*
 * Copyright (c) 2001, 2002, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Driver code for whale mating problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <test.h>

#define NMATING 10

static struct semaphore *done_sem;
static struct semaphore *male_sem;
static struct semaphore *female_sem;
static struct semaphore *matchmaker_sem;
static struct semaphore *mating_sem;
//static struct semaphore *mating_sem;
static struct semaphore *test_sem;

static struct lock *mating_lock;
static struct lock *male_lock;
static struct lock *female_lock;
static struct lock *matchmaker_lock;
static struct lock *count_lock;

static struct cv *male_cv;
static struct cv *female_cv;
static struct cv *matchmaker_cv;
static struct cv *mating_cv;
static struct cv *mating_fin_cv;

volatile unsigned int male_count;
volatile unsigned int female_count;
volatile unsigned int matchmaker_count;
volatile unsigned int mating_count;

/* Initializing */
static
void
inititems(void)
{
        // Create semaphores
        if (done_sem == NULL) {
                done_sem = sem_create("done_sem", 0);
                if (done_sem == NULL) {
                        panic("whalemating: sem_create failed\n");
                }
        }

        if (male_sem == NULL) {
                male_sem = sem_create("male_sem", 1);
                if (male_sem == NULL) {
                        panic("whalemating: sem_create failed\n");
                }
        }

        if (female_sem == NULL) {
                female_sem = sem_create("female_sem", 1);
                if (female_sem == NULL) {
                        panic("whalemating: sem_create failed\n");
                }
        }

        if (matchmaker_sem == NULL) {
                matchmaker_sem = sem_create("matchmaker_sem", 1);
                if (matchmaker_sem == NULL) {
                        panic("whalemating: sem_create failed\n");
                }
        }
        
        if (mating_sem == NULL) {
                mating_sem = sem_create("mating_sem", 3);
                if (mating_sem == NULL) {
                        panic("whalemating: sem_create failed\n");
                }
        }

        if (test_sem == NULL) {
                test_sem = sem_create("test_sem", 1);
                if (test_sem == NULL) {
                        panic("whalemating: sem_create failed\n");
                }
        }

        // Create locks 
        if (male_lock == NULL) {
                male_lock = lock_create("male_lock");
                if (male_lock == NULL) {
                        panic("whalemating: lock_create failed\n");
                }
        }

        if (female_lock == NULL) {
                female_lock = lock_create("female_lock");
                if (female_lock == NULL) {
                        panic("whalemating: lock_create failed\n");
                }
        }

        if (matchmaker_lock == NULL) {
                matchmaker_lock = lock_create("matchmaker_lock");
                if (matchmaker_lock == NULL) {
                        panic("whalemating: lock_create failed\n");
                }
        }

        if (mating_lock == NULL) {
                mating_lock = lock_create("mating_lock");
                if (mating_lock == NULL) {
                        panic("whalemating: lock_create failed\n");
                }
        }

        if (count_lock == NULL) {
                count_lock = lock_create("count_lock");
                if (count_lock == NULL) {
                        panic("whalemating: lock_create failed\n");
                }
        }

        // Create condition variables
        if (male_cv == NULL) {
                male_cv = cv_create("male_cv");
                if (male_cv == NULL) {
                        panic("whalemating: cv_create failed\n");
                }
        }

        if (female_cv == NULL) {
                female_cv = cv_create("female_cv");
                if (female_cv == NULL) {
                        panic("whalemating: cv_create failed\n");
                }
        }

        if (matchmaker_cv == NULL) {
                matchmaker_cv = cv_create("matchmaker_cv");
                if (matchmaker_cv == NULL) {
                        panic("whalemating: cv_create failed\n");
                }
        }
        
        if (mating_cv == NULL) {
                mating_cv = cv_create("mating_cv");
                if (mating_cv == NULL) {
                        panic("whalemating: cv_create failed\n");
                }
        }

        if (mating_fin_cv == NULL) {
                mating_fin_cv = cv_create("mating_fin_cv");
                if (mating_fin_cv == NULL) {
                        panic("whalemating: cv_create failed\n");
                }
        }

        // Initialize counts
        male_count = 0;
        female_count = 0;
        matchmaker_count = 0;
        mating_count = 0;
}


static
void
male(void *p, unsigned long which)
{
	(void)p;

        kprintf("%-11s\t%-4ld\t%-10s\n", "Male", which, "Starting");

        P(male_sem);
        
        lock_acquire(mating_lock);
        P(mating_sem);
        kprintf("%-11s\t%-4ld\t%-10s\n", "Male", which, "Mating");
        if (mating_sem->sem_count == 0) {
                cv_broadcast(mating_cv, mating_lock);
        }
        else {
                cv_wait(mating_cv, mating_lock);
        }

        V(mating_sem);
        lock_release(mating_lock);

        lock_acquire(count_lock);
        if (mating_sem->sem_count == 3) {
                cv_broadcast(mating_fin_cv, count_lock);
        }
        else {
                cv_wait(mating_fin_cv, count_lock);
                lock_release(count_lock);
        }

        kprintf("%-11s\t%-4ld\t%-10s\n", "Male", which, "Finished");
        V(male_sem);

        V(done_sem);
}

static
void
female(void *p, unsigned long which)
{
	(void)p;

        kprintf("%-11s\t%-4ld\t%-10s\n", "Female", which, "Starting");

        P(female_sem);

        lock_acquire(mating_lock);
        P(mating_sem);
        kprintf("%-11s\t%-4ld\t%-10s\n", "Female", which, "Mating");
        if (mating_sem->sem_count == 0) {
                cv_broadcast(mating_cv, mating_lock);
        }
        else {
                cv_wait(mating_cv, mating_lock);
        }

        V(mating_sem);
        lock_release(mating_lock);

        lock_acquire(count_lock);
        if (mating_sem->sem_count == 3) {
                cv_broadcast(mating_fin_cv, count_lock);
        }
        else {
                cv_wait(mating_fin_cv, count_lock);
                lock_release(count_lock);
        }

        kprintf("%-11s\t%-4ld\t%-10s\n", "Female", which, "Finished");
        V(female_sem);

        V(done_sem);
}

static
void
matchmaker(void *p, unsigned long which)
{
	(void)p;

        kprintf("%-11s\t%-4ld\t%-10s\n", "Matchmaker", which, "Starting");
        
        P(matchmaker_sem);

        lock_acquire(mating_lock);
        P(mating_sem);
        kprintf("%-11s\t%-4ld\t%-10s\n", "Matchmaker", which, "Assisting");
        if (mating_sem->sem_count == 0) {
                cv_broadcast(mating_cv, mating_lock);
        }
        else {
                cv_wait(mating_cv, mating_lock);
        }

        V(mating_sem);
        lock_release(mating_lock);

        lock_acquire(count_lock);
        if (mating_sem->sem_count == 3) {
                cv_broadcast(mating_fin_cv, count_lock);
        }
        else {
                cv_wait(mating_fin_cv, count_lock);
                lock_release(count_lock);
        }

        kprintf("%-11s\t%-4ld\t%-10s\n", "Matchmaker", which, "Finished");
        V(matchmaker_sem);

        V(done_sem);
}


// Change this function as necessary
int
whalemating(int nargs, char **args)
{

	int i, j, err=0;

	(void)nargs;
	(void)args;

        inititems();

        kprintf("Whale mating begins\n");

        kprintf("%-11s\t%-4s\t%-10s\n", "Thread", "No.", "Status");

	for (i = 0; i < 3; i++) {
		for (j = 0; j < NMATING; j++) {
			switch(i) {
			    case 0:
				err = thread_fork("Male Whale Thread",
						  NULL, male, NULL, j);
				break;
			    case 1:
				err = thread_fork("Female Whale Thread",
						  NULL, female, NULL, j);
				break;
			    case 2:
				err = thread_fork("Matchmaker Whale Thread",
						  NULL, matchmaker, NULL, j);
				break;
			}
			if (err) {
				panic("whalemating: thread_fork failed: %s)\n",
				      strerror(err));
			}
		}
	}

        for (i = 0; i < 3; i++) {
                for (j = 0; j < NMATING; j++) {
                        P(done_sem);
                }
        }

        kprintf("Whale mating finished\n");

	return 0;
}
