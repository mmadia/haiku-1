#include <syscalls.h>
#include <signal.h>

/*
 *  Copyright (c) 2002, OpenBeOS Project. All rights reserved.
 *  Distributed under the terms of the OpenBeOS license.
 *
 *
 *  raise.c:
 *  implements the signal function raise()
 *
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 *
 */


int
raise(int sig)
{
	thread_id tid = sys_get_current_thread_id();
	
	return sys_send_signal(tid, sig);
}

