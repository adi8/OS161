/*
 * Constants and data structures required
 * for pid maintanence.
 */
#ifndef _PID_H_
#define _PID_H_

#include <synch.h>
#include <limits.h>

struct lock *pid_gen_lock; /* Lock to generate pid */
int32_t pid_gen;           /* Counter to generate new process id */
uint32_t counter;          /* Stores number of active processes */

// TODO: Could define array using array.h
bool pid_arr[PID_MAX];     /* List of pids assigned */

/* Call once during system startup to allocate data structures */
void pid_bootstrap(void);

/* Call to retrieve next pid to assign */
int pid_retrieve(pid_t *ret);

/* Call to reclaim pid */
int pid_reclaim(pid_t pid);

#endif
