#ifndef _EXECV_H_
#define _EXECV_H_

#define MAXEXECVARGS 16

/* Count arguments of given argument array */
int count_args(char **args, int *ret);

/* Copy arguments into allocated space in kernel */
int copy_args(char **dest, char **src, int len);

/* Copy arguments into allocated space in kernel */
int kcopy_args(char **dest, char **src, int len);

/* Load arguments into user address space stack */
int load_args(userptr_t *sp, char **src, int len );

#endif /* _EXECV_H_ */ 
