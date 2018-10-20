#ifndef _PROCLIST_H_
#define _PROCLIST_H_

struct proc; /* from <proc.h> */

struct proclistnode {
        struct proclistnode *pln_prev;
        struct proclistnode *pln_next;
        struct proc *pln_self;
};

struct proclist {
	struct proclistnode pl_head;
	struct proclistnode pl_tail;
	unsigned pl_count;
};

/* Initialize and clean up a proc list node. */
void proclistnode_init(struct proclistnode *pln, struct proc *self);
void proclistnode_cleanup(struct proclistnode *pln);

/* Initialize and clean up a proc list. Must be empty at cleanup. */
void proclist_init(struct proclist *pl);
void proclist_cleanup(struct proclist *pl);

/* Check if it's empty */
bool proclist_isempty(struct proclist *pl);

/* Add and remove: at ends */
void proclist_addhead(struct proclist *pl, struct proc *p);
void proclist_addtail(struct proclist *pl, struct proc *p);
struct proc *proclist_remhead(struct proclist *pl);
struct proc *proclist_remtail(struct proclist *pl);

/* Add and remove: in middle. (PL is needed to maintain ->pl_count.) */
void proclist_insertafter(struct proclist *pl,
			    struct proc *onlist, struct proc *addee);
void proclist_insertbefore(struct proclist *pl,
			     struct proc *addee, struct proc *onlist);
void proclist_remove(struct proclist *pl, struct proc *p);

/* Iteration; itervar should previously be declared as (struct proc *) */
#define PROCLIST_FORALL(itervar, pl) \
	for ((itervar) = (pl).pl_head.pln_next->pln_self; \
	     (itervar) != NULL; \
	     (itervar) = (itervar)->p_listnode.pln_next->pln_self)

#define PROCLIST_FORALL_REV(itervar, pl) \
	for ((itervar) = (pl).pl_tail.pln_prev->pln_self; \
	     (itervar) != NULL; \
	     (itervar) = (itervar)->p_listnode.pln_prev->pln_self)

#endif
