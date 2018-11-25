#ifndef _COREMAP_H_
#define _COREMAP_H_

/* Coremap related data structures */
enum coremap_state {
        FREE,
        ALLOCATED,
        FIXED
};

struct coremap_entry {
        enum coremap_state state;
        unsigned long chunksize;
};

uint32_t num_total_frames; /* Total number of physical frames */
uint32_t num_free_frames;  /* Total number of free frames */
uint32_t first_index;      /* First free index */

struct coremap_entry *coremap; /* Tracks the status of each physical frame */

/* Coremap initialization function */
void coremap_bootstrap(void);

/* Gets free pages using coremap */
paddr_t coremap_getpages(unsigned long npages);

/* Frees pages using coremap */
void coremap_freepages(vaddr_t vaddr);

#endif /* _COREMAP_H_ */
