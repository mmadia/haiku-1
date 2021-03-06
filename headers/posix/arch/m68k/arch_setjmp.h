/*
 * Copyright 2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_SETJMP_H_
#define _ARCH_SETJMP_H_

#define _SETJMP_BUF_SZ (7+6+2+8*((96/8)/4))
typedef int __jmp_buf[_SETJMP_BUF_SZ];
#warning M68K: fix jmpbuf size

#endif	/* _ARCH_SETJMP_H_ */
