/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 *
 * Some of this code is based on previous work by Marcus Overhagen.
 */
#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <stdint.h>
#include <stdio.h>

#include <KernelExport.h>
#include <drivers/PCI.h>

#include <net_stack.h>

#include <compat/sys/kernel.h>
#include <compat/net/if.h>
#include <compat/net/if_var.h>

struct ifnet;

struct device {
	struct device * parent;
	char			dev_name[128];

	driver_t		*driver;

	int32			flags;

	int				unit;
	char			nameunit[64];
	const char *	description;
	void *			softc;

	struct {
		int (*probe)(device_t dev);
		int (*attach)(device_t dev);
		int (*detach)(device_t dev);
		int (*suspend)(device_t dev);
		int (*resume)(device_t dev);
		void (*shutdown)(device_t dev);

		int (*miibus_readreg)(device_t, int, int);
		int (*miibus_writereg)(device_t, int, int, int);
		void (*miibus_statchg)(device_t);
	} methods;
};


struct network_device {
	struct device base;

	pci_info		pci_info;

	int32			open;

	struct ifqueue	receive_queue;
	sem_id			receive_sem;

	sem_id			link_state_sem;

	struct ifnet *	ifp;
};


#define DEVNET(dev)		((device_t)(&(dev)->base))
#define NETDEV(base)	((struct network_device *)(base))


enum {
	DEVICE_OPEN			= 1 << 0,
	DEVICE_CLOSED		= 1 << 1,
	DEVICE_NON_BLOCK	= 1 << 2,
	DEVICE_DESC_ALLOCED	= 1 << 3,
};


static inline void
__unimplemented(const char *method)
{
	char msg[128];
	snprintf(msg, sizeof(msg), "fbsd compat, unimplemented: %s", method);
	panic(msg);
}


#define UNIMPLEMENTED() __unimplemented(__FUNCTION__)

status_t init_mbufs(void);
void uninit_mbufs(void);

status_t init_mutexes(void);
void uninit_mutexes(void);

status_t init_compat_layer(void);

status_t init_taskqueues(void);
void uninit_taskqueues(void);

/* busdma_machdep.c */
void init_bounce_pages(void);
void uninit_bounce_pages(void);

void driver_printf(const char *format, ...)
	__attribute__ ((format (__printf__, 1, 2)));
void driver_vprintf(const char *format, va_list vl);

void device_sprintf_name(device_t dev, const char *format, ...)
	__attribute__ ((format (__printf__, 2, 3)));

void ifq_init(struct ifqueue *ifq, const char *name);
void ifq_uninit(struct ifqueue *ifq);

extern struct net_stack_module_info *gStack;
extern pci_module_info *gPci;

extern const char *gDevNameList[];
extern struct network_device *gDevices[];

#endif
