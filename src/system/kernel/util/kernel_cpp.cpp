/*
 * Copyright 2003-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Ingo Weinhold, bonefish@users.sf.net.
 */

//!	C++ in the kernel


#include "util/kernel_cpp.h"

#ifdef _BOOT_MODE
#	include <boot/platform.h>
#else
#	include <KernelExport.h>
#	include <stdio.h>
#endif

#ifdef _LOADER_MODE
#	define panic printf
#	define dprintf printf
#	define kernel_debugger printf
#endif


// Always define the symbols needed when not linking against libgcc.a --
// we simply override them.

// ... it doesn't seem to work with this symbol at least.
#ifndef USING_LIBGCC
#	if __GNUC__ >= 3
const std::nothrow_t std::nothrow = {};
#	else
const nothrow_t std::nothrow = {};
#	endif
#endif

const mynothrow_t mynothrow = {};

#if __GNUC__ == 2

extern "C" void
__pure_virtual()
{
	panic("pure virtual function call\n");
}

#elif __GNUC__ >= 3

extern "C" void
__cxa_pure_virtual()
{
	panic("pure virtual function call\n");
}

#endif

// full C++ support in the kernel
#if (defined(_KERNEL_MODE) || defined(_LOADER_MODE)) && !defined(_BOOT_MODE)

FILE *stderr = NULL;

extern "C"
int
fprintf(FILE *f, const char *format, ...)
{
	// TODO: Introduce a vdprintf()...
	dprintf("fprintf(`%s',...)\n", format);
	return 0;
}

#if __GNUC__ >= 3

extern "C"
size_t
fwrite(const void *buffer, size_t size, size_t numItems, FILE *stream)
{
	dprintf("%.*s", int(size * numItems), (char*)buffer);
	return 0;
}

extern "C"
int
fputs(const char *string, FILE *stream)
{
	dprintf("%s", string);
	return 0;
}

extern "C"
int
fputc(int c, FILE *stream)
{
	dprintf("%c", c);
	return 0;
}

#ifndef _LOADER_MODE
extern "C"
int
printf(const char *format, ...)
{
	// TODO: Introduce a vdprintf()...
	dprintf("printf(`%s',...)\n", format);
	return 0;
}
#endif

extern "C"
int
puts(const char *string)
{
	return fputs(string, NULL);
}


#endif	// __GNUC__ >= 3

#if __GNUC__ >= 4 && !defined(USING_LIBGCC)

extern "C"
void
_Unwind_DeleteException()
{
	panic("_Unwind_DeleteException");
}

extern "C"
void
_Unwind_Find_FDE()
{
	panic("_Unwind_Find_FDE");
}


extern "C"
void
_Unwind_GetDataRelBase()
{
	panic("_Unwind_GetDataRelBase");
}

extern "C"
void
_Unwind_GetGR()
{
	panic("_Unwind_GetGR");
}

extern "C"
void
_Unwind_GetIP()
{
	panic("_Unwind_GetIP");
}

extern "C"
void
_Unwind_GetLanguageSpecificData()
{
	panic("_Unwind_GetLanguageSpecificData");
}

extern "C"
void
_Unwind_GetRegionStart()
{
	panic("_Unwind_GetRegionStart");
}

extern "C"
void
_Unwind_GetTextRelBase()
{
	panic("_Unwind_GetTextRelBase");
}

extern "C"
void
_Unwind_RaiseException()
{
	panic("_Unwind_RaiseException");
}

extern "C"
void
_Unwind_Resume()
{
	panic("_Unwind_Resume");
}

extern "C"
void
_Unwind_Resume_or_Rethrow()
{
	panic("_Unwind_Resume_or_Rethrow");
}

extern "C"
void
_Unwind_SetGR()
{
	panic("_Unwind_SetGR");
}

extern "C"
void
_Unwind_SetIP()
{
	panic("_Unwind_SetIP");
}

extern "C"
void
__deregister_frame_info()
{
	panic("__deregister_frame_info");
}

extern "C"
void
__register_frame_info()
{
	panic("__register_frame_info");
}


#endif	// __GNUC__ >= 4

extern "C"
void
abort()
{
	panic("abort() called!");
}

extern "C"
void
debugger(const char *message)
{
	kernel_debugger(message);
}

#endif	// _#if KERNEL_MODE
