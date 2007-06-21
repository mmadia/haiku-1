/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_DEPENDENCIES_H
#define _SYSTEM_DEPENDENCIES_H


#ifdef BFS_SHELL

#include "fssh_api_wrapper.h"

#else	// !BFS_SHELL

#include <util/DoublyLinkedList.h>
#include <util/kernel_cpp.h>
#include <util/Stack.h>

#include <ByteOrder.h>

#ifndef _BOOT_MODE
#	include <driver_settings.h>
#	include <fs_attr.h>
#	include <fs_cache.h>
#	include <fs_index.h>
#	include <fs_info.h>
#	include <fs_interface.h>
#	include <fs_query.h>
#	include <fs_volume.h>
#	include <Drivers.h>
#	include <KernelExport.h>
#	include <NodeMonitor.h>
#	include <SupportDefs.h>
#	include <TypeConstants.h>
#endif	// _BOOT_MODE

#include <ctype.h>
#include <errno.h>
#include <null.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#endif	// !BFS_SHELL


#endif	// _SYSTEM_DEPENDENCIES_H
