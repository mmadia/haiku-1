/*
 * Copyright 2006-2007, Marcus Overhagen. All rights reserved.
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <string.h>
#include <KernelExport.h>
#define __HAIKU_PCI_BUS_MANAGER_TESTING 1
#include <PCI.h>

#include "util/kernel_cpp.h"
#include "pci_fixup.h"
#include "pci_priv.h"
#include "pci.h"

#define TRACE_CAP(x...) dprintf(x)
#define FLOW(x...)
//#define FLOW(x...) dprintf(x)

static PCI *sPCI;

// #pragma mark bus manager exports


status_t
pci_controller_add(pci_controller *controller, void *cookie)
{
	return sPCI->AddController(controller, cookie);
}


long
pci_get_nth_pci_info(long index, pci_info *outInfo)
{
	return sPCI->GetNthPciInfo(index, outInfo);
}


uint32
pci_read_config(uint8 virtualBus, uint8 device, uint8 function, uint8 offset, uint8 size)
{
	uint8 bus;
	int domain;
	uint32 value;
	
	if (sPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return 0xffffffff;
		
	if (sPCI->ReadPciConfig(domain, bus, device, function, offset, size, &value) != B_OK)
		return 0xffffffff;
		
	return value;
}


void
pci_write_config(uint8 virtualBus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32 value)
{
	uint8 bus;
	int domain;
	
	if (sPCI->ResolveVirtualBus(virtualBus, &domain, &bus) != B_OK)
		return;
		
	sPCI->WritePciConfig(domain, bus, device, function, offset, size, value);
}


status_t 
pci_find_capability(uchar bus, uchar device, uchar function, uchar cap_id, uchar *offset)
{
	uint16 status;
	uint8 header_type;
	uint8 cap_ptr;
	int i;

	if (!offset) {
		TRACE_CAP("PCI: find_pci_capability ERROR %u:%u:%u capability %#02x offset NULL pointer\n", bus, device, function, cap_id);
		return B_BAD_VALUE;
	}

	status = pci_read_config(bus, device, function, PCI_status, 2);
	if (!(status & PCI_status_capabilities)) {
		TRACE_CAP("PCI: find_pci_capability ERROR %u:%u:%u capability %#02x not supported\n", bus, device, function, cap_id);
		return B_ERROR;
	}

	header_type = pci_read_config(bus, device, function, PCI_header_type, 1);
	switch (header_type & PCI_header_type_mask) {
		case PCI_header_type_generic:
		case PCI_header_type_PCI_to_PCI_bridge:
			cap_ptr = pci_read_config(bus, device, function, PCI_capabilities_ptr, 1);
			break;
		case PCI_header_type_cardbus:
			cap_ptr = pci_read_config(bus, device, function, PCI_capabilities_ptr_2, 1);
			break;
		default:
			TRACE_CAP("PCI: find_pci_capability ERROR %u:%u:%u capability %#02x unknown header type\n", bus, device, function, cap_id);
			return B_ERROR;
	}

	cap_ptr &= ~3;
	if (!cap_ptr) {
		TRACE_CAP("PCI: find_pci_capability ERROR %u:%u:%u capability %#02x empty list\n", bus, device, function, cap_id);
		return B_NAME_NOT_FOUND;
	}

	for (i = 0; i < 48; i++) {
		uint8 this_cap_id = pci_read_config(bus, device, function, cap_ptr, 1);
		if (this_cap_id == cap_id) {
			*offset = cap_ptr;
			return B_OK;
		}

		cap_ptr = pci_read_config(bus, device, function, cap_ptr + 1, 1);
		cap_ptr &= ~3;

		if (!cap_ptr)
			return B_NAME_NOT_FOUND;
	}

	TRACE_CAP("PCI: find_pci_capability ERROR %u:%u:%u capability %#02x circular list\n", bus, device, function, cap_id);
	return B_ERROR;
}


// #pragma mark kernel debugger commands

static int
display_io(int argc, char **argv)
{
	int32 displayWidth;
	int32 itemSize;
	int32 num = 1;
	int address;
	int i = 1, j;

	switch (argc) {
	case 3:
		num = atoi(argv[2]);
	case 2:
		address = strtoul(argv[1], NULL, 0);
		break;
	default:
		kprintf("usage: %s <address> [num]\n", argv[0]);
		return 0;
	}

	// build the format string
	if (strcmp(argv[0], "inb") == 0 || strcmp(argv[0], "in8") == 0) {
		itemSize = 1;
		displayWidth = 16;
	} else if (strcmp(argv[0], "ins") == 0 || strcmp(argv[0], "in16") == 0) {
		itemSize = 2;
		displayWidth = 8;
	} else if (strcmp(argv[0], "inw") == 0 || strcmp(argv[0], "in32") == 0) {
		itemSize = 4;
		displayWidth = 4;
	} else {
		kprintf("display_io called in an invalid way!\n");
		return 0;
	}

	for (i = 0; i < num; i++) {
		if ((i % displayWidth) == 0) {
			int32 displayed = min_c(displayWidth, (num-i)) * itemSize;
			if (i != 0)
				kprintf("\n");

			kprintf("[0x%lx]  ", address + i * itemSize);

			if (num > displayWidth) {
				// make sure the spacing in the last line is correct
				for (j = displayed; j < displayWidth * itemSize; j++)
					kprintf(" ");
			}
			kprintf("  ");
		}

		switch (itemSize) {
			case 1:
				kprintf(" %02x", pci_read_io_8(address + i * itemSize));
				break;
			case 2:
				kprintf(" %04x", pci_read_io_16(address + i * itemSize));
				break;
			case 4:
				kprintf(" %08lx", pci_read_io_32(address + i * itemSize));
				break;
		}
	}

	kprintf("\n");
	return 0;
}


static int
write_io(int argc, char **argv)
{
	int32 itemSize;
	uint32 value;
	int address;
	int i = 1;

	if (argc < 3) {
		kprintf("usage: %s <address> <value> [value [...]]\n", argv[0]);
		return 0;
	}

	address = strtoul(argv[1], NULL, 0);

	if (strcmp(argv[0], "outb") == 0 || strcmp(argv[0], "out8") == 0) {
		itemSize = 1;
	} else if (strcmp(argv[0], "outs") == 0 || strcmp(argv[0], "out16") == 0) {
		itemSize = 2;
	} else if (strcmp(argv[0], "outw") == 0 || strcmp(argv[0], "out32") == 0) {
		itemSize = 4;
	} else {
		kprintf("write_io called in an invalid way!\n");
		return 0;
	}

	// skip cmd name and address
	argv += 2;
	argc -= 2;

	for (i = 0; i < argc; i++) {
		value = strtoul(argv[i], NULL, 0);
		switch (itemSize) {
			case 1:
				pci_write_io_8(address + i * itemSize, value);
				break;
			case 2:
				pci_write_io_16(address + i * itemSize, value);
				break;
			case 4:
				pci_write_io_32(address + i * itemSize, value);
				break;
		}
	}

	return 0;
}


static int
pcistatus(int argc, char **argv)
{
	sPCI->ClearDeviceStatus(NULL, true);
	return 0;
}

// #pragma mark bus manager init/uninit

status_t
pci_init(void)
{
	sPCI = new PCI;

	if (pci_io_init() != B_OK) {
		TRACE(("PCI: pci_io_init failed\n"));
		return B_ERROR;
	}

	add_debugger_command("inw", &display_io, "dump io words (32-bit)");
	add_debugger_command("in32", &display_io, "dump io words (32-bit)");
	add_debugger_command("ins", &display_io, "dump io shorts (16-bit)");
	add_debugger_command("in16", &display_io, "dump io shorts (16-bit)");
	add_debugger_command("inb", &display_io, "dump io bytes (8-bit)");
	add_debugger_command("in8", &display_io, "dump io bytes (8-bit)");

	add_debugger_command("outw", &write_io, "write io words (32-bit)");
	add_debugger_command("out32", &write_io, "write io words (32-bit)");
	add_debugger_command("outs", &write_io, "write io shorts (16-bit)");
	add_debugger_command("out16", &write_io, "write io shorts (16-bit)");
	add_debugger_command("outb", &write_io, "write io bytes (8-bit)");
	add_debugger_command("out8", &write_io, "write io bytes (8-bit)");

	if (pci_controller_init() != B_OK) {
		TRACE(("PCI: pci_controller_init failed\n"));
		return B_ERROR;
	}

	sPCI->InitDomainData();
	sPCI->InitBus();

	add_debugger_command("pcistatus", &pcistatus, "dump and clear pci device status registers");

	return B_OK;
}


void
pci_uninit(void)
{
	remove_debugger_command("outw", &write_io);
	remove_debugger_command("out32", &write_io);
	remove_debugger_command("outs", &write_io);
	remove_debugger_command("out16", &write_io);
	remove_debugger_command("outb", &write_io);
	remove_debugger_command("out8", &write_io);

	remove_debugger_command("inw", &display_io);
	remove_debugger_command("in32", &display_io);
	remove_debugger_command("ins", &display_io);
	remove_debugger_command("in16", &display_io);
	remove_debugger_command("inb", &display_io);
	remove_debugger_command("in8", &display_io);

	delete sPCI;
}


// #pragma mark PCI class

PCI::PCI()
 :	fRootBus(0)
 ,	fDomainCount(0)
 ,	fBusEnumeration(false)
 ,	fVirtualBusMap()
 ,	fNextVirtualBus(0)
{
	#if defined(__POWERPC__) || defined(__M68K__)
		fBusEnumeration = true;
	#endif
}


void
PCI::InitBus()
{
	PCIBus **ppnext = &fRootBus;
	for (int i = 0; i < fDomainCount; i++) {
		PCIBus *bus = new PCIBus;
		bus->next = NULL;
		bus->parent = NULL;
		bus->child = NULL;
		bus->domain = i;
		bus->bus = 0;
		*ppnext = bus;
		ppnext = &bus->next;
	}
	
	if (fBusEnumeration) {
		for (int i = 0; i < fDomainCount; i++) {
			EnumerateBus(i, 0);
		}
	}

	if (1) {
		for (int i = 0; i < fDomainCount; i++) {
			FixupDevices(i, 0);
		}
	}
	
	if (fRootBus) {
		DiscoverBus(fRootBus);
		ConfigureBridges(fRootBus);
		ClearDeviceStatus(fRootBus, false);
		RefreshDeviceInfo(fRootBus);
	}
}


PCI::~PCI()
{
}


status_t
PCI::CreateVirtualBus(int domain, uint8 bus, uint8 *virtualBus)
{
#if defined(__INTEL__)

	// IA32 doesn't use domains
	if (domain)
		panic("PCI::CreateVirtualBus domain != 0");
	*virtualBus = bus;
	return B_OK;

#else

	if (fNextVirtualBus > 0xff)
		panic("PCI::CreateVirtualBus: virtual bus number space exhausted");
	if (unsigned(domain) > 0xff)
		panic("PCI::CreateVirtualBus: domain %d too large", domain);

	uint16 value = domain << 8 | bus;

	for (VirtualBusMap::Iterator it = fVirtualBusMap.Begin(); it != fVirtualBusMap.End(); ++it) {
		if (it->Value() == value) {
			*virtualBus = it->Key();
			FLOW("PCI::CreateVirtualBus: domain %d, bus %d already in map => virtualBus %d\n", domain, bus, *virtualBus);
			return B_OK;
		}
	}

	*virtualBus = fNextVirtualBus++;

	FLOW("PCI::CreateVirtualBus: domain %d, bus %d => virtualBus %d\n", domain, bus, *virtualBus);
	
	return fVirtualBusMap.Insert(*virtualBus, value);

#endif
}


status_t
PCI::ResolveVirtualBus(uint8 virtualBus, int *domain, uint8 *bus)
{
#if defined(__INTEL__)

	// IA32 doesn't use domains
	*bus = virtualBus;
	*domain = 0;
	return B_OK;

#else

	if (virtualBus >= fNextVirtualBus)
		return B_ERROR;

	uint16 value = fVirtualBusMap.Get(virtualBus);
	*domain = value >> 8;
	*bus = value & 0xff;
	return B_OK;

#endif
}


// used by pci_info.cpp print_info_basic()
void
__pci_resolve_virtual_bus(uint8 virtualBus, int *domain, uint8 *bus)
{
	if (sPCI->ResolveVirtualBus(virtualBus, domain, bus) < B_OK)
		panic("ResolveVirtualBus failed");
}


status_t
PCI::AddController(pci_controller *controller, void *controller_cookie)
{
	if (fDomainCount == MAX_PCI_DOMAINS)
		return B_ERROR;
	
	fDomainData[fDomainCount].controller = controller;
	fDomainData[fDomainCount].controller_cookie = controller_cookie;

	// initialized later to avoid call back into controller at this point
	fDomainData[fDomainCount].max_bus_devices = -1;
	
	fDomainCount++;
	return B_OK;
}

void
PCI::InitDomainData()
{
	for (int i = 0; i < fDomainCount; i++) {
		int32 count;
		status_t status;
		
		status = (*fDomainData[i].controller->get_max_bus_devices)(fDomainData[i].controller_cookie, &count);
		fDomainData[i].max_bus_devices = (status == B_OK) ? count : 0;
	}
}


domain_data *
PCI::GetDomainData(int domain)
{
	if (domain < 0 || domain >= fDomainCount)
		return NULL;

	return &fDomainData[domain];
}


status_t
PCI::GetNthPciInfo(long index, pci_info *outInfo)
{
	long curindex = 0;
	if (!fRootBus)
		return B_ERROR;
	return GetNthPciInfo(fRootBus, &curindex, index, outInfo);
}


status_t
PCI::GetNthPciInfo(PCIBus *bus, long *curindex, long wantindex, pci_info *outInfo)
{
	// maps tree structure to linear indexed view
	PCIDev *dev = bus->child;
	while (dev) {
		if (*curindex == wantindex) {
			*outInfo = dev->info;
			return B_OK;
		}
		*curindex += 1;
		if (dev->child && B_OK == GetNthPciInfo(dev->child, curindex, wantindex, outInfo))
			return B_OK;
		dev = dev->next;
	}
	
	if (bus->next)
		return GetNthPciInfo(bus->next, curindex, wantindex, outInfo);
		
	return B_ERROR;
}

void
PCI::EnumerateBus(int domain, uint8 bus, uint8 *subordinate_bus)
{
	TRACE(("PCI: EnumerateBus: domain %u, bus %u\n", domain, bus));
	
	int max_bus_devices = GetDomainData(domain)->max_bus_devices;

	// step 1: disable all bridges on this bus
	for (int dev = 0; dev < max_bus_devices; dev++) {
		uint16 vendor_id = ReadPciConfig(domain, bus, dev, 0, PCI_vendor_id, 2);
		if (vendor_id == 0xffff)
			continue;
		
		uint8 type = ReadPciConfig(domain, bus, dev, 0, PCI_header_type, 1);
		int nfunc = (type & PCI_multifunction) ? 8 : 1;
		for (int func = 0; func < nfunc; func++) {
			uint16 device_id = ReadPciConfig(domain, bus, dev, func, PCI_device_id, 2);
			if (device_id == 0xffff)
				continue;

			uint8 base_class = ReadPciConfig(domain, bus, dev, func, PCI_class_base, 1);
			uint8 sub_class	 = ReadPciConfig(domain, bus, dev, func, PCI_class_sub, 1);
			if (base_class != PCI_bridge || sub_class != PCI_pci)
				continue;
			
			TRACE(("PCI: found PCI-PCI bridge: domain %u, bus %u, dev %u, func %u\n", domain, bus, dev, func));
			TRACE(("PCI: original settings: pcicmd %04lx, primary-bus %lu, secondary-bus %lu, subordinate-bus %lu\n",
				ReadPciConfig(domain, bus, dev, func, PCI_command, 2),
				ReadPciConfig(domain, bus, dev, func, PCI_primary_bus, 1),
				ReadPciConfig(domain, bus, dev, func, PCI_secondary_bus, 1),
				ReadPciConfig(domain, bus, dev, func, PCI_subordinate_bus, 1)));
			
			// disable decoding
			uint16 pcicmd;
			pcicmd = ReadPciConfig(domain, bus, dev, func, PCI_command, 2);
			pcicmd &= ~(PCI_command_io | PCI_command_memory | PCI_command_master);
			WritePciConfig(domain, bus, dev, func, PCI_command, 2, pcicmd);

			// disable busses
			WritePciConfig(domain, bus, dev, func, PCI_primary_bus, 1, 0);
			WritePciConfig(domain, bus, dev, func, PCI_secondary_bus, 1, 0);
			WritePciConfig(domain, bus, dev, func, PCI_subordinate_bus, 1, 0);

			TRACE(("PCI: disabled settings: pcicmd %04lx, primary-bus %lu, secondary-bus %lu, subordinate-bus %lu\n",
				ReadPciConfig(domain, bus, dev, func, PCI_command, 2),
				ReadPciConfig(domain, bus, dev, func, PCI_primary_bus, 1),
				ReadPciConfig(domain, bus, dev, func, PCI_secondary_bus, 1),
				ReadPciConfig(domain, bus, dev, func, PCI_subordinate_bus, 1)));
		}
	}
	
	uint8 last_used_bus_number = bus;
	
	// step 2: assign busses to all bridges, and enable them again
	for (int dev = 0; dev < max_bus_devices; dev++) {
		uint16 vendor_id = ReadPciConfig(domain, bus, dev, 0, PCI_vendor_id, 2);
		if (vendor_id == 0xffff)
			continue;
		
		uint8 type = ReadPciConfig(domain, bus, dev, 0, PCI_header_type, 1);
		int nfunc = (type & PCI_multifunction) ? 8 : 1;
		for (int func = 0; func < nfunc; func++) {
			uint16 device_id = ReadPciConfig(domain, bus, dev, func, PCI_device_id, 2);
			if (device_id == 0xffff)
				continue;

			uint8 base_class = ReadPciConfig(domain, bus, dev, func, PCI_class_base, 1);
			uint8 sub_class	 = ReadPciConfig(domain, bus, dev, func, PCI_class_sub, 1);
			if (base_class != PCI_bridge || sub_class != PCI_pci)
				continue;
			
			TRACE(("PCI: configuring PCI-PCI bridge: domain %u, bus %u, dev %u, func %u\n", 
				domain, bus, dev, func));
			
			// open Scheunentor for enumerating the bus behind the bridge
			WritePciConfig(domain, bus, dev, func, PCI_primary_bus, 1, bus);
			WritePciConfig(domain, bus, dev, func, PCI_secondary_bus, 1, last_used_bus_number + 1);
			WritePciConfig(domain, bus, dev, func, PCI_subordinate_bus, 1, 255);
			
			// enable decoding (too early here?)
			uint16 pcicmd;
			pcicmd = ReadPciConfig(domain, bus, dev, func, PCI_command, 2);
			pcicmd |= PCI_command_io | PCI_command_memory | PCI_command_master;
			WritePciConfig(domain, bus, dev, func, PCI_command, 2, pcicmd);

			TRACE(("PCI: probing settings: pcicmd %04lx, primary-bus %lu, secondary-bus %lu, subordinate-bus %lu\n",
				ReadPciConfig(domain, bus, dev, func, PCI_command, 2),
				ReadPciConfig(domain, bus, dev, func, PCI_primary_bus, 1),
				ReadPciConfig(domain, bus, dev, func, PCI_secondary_bus, 1),
				ReadPciConfig(domain, bus, dev, func, PCI_subordinate_bus, 1)));
			
			// enumerate bus
			EnumerateBus(domain, last_used_bus_number + 1, &last_used_bus_number);

			// close Scheunentor
			WritePciConfig(domain, bus, dev, func, PCI_subordinate_bus, 1, last_used_bus_number);
			
			TRACE(("PCI: configured settings: pcicmd %04lx, primary-bus %lu, secondary-bus %lu, subordinate-bus %lu\n",
				ReadPciConfig(domain, bus, dev, func, PCI_command, 2),
				ReadPciConfig(domain, bus, dev, func, PCI_primary_bus, 1),
				ReadPciConfig(domain, bus, dev, func, PCI_secondary_bus, 1),
				ReadPciConfig(domain, bus, dev, func, PCI_subordinate_bus, 1)));
			}
	}
	if (subordinate_bus)
		*subordinate_bus = last_used_bus_number;

	TRACE(("PCI: EnumerateBus done: domain %u, bus %u, last used bus number %u\n", domain, bus, last_used_bus_number));
}


void
PCI::FixupDevices(int domain, uint8 bus)
{
	FLOW("PCI: FixupDevices domain %u, bus %u\n", domain, bus);

	int maxBusDevices = GetDomainData(domain)->max_bus_devices;

	for (int dev = 0; dev < maxBusDevices; dev++) {
		uint16 vendorId = ReadPciConfig(domain, bus, dev, 0, PCI_vendor_id, 2);
		if (vendorId == 0xffff)
			continue;
		
		uint8 type = ReadPciConfig(domain, bus, dev, 0, PCI_header_type, 1);
		int nfunc = (type & PCI_multifunction) ? 8 : 1;
		for (int func = 0; func < nfunc; func++) {
			uint16 deviceId = ReadPciConfig(domain, bus, dev, func, PCI_device_id, 2);
			if (deviceId == 0xffff)
				continue;

			pci_fixup_device(this, domain, bus, dev, func);

			uint8 base_class = ReadPciConfig(domain, bus, dev, func, PCI_class_base, 1);
			if (base_class != PCI_bridge)
				continue;
			uint8 sub_class	 = ReadPciConfig(domain, bus, dev, func, PCI_class_sub, 1);
			if (sub_class != PCI_pci)
				continue;

			int busBehindBridge = ReadPciConfig(domain, bus, dev, func, PCI_secondary_bus, 1);
			
			FixupDevices(domain, busBehindBridge);
		}
	}
}


void
PCI::ConfigureBridges(PCIBus *bus)
{
	for (PCIDev *dev = bus->child; dev; dev = dev->next) {
		if (dev->info.class_base == PCI_bridge && dev->info.class_sub == PCI_pci) {
			uint16 bridgeControlOld = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_bridge_control, 2);
			uint16 bridgeControlNew = bridgeControlOld;
			// Enable: Parity Error Response, SERR, Master Abort Mode, Discard Timer SERR
			// Clear: Discard Timer Status
			bridgeControlNew |= (1 << 0) | (1 << 1) | (1 << 5) | (1 << 10) | (1 << 11);
			// Set discard timer to 2^15 PCI clocks
			bridgeControlNew &= ~((1 << 8) | (1 << 9));
			WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_bridge_control, 2, bridgeControlNew);
			bridgeControlNew = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_bridge_control, 2);
			dprintf("PCI: dom %u, bus %u, dev %2u, func %u, changed PCI bridge control from 0x%04x to 0x%04x\n",
				dev->domain, dev->bus, dev->dev, dev->func, bridgeControlOld, bridgeControlNew);
		}

		if (dev->child)
			ConfigureBridges(dev->child);
	}
	
	if (bus->next)
		ConfigureBridges(bus->next);
}


void
PCI::ClearDeviceStatus(PCIBus *bus, bool dumpStatus)
{
	if (!bus) {
		if (!fRootBus)
			return;
		bus = fRootBus;
	}

	for (PCIDev *dev = bus->child; dev; dev = dev->next) {

		// Clear and dump PCI device status
		uint16 status = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_status, 2);
		WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_status, 2, status);
		if (dumpStatus) {
			kprintf("domain %u, bus %u, dev %2u, func %u, PCI device status 0x%04x\n",
				dev->domain, dev->bus, dev->dev, dev->func, status);
			if (status & (1 << 15))
				kprintf("  Detected Parity Error\n");
			if (status & (1 << 14))
				kprintf("  Signalled System Error\n");
			if (status & (1 << 13))
				kprintf("  Received Master-Abort\n");
			if (status & (1 << 12))
				kprintf("  Received Target-Abort\n");
			if (status & (1 << 11))
				kprintf("  Signalled Target-Abort\n");
			if (status & (1 << 8))
				kprintf("  Master Data Parity Error\n");
		}

		if (dev->info.class_base == PCI_bridge && dev->info.class_sub == PCI_pci) {
	
			// Clear and dump PCI bridge secondary status
			uint16 secondaryStatus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_status, 2);
			WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_status, 2, secondaryStatus);
			if (dumpStatus) {				
				kprintf("domain %u, bus %u, dev %2u, func %u, PCI bridge secondary status 0x%04x\n", 
					dev->domain, dev->bus, dev->dev, dev->func, secondaryStatus);
				if (secondaryStatus & (1 << 15))
					kprintf("  Detected Parity Error\n");
				if (secondaryStatus & (1 << 14))
					kprintf("  Received System Error\n");
				if (secondaryStatus & (1 << 13))
					kprintf("  Received Master-Abort\n");
				if (secondaryStatus & (1 << 12))
					kprintf("  Received Target-Abort\n");
				if (secondaryStatus & (1 << 11))
					kprintf("  Signalled Target-Abort\n");
				if (secondaryStatus & (1 << 8))
					kprintf("  Data Parity Reported\n");
			}

			// Clear and dump the discard-timer error bit located in bridge-control register
			uint16 bridgeControl = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_bridge_control, 2);
			WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_bridge_control, 2, bridgeControl);
			if (dumpStatus) {
				kprintf("domain %u, bus %u, dev %2u, func %u, PCI bridge control 0x%04x\n", 
					dev->domain, dev->bus, dev->dev, dev->func, bridgeControl);
				if (bridgeControl & (1 << 10)) {
					kprintf("  bridge-control: Discard Timer Error\n");
				}
			}
		}

		if (dev->child)
			ClearDeviceStatus(dev->child, dumpStatus);
	}
	
	if (bus->next)
		ClearDeviceStatus(bus->next, dumpStatus);
}


void
PCI::DiscoverBus(PCIBus *bus)
{
	FLOW("PCI: DiscoverBus, domain %u, bus %u\n", bus->domain, bus->bus);
	
	int max_bus_devices = GetDomainData(bus->domain)->max_bus_devices;

	for (int dev = 0; dev < max_bus_devices; dev++) {
		uint16 vendor_id = ReadPciConfig(bus->domain, bus->bus, dev, 0, PCI_vendor_id, 2);
		if (vendor_id == 0xffff)
			continue;
		
		uint8 type = ReadPciConfig(bus->domain, bus->bus, dev, 0, PCI_header_type, 1);
		int nfunc = (type & PCI_multifunction) ? 8 : 1;
		for (int func = 0; func < nfunc; func++)
			DiscoverDevice(bus, dev, func);
	}
	
	if (bus->next)
		DiscoverBus(bus->next);
}


void
PCI::DiscoverDevice(PCIBus *bus, uint8 dev, uint8 func)
{
	FLOW("PCI: DiscoverDevice, domain %u, bus %u, dev %u, func %u\n", bus->domain, bus->bus, dev, func);

	uint16 device_id = ReadPciConfig(bus->domain, bus->bus, dev, func, PCI_device_id, 2);
	if (device_id == 0xffff)
		return;

	PCIDev *newdev = CreateDevice(bus, dev, func);

	uint8 base_class = ReadPciConfig(bus->domain, bus->bus, dev, func, PCI_class_base, 1);
	uint8 sub_class	 = ReadPciConfig(bus->domain, bus->bus, dev, func, PCI_class_sub, 1);
	if (base_class == PCI_bridge && sub_class == PCI_pci) {
		uint8 secondary_bus = ReadPciConfig(bus->domain, bus->bus, dev, func, PCI_secondary_bus, 1);
		PCIBus *newbus = CreateBus(newdev, bus->domain, secondary_bus);
		DiscoverBus(newbus);
	}
}


PCIBus *
PCI::CreateBus(PCIDev *parent, int domain, uint8 bus)
{
	PCIBus *newbus = new PCIBus;
	newbus->next = NULL;
	newbus->parent = parent;
	newbus->child = NULL;
	newbus->domain = domain;
	newbus->bus = bus;
	
	// append
	parent->child = newbus;
	
	return newbus;
}

	
PCIDev *
PCI::CreateDevice(PCIBus *parent, uint8 dev, uint8 func)
{
	FLOW("PCI: CreateDevice, domain %u, bus %u, dev %u, func %u:\n", parent->domain, parent->bus, dev, func);
	
	PCIDev *newdev = new PCIDev;
	newdev->next = NULL;
	newdev->parent = parent;
	newdev->child = NULL;
	newdev->domain = parent->domain;
	newdev->bus = parent->bus;
	newdev->dev = dev;
	newdev->func = func;

	ReadPciBasicInfo(newdev);

	FLOW("PCI: CreateDevice, vendor 0x%04x, device 0x%04x, class_base 0x%02x, class_sub 0x%02x\n",
		newdev->info.vendor_id, newdev->info.device_id, newdev->info.class_base, newdev->info.class_sub);

	// append
	if (parent->child == 0) {
		parent->child = newdev;
	} else {
		PCIDev *sub = parent->child;
		while (sub->next)
			sub = sub->next;
		sub->next = newdev;
	}
	
	return newdev;
}


uint32
PCI::BarSize(uint32 bits, uint32 mask)
{
	bits &= mask;
	if (!bits)
		return 0;
	uint32 size = 1;
	while (!(bits & size))
		size <<= 1;
	return size;
}


void
PCI::GetBarInfo(PCIDev *dev, uint8 offset, uint32 *address, uint32 *size, uint8 *flags)
{
	uint32 oldvalue = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4);
	WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4, 0xffffffff);
	uint32 newvalue = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4);
	WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4, oldvalue);
	
	*address = oldvalue & PCI_address_memory_32_mask;
	if (size)
		*size = BarSize(newvalue, PCI_address_memory_32_mask);
	if (flags)
		*flags = newvalue & 0xf;
}


void
PCI::GetRomBarInfo(PCIDev *dev, uint8 offset, uint32 *address, uint32 *size, uint8 *flags)
{
	uint32 oldvalue = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4);
	WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4, 0xfffffffe); // LSB must be 0
	uint32 newvalue = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4);
	WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4, oldvalue);
	
	*address = oldvalue & PCI_rom_address_mask;
	if (size)
		*size = BarSize(newvalue, PCI_rom_address_mask);
	if (flags)
		*flags = newvalue & 0xf;
}


void
PCI::ReadPciBasicInfo(PCIDev *dev)
{
	uint8 virtualBus;
	
	if (CreateVirtualBus(dev->domain, dev->bus, &virtualBus) != B_OK) {
		dprintf("PCI: CreateVirtualBus failed, domain %u, bus %u\n", dev->domain, dev->bus);
		return;
	}
	
	dev->info.vendor_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_vendor_id, 2);
	dev->info.device_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_device_id, 2);
	dev->info.bus = virtualBus;
	dev->info.device = dev->dev;
	dev->info.function = dev->func;
	dev->info.revision = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_revision, 1);
	dev->info.class_api = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_class_api, 1);
	dev->info.class_sub = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_class_sub, 1);
	dev->info.class_base = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_class_base, 1);
	dev->info.line_size = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_line_size, 1);
	dev->info.latency = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_latency, 1);
	// BeOS does not mask off the multifunction bit, developer must use (header_type & PCI_header_type_mask)
	dev->info.header_type = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_header_type, 1);
	dev->info.bist = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_bist, 1);
	dev->info.reserved = 0;
}


void
PCI::ReadPciHeaderInfo(PCIDev *dev)
{
	switch (dev->info.header_type & PCI_header_type_mask) {
		case PCI_header_type_generic:
		{
			// disable PCI device address decoding (io and memory) while BARs are modified
			uint16 pcicmd = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_command, 2);
			WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd & ~(PCI_command_io | PCI_command_memory));

			// get BAR size infos			
			GetRomBarInfo(dev, PCI_rom_base, &dev->info.u.h0.rom_base_pci, &dev->info.u.h0.rom_size);
			for (int i = 0; i < 6; i++) {
				GetBarInfo(dev, PCI_base_registers + 4*i,
					&dev->info.u.h0.base_registers_pci[i],
					&dev->info.u.h0.base_register_sizes[i],
					&dev->info.u.h0.base_register_flags[i]);
			}
			
			// restore PCI device address decoding
			WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd);

			dev->info.u.h0.rom_base = (ulong)pci_ram_address((void *)dev->info.u.h0.rom_base_pci);
			for (int i = 0; i < 6; i++) {
				dev->info.u.h0.base_registers[i] = (ulong)pci_ram_address((void *)dev->info.u.h0.base_registers_pci[i]);
			}
			
			dev->info.u.h0.cardbus_cis = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_cardbus_cis, 4);
			dev->info.u.h0.subsystem_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_subsystem_id, 2);
			dev->info.u.h0.subsystem_vendor_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_subsystem_vendor_id, 2);
			dev->info.u.h0.interrupt_line = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_interrupt_line, 1);
			dev->info.u.h0.interrupt_pin = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_interrupt_pin, 1);
			dev->info.u.h0.min_grant = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_min_grant, 1);
			dev->info.u.h0.max_latency = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_max_latency, 1);
			break;
		}

		case PCI_header_type_PCI_to_PCI_bridge:
		{
			// disable PCI device address decoding (io and memory) while BARs are modified
			uint16 pcicmd = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_command, 2);
			WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd & ~(PCI_command_io | PCI_command_memory));

			GetRomBarInfo(dev, PCI_bridge_rom_base, &dev->info.u.h1.rom_base_pci);
			for (int i = 0; i < 2; i++) {
				GetBarInfo(dev, PCI_base_registers + 4*i,
					&dev->info.u.h1.base_registers_pci[i],
					&dev->info.u.h1.base_register_sizes[i],
					&dev->info.u.h1.base_register_flags[i]);
			}

			// restore PCI device address decoding
			WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd);

			dev->info.u.h1.rom_base = (ulong)pci_ram_address((void *)dev->info.u.h1.rom_base_pci);
			for (int i = 0; i < 2; i++) {
				dev->info.u.h1.base_registers[i] = (ulong)pci_ram_address((void *)dev->info.u.h1.base_registers_pci[i]);
			}

			dev->info.u.h1.primary_bus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_primary_bus, 1);
			dev->info.u.h1.secondary_bus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_bus, 1);
			dev->info.u.h1.subordinate_bus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_subordinate_bus, 1);
			dev->info.u.h1.secondary_latency = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_latency, 1);
			dev->info.u.h1.io_base = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_base, 1);
			dev->info.u.h1.io_limit = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_limit, 1);
			dev->info.u.h1.secondary_status = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_status, 2);
			dev->info.u.h1.memory_base = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_memory_base, 2);
			dev->info.u.h1.memory_limit = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_memory_limit, 2);
			dev->info.u.h1.prefetchable_memory_base = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_base, 2);
			dev->info.u.h1.prefetchable_memory_limit = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_limit, 2);
			dev->info.u.h1.prefetchable_memory_base_upper32 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_base_upper32, 4);
			dev->info.u.h1.prefetchable_memory_limit_upper32 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_limit_upper32, 4);
			dev->info.u.h1.io_base_upper16 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_base_upper16, 2);
			dev->info.u.h1.io_limit_upper16 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_limit_upper16, 2);
			dev->info.u.h1.interrupt_line = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_interrupt_line, 1);
			dev->info.u.h1.interrupt_pin = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_interrupt_pin, 1);
			dev->info.u.h1.bridge_control = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_bridge_control, 2);	
			dev->info.u.h1.subsystem_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_sub_device_id_1, 2);
			dev->info.u.h1.subsystem_vendor_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_sub_vendor_id_1, 2);
			break;
		}
		
		case PCI_header_type_cardbus:
		{
			// for testing only, not final:
			dev->info.u.h2.subsystem_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_sub_device_id_2, 2);
			dev->info.u.h2.subsystem_vendor_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_sub_vendor_id_2, 2);
			dev->info.u.h2.primary_bus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_primary_bus_2, 1);
			dev->info.u.h2.secondary_bus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_bus_2, 1);
			dev->info.u.h2.subordinate_bus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_subordinate_bus_2, 1);
			dev->info.u.h2.secondary_latency = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_latency_2, 1);
			dev->info.u.h2.reserved = 0;
			dev->info.u.h2.memory_base = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_memory_base0_2, 4);
			dev->info.u.h2.memory_limit = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_memory_limit0_2, 4);
			dev->info.u.h2.memory_base_upper32 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_memory_base1_2, 4);
			dev->info.u.h2.memory_limit_upper32 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_memory_limit1_2, 4);
			dev->info.u.h2.io_base = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_base0_2, 4);
			dev->info.u.h2.io_limit = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_limit0_2, 4);
			dev->info.u.h2.io_base_upper32 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_base1_2, 4);
			dev->info.u.h2.io_limit_upper32 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_limit1_2, 4);
			dev->info.u.h2.secondary_status = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_status_2, 2);
			dev->info.u.h2.bridge_control = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_bridge_control_2, 2);
			break;
		}

		default:
			TRACE(("PCI: Header type unknown (0x%02x)\n", dev->info.header_type));
			break;
	}
}


void
PCI::RefreshDeviceInfo(PCIBus *bus)
{
	for (PCIDev *dev = bus->child; dev; dev = dev->next) {
		ReadPciBasicInfo(dev);
		ReadPciHeaderInfo(dev);
		if (dev->child)
			RefreshDeviceInfo(dev->child);
	}
	
	if (bus->next)
		RefreshDeviceInfo(bus->next);
}


status_t
PCI::ReadPciConfig(int domain, uint8 bus, uint8 device, uint8 function,
				   uint8 offset, uint8 size, uint32 *value)
{
	domain_data *info;
	info = GetDomainData(domain);
	if (!info)
		return B_ERROR;
	
	if (device > (info->max_bus_devices - 1) 
		|| function > 7 
		|| (size != 1 && size != 2 && size != 4)
		|| (size == 2 && (offset & 3) == 3) 
		|| (size == 4 && (offset & 3) != 0)) {
		dprintf("PCI: can't read config for domain %d, bus %u, device %u, function %u, offset %u, size %u\n",
			 domain, bus, device, function, offset, size);
		return B_ERROR;
	}

	status_t status;
	status = (*info->controller->read_pci_config)(info->controller_cookie,
												  bus, device, function,
												  offset, size, value);
	return status;
}


uint32
PCI::ReadPciConfig(int domain, uint8 bus, uint8 device, uint8 function,
				   uint8 offset, uint8 size)
{
	uint32 value;
	
	if (ReadPciConfig(domain, bus, device, function, offset, size, &value) != B_OK)
		return 0xffffffff;
		
	return value;
}


status_t
PCI::WritePciConfig(int domain, uint8 bus, uint8 device, uint8 function,
					uint8 offset, uint8 size, uint32 value)
{
	domain_data *info;
	info = GetDomainData(domain);
	if (!info)
		return B_ERROR;
	
	if (device > (info->max_bus_devices - 1) 
		|| function > 7 
		|| (size != 1 && size != 2 && size != 4)
		|| (size == 2 && (offset & 3) == 3) 
		|| (size == 4 && (offset & 3) != 0)) {
		dprintf("PCI: can't write config for domain %d, bus %u, device %u, function %u, offset %u, size %u\n",
			 domain, bus, device, function, offset, size);
		return B_ERROR;
	}

	status_t status;
	status = (*info->controller->write_pci_config)(info->controller_cookie,
												   bus, device, function,
												   offset, size, value);
	return status;
}

