/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/

#include "util/kernel_cpp.h"

#ifdef _BOOT_MODE
#	include <boot/platform.h>
#elif defined(_KERNEL_MODE)
#	include <KernelExport.h>
#endif

// Always define the symbols needed when not linking against libgcc.a --
// we simply override them.

// ... it doesn't seem to work with this symbol at least.
#ifndef USING_LIBGCC
const nothrow_t std::nothrow = {};
#endif

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
#if defined(_KERNEL_MODE) && !defined(_BOOT_MODE)

#include <stdio.h>

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

#endif	// __GNUC__ >= 3

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
