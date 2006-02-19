/*
 * Copyright 2004-2006 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 hid device driver
 *
 * Authors (in chronological order):
 *		Elad Lahav (elad@eldarshany.com)
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 *      Marcus Overhagen <marcus@overhagen.de>
 */
#ifndef __PS2_COMMON_H
#define __PS2_COMMON_H


#include <ISA.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>

#include "ps2_defs.h"
#include "ps2_dev.h"


// debug defines
#ifdef DEBUG
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// global variables
extern isa_module_info *gIsa;

extern device_hooks gKeyboardDeviceHooks;
extern device_hooks gMouseDeviceHooks;

extern bool gActiveMultiplexingEnabled;
extern sem_id gControllerSem;

// prototypes from common.c

status_t		ps2_init(void);
void			ps2_uninit(void);

uint8			ps2_read_ctrl(void);
uint8			ps2_read_data(void);
void			ps2_write_ctrl(uint8 ctrl);
void			ps2_write_data(uint8 data);
status_t		ps2_wait_read(void);
status_t		ps2_wait_write(void);

void			ps2_flush(void);

extern status_t ps2_command(uint8 cmd, const uint8 *out, int out_count, uint8 *in, int in_count);

// prototypes from keyboard.c & mouse.c
extern status_t probe_keyboard(void);

extern int32 mouse_handle_int(ps2_dev *dev, uint8 data);
extern int32 keyboard_handle_int(uint8 data);

#endif /* __PS2_COMMON_H */
