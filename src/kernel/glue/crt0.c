/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <user_runtime.h>
#include <syscalls.h>
#include <string.h>

extern int __stdio_init(void);
extern int __stdio_deinit(void);

extern void sys_exit(int retcode);

extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);

extern int main(int argc,char **argv);

int _start(struct uspace_program_args *);
void _call_ctors(void);

static char empty[1];
char *__progname = empty;

char **environ = NULL;


int
_start(struct uspace_program_args *args)
{
	int retcode;
	register char *ap;
	_call_ctors();

//	__stdio_init();

	if ((ap = args->argv[0])) {
		if ((__progname = strrchr(ap, '/')) == NULL)
			__progname = ap;
		else
			++__progname;
	}

	environ = args->envp;
	retcode = main(args->argc, args->argv);

//	__stdio_deinit();

	sys_exit(retcode);
	return 0;
}


void
_call_ctors(void)
{ 
	void (**f)(void);

	for (f = &__ctor_list; f < &__ctor_end; f++) {
		(**f)();
	}
}

