/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the Haiku License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* This file contains the debugger */

#include "blue_screen.h"

#include <debug.h>
#include <frame_buffer_console.h>
#include <gdb.h>

#include <smp.h>
#include <int.h>
#include <vm.h>
#include <driver_settings.h>
#include <arch/debug_console.h>
#include <arch/debug.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef struct debugger_command {
	struct debugger_command *next;
	int (*func)(int, char **);
	const char *name;
	const char *description;
} debugger_command;

int dbg_register_file[B_MAX_CPU_COUNT][14];
	/* XXXmpetit -- must be made generic */

static bool sSerialDebugEnabled = false;
static bool sSyslogOutputEnabled = false;
static bool sBlueScreenEnabled = false;
static bool sBlueScreenOutput = false;
static spinlock sSpinlock = 0;
static int32 sDebuggerOnCPU = -1;

static struct debugger_command *sCommands;

#define OUTPUT_BUFFER_SIZE 1024
static char sOutputBuffer[OUTPUT_BUFFER_SIZE];

#define LINE_BUFFER_SIZE 1024
#define MAX_ARGS 16
#define HISTORY_SIZE 16

static char sLineBuffer[HISTORY_SIZE][LINE_BUFFER_SIZE] = { "", };
static char sParseLine[LINE_BUFFER_SIZE] = "";
static int32 sCurrentLine = 0;
static char *args[MAX_ARGS] = { NULL, };

#define distance(a, b) ((a) < (b) ? (b) - (a) : (a) - (b))


static debugger_command *
find_command(char *name, bool partialMatch)
{
	debugger_command *command;
	int length;

	// search command by full name

	for (command = sCommands; command != NULL; command = command->next) {
		if (strcmp(name, command->name) == 0)
			return command;
	}

	// if it couldn't be found, search for a partial match

	if (partialMatch) {
		length = strlen(name);
		if (length == 0)
			return NULL;

		for (command = sCommands; command != NULL; command = command->next) {
			if (strncmp(name, command->name, length) == 0)
				return command;
		}
	}

	return NULL;
}


static void
kputchar(char c)
{
	if (sSerialDebugEnabled)
		arch_debug_serial_putchar(c);
	if (sBlueScreenOutput)
		blue_screen_putchar(c);
}


static void
kputs(const char *s)
{
	if (sSerialDebugEnabled)
		arch_debug_serial_puts(s);
	if (sBlueScreenOutput)
		blue_screen_puts(s);
}


static int
read_line(char *buffer, int32 maxLength)
{
	int32 currentHistoryLine = sCurrentLine;
	int32 position = 0;
	bool done = false;
	char c;

	char (*readChar)(void);
	if (sBlueScreenEnabled)
		readChar = blue_screen_getchar;
	else
		readChar = arch_debug_serial_getchar;

	while (!done) {
		c = readChar();

		switch (c) {
			case '\n':
			case '\r':
				buffer[position++] = '\0';
				kputchar('\n');
				done = true;
				break;
			case 8: // backspace
				if (position > 0) {
					kputs("\x1b[1D"); // move to the left one
					kputchar(' ');
					kputs("\x1b[1D"); // move to the left one
					position--;
				}
				break;
			case 27: // escape sequence
				c = readChar();
				if (c != '[') {
					// ignore broken escape sequence
					break;
				}
				c = readChar();
				switch (c) {
					case 'C': // right arrow acts like space
						buffer[position++] = ' ';
						kputchar(' ');
						break;
					case 'D': // left arrow acts like backspace
						if (position > 0) {
							kputs("\x1b[1D"); // move to the left one
							kputchar(' ');
							kputs("\x1b[1D"); // move to the left one
							position--;
						}
						break;
					case 'A': // up arrow
					case 'B': // down arrow
					{
						int32 historyLine = 0;

						if (c == 65) {
							// up arrow
							historyLine = currentHistoryLine - 1;
							if (historyLine < 0)
								historyLine = HISTORY_SIZE - 1;
						} else {
							// down arrow
							if (currentHistoryLine == sCurrentLine)
								break;

							historyLine = currentHistoryLine + 1;
							if (historyLine >= HISTORY_SIZE)
								historyLine = 0;
						}

						// clear the history again if we're in the current line again
						// (the buffer we get just is the current line buffer)
						if (historyLine == sCurrentLine)
							sLineBuffer[historyLine][0] = '\0';

						// swap the current line with something from the history
						if (position > 0)
							kprintf("\x1b[%ldD", position); // move to beginning of line

						strcpy(buffer, sLineBuffer[historyLine]);
						position = strlen(buffer);
						kprintf("%s\x1b[K", buffer); // print the line and clear the rest
						currentHistoryLine = historyLine;
						break;
					}
					default:
						break;
				}
				break;
			case '$':
			case '+':
				if (!sBlueScreenEnabled) {
					/* HACK ALERT!!!
					 *
					 * If we get a $ at the beginning of the line
					 * we assume we are talking with GDB
					 */
					if (position == 0) {
						strcpy(buffer, "gdb");
						position = 4;
						done = true;
						break;
					} 
				}
				/* supposed to fall through */
			default:
				buffer[position++] = c;
				kputchar(c);
		}
		if (position >= maxLength - 2) {
			buffer[position++] = '\0';
			kputchar('\n');
			done = true;
			break;
		}
	}

	return position;
}


static int
parse_line(char *buf, char **argv, int *argc, int max_args)
{
	int pos = 0;

	strcpy(sParseLine, buf);

	if (!isspace(sParseLine[0])) {
		argv[0] = sParseLine;
		*argc = 1;
	} else
		*argc = 0;

	while (sParseLine[pos] != '\0') {
		if (isspace(sParseLine[pos])) {
			sParseLine[pos] = '\0';
			// scan all of the whitespace out of this
			while (isspace(sParseLine[++pos]))
				;
			if (sParseLine[pos] == '\0')
				break;
			argv[*argc] = &sParseLine[pos];
			(*argc)++;

			if (*argc >= max_args - 1)
				break;
		}
		pos++;
	}

	return *argc;
}


static void
kernel_debugger_loop(void)
{
	sDebuggerOnCPU = smp_get_current_cpu();

	kprintf("Welcome to Kernel Debugging Land...\n");
	kprintf("Running on CPU %ld\n", sDebuggerOnCPU);

	for (;;) {
		struct debugger_command *cmd = NULL;
		int argc;

		kprintf("kdebug> ");
		read_line(sLineBuffer[sCurrentLine], LINE_BUFFER_SIZE);
		parse_line(sLineBuffer[sCurrentLine], args, &argc, MAX_ARGS);

		// We support calling last executed command again if
		// B_KDEDUG_CONT was returned last time, so cmd != NULL
		if (argc <= 0 && cmd == NULL)
			continue;

		sDebuggerOnCPU = smp_get_current_cpu();

		if (argc > 0)
			cmd = find_command(args[0], true);

		if (cmd == NULL)
			kprintf("unknown command, enter \"help\" to get a list of all supported commands\n");
		else {
			int rc = cmd->func(argc, args);

			if (rc == B_KDEBUG_QUIT)
				break;	// okay, exit now.

			if (rc != B_KDEBUG_CONT)
				cmd = NULL;		// forget last command executed...
		}

		if (++sCurrentLine >= HISTORY_SIZE)
			sCurrentLine = 0;
	}

	sDebuggerOnCPU = -1;
}


static int
cmd_reboot(int argc, char **argv)
{
	arch_cpu_shutdown(true);
	return 0;
		// I'll be really suprised if this line ever runs! ;-)
}


static int
cmd_help(int argc, char **argv)
{
	debugger_command *command, *specified = NULL;
	const char *start = NULL;
	int32 startLength = 0;

	if (argc > 1) {
		specified = find_command(argv[1], false);
		if (specified == NULL) {
			start = argv[1];
			startLength = strlen(start);
		}
	}

	if (specified != NULL) {
		// only print out the help of the specified command (and all of its aliases)
		kprintf("debugger command for \"%s\" and aliases:\n", specified->name);
	} else if (start != NULL)
		kprintf("debugger commands starting with \"%s\":\n", start);
	else
		kprintf("debugger commands:\n");

	for (command = sCommands; command != NULL; command = command->next) {
		if (specified && command->func != specified->func)
			continue;
		if (start != NULL && strncmp(start, command->name, startLength))
			continue;

		kprintf(" %-20s\t\t%s\n", command->name, command->description ? command->description : "-");
	}

	return 0;
}


static int
cmd_continue(int argc, char **argv)
{
	return B_KDEBUG_QUIT;
}


//	#pragma mark - private kernel API


bool
debug_debugger_running(void)
{
	return sDebuggerOnCPU != -1;
}


void
debug_puts(const char *string)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	kputs(string);

	release_spinlock(&sSpinlock);
	restore_interrupts(state);
}


void
debug_early_boot_message(const char *string)
{
	arch_debug_serial_early_boot_message(string);
}


status_t
debug_init(kernel_args *args)
{
	return arch_debug_console_init(args);
}


status_t
debug_init_post_vm(kernel_args *args)
{
	void *handle;

	add_debugger_command("help", &cmd_help, "List all debugger commands");
	add_debugger_command("reboot", &cmd_reboot, "Reboot the system");
	add_debugger_command("gdb", &cmd_gdb, "Connect to remote gdb");
	add_debugger_command("continue", &cmd_continue, "Leave kernel debugger");
	add_debugger_command("exit", &cmd_continue, NULL);
	add_debugger_command("es", &cmd_continue, NULL);

	frame_buffer_console_init(args);
	arch_debug_console_init_settings(args);

	// get debug settings
	handle = load_driver_settings("kernel");
	if (handle != NULL) {
		if (get_driver_boolean_parameter(handle, "serial_debug_output", true, true))
			sSerialDebugEnabled = true;
		if (get_driver_boolean_parameter(handle, "syslog_debug_output", false, false))
			sSyslogOutputEnabled = true;
		if (get_driver_boolean_parameter(handle, "bluescreen", false, false)) {
			if (blue_screen_init() == B_OK)
				sBlueScreenEnabled = true;
		}

		unload_driver_settings(handle);
	}

	return arch_debug_init(args);
}


//	#pragma mark - public API


int
add_debugger_command(char *name, int (*func)(int, char **), char *desc)
{
	cpu_status state;
	struct debugger_command *cmd;

	cmd = (struct debugger_command *)malloc(sizeof(struct debugger_command));
	if (cmd == NULL)
		return ENOMEM;

	cmd->func = func;
	cmd->name = name;
	cmd->description = desc;

	state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	cmd->next = sCommands;
	sCommands = cmd;

	release_spinlock(&sSpinlock);
	restore_interrupts(state);

	return B_NO_ERROR;
}


int
remove_debugger_command(char * name, int (*func)(int, char **))
{
	struct debugger_command *cmd = sCommands;
	struct debugger_command *prev = NULL;
	cpu_status state;

	state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	while (cmd) {
		if (!strcmp(cmd->name, name) && cmd->func == func)
			break;

		prev = cmd;
		cmd = cmd->next;
	}

	if (cmd) {
		if (cmd == sCommands)
			sCommands = cmd->next;
		else
			prev->next = cmd->next;
	}

	release_spinlock(&sSpinlock);
	restore_interrupts(state);

	if (cmd) {
		free(cmd);
		return B_NO_ERROR;
	}

	return B_NAME_NOT_FOUND;
}


uint32
parse_expression(const char *expression)
{
	return strtoul(expression, NULL, 0);
}


void
panic(const char *format, ...)
{
	va_list args;
	char temp[128];
	
	set_dprintf_enabled(true);

	va_start(args, format);
	vsnprintf(temp, sizeof(temp), format, args);
	va_end(args);

	kernel_debugger(temp);
}


void
kernel_debugger(const char *message)
{
	cpu_status state;

	arch_debug_save_registers(&dbg_register_file[smp_get_current_cpu()][0]);
	set_dprintf_enabled(true);

	state = disable_interrupts();

	if (sDebuggerOnCPU != smp_get_current_cpu()) {
		// halt all of the other cpus

		// XXX need to flush current smp mailbox to make sure this goes
		// through. Otherwise it'll hang
		smp_send_broadcast_ici(SMP_MSG_CPU_HALT, 0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
	}

	if (sBlueScreenEnabled) {
		sBlueScreenOutput = true;
		blue_screen_enter();
	}

	if (message)
		kprintf("PANIC: %s\n", message);

	kernel_debugger_loop();

	sBlueScreenOutput = false;
	restore_interrupts(state);

	// ToDo: in case we change dbg_register_file - don't we want to restore it?
}


bool
set_dprintf_enabled(bool newState)
{
	bool oldState = sSerialDebugEnabled;
	sSerialDebugEnabled = newState;

	return oldState;
}


void
dprintf(const char *format, ...)
{
	cpu_status state;
	va_list args;

	if (!sSerialDebugEnabled)
		return;

	// ToDo: maybe add a non-interrupt buffer and path that only
	//	needs to acquire a semaphore instead of needing to disable
	//	interrupts?

	state = disable_interrupts();
	acquire_spinlock(&sSpinlock);

	va_start(args, format);
	vsnprintf(sOutputBuffer, OUTPUT_BUFFER_SIZE, format, args);
	va_end(args);

	arch_debug_serial_puts(sOutputBuffer);
	if (sBlueScreenOutput)
		blue_screen_puts(sOutputBuffer);

	release_spinlock(&sSpinlock);
	restore_interrupts(state);
}


/**	Similar to dprintf() but thought to be used in the kernel
 *	debugger only (it doesn't lock).
 */

void
kprintf(const char *format, ...)
{
	va_list args;

	// ToDo: don't print anything if the debugger is not running!

	va_start(args, format);
	vsnprintf(sOutputBuffer, OUTPUT_BUFFER_SIZE, format, args);
	va_end(args);

	if (sSerialDebugEnabled)
		arch_debug_serial_puts(sOutputBuffer);
	if (sBlueScreenOutput)
		blue_screen_puts(sOutputBuffer);
}


//	#pragma mark -
//	userland syscalls


void
_user_debug_output(const char *userString)
{
	char string[512];
	int32 length;

	if (!sSerialDebugEnabled)
		return;

	if (!IS_USER_ADDRESS(userString))
		return;

	do {
		length = user_strlcpy(string, userString, sizeof(string));
		debug_puts(string);
		userString += sizeof(string) - 1;
	} while (length >= (ssize_t)sizeof(string));
}
