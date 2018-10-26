#ifndef _EXECV_H_
#define _EXECV_H_

/* Copy arguments into allocated space in kernel */
int copy_args(char **dest, char **src, int len);

/* Load arguments into user address space stack */
int load_args(userptr_t *sp, char **src, int len );

#endif /* _EXECV_H_ */ 
