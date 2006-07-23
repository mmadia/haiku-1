/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#include "usb_p.h"


#define TRACE_BUSMANAGER
#ifdef TRACE_BUSMANAGER
#define TRACE(x)	dprintf x
#else
#define TRACE(x)	/* nothing */
#endif


BusManager::BusManager()
{
	fInitOK = false;
	fRootHub = NULL;

	// Set up the semaphore
	fLock = create_sem(1, "bus manager lock");
	if (fLock < B_OK)
		return;

	set_sem_owner(B_SYSTEM_TEAM, fLock);

	// Clear the device map
	for (int32 i = 0; i < 128; i++)
		fDeviceMap[i] = false;

	// Set up the default pipes
	fDefaultPipe = new ControlPipe(this, 0, Pipe::Default, Pipe::NormalSpeed,
		0, 8);
	fDefaultPipeLowSpeed = new ControlPipe(this, 0, Pipe::Default,
		Pipe::LowSpeed, 0, 8);

	fExploreThread = -1; 
	fInitOK = true;
}


BusManager::~BusManager()
{
}


status_t
BusManager::InitCheck()
{
	if (fInitOK)
		return B_OK;

	return B_ERROR;
}


/*
	This is the 'main' function of the explore thread, which keeps track of
	the hub states.
*/
int32
BusManager::ExploreThread(void *data)
{
	Hub *rootHub = (Hub *)data;
	if (!rootHub)
		return B_ERROR;

	snooze(3000000);
	while (true) {
		rootHub->Explore();
		snooze(1000000);
	}

	return B_OK;
}


Device *
BusManager::AllocateNewDevice(Device *parent, bool lowSpeed)
{
	// Check if there is a free entry in the device map (for the device number)
	int8 deviceAddress = AllocateAddress();
	if (deviceAddress < 0) {
		TRACE(("usb BusManager::AllocateNewDevice(): could not get a new address\n"));
		return NULL;
	}

	TRACE(("usb BusManager::AllocateNewDevice(): setting device address to %d\n", deviceAddress));

	ControlPipe *defaultPipe = (lowSpeed ? fDefaultPipeLowSpeed : fDefaultPipe);

	// Set the address of the device USB 1.1 spec p202
	status_t result = defaultPipe->SendRequest(
		USB_REQTYPE_DEVICE_OUT | USB_REQTYPE_STANDARD,		// type
		USB_REQUEST_SET_ADDRESS,							// request
		deviceAddress,										// value
		0,													// index
		0,													// length
		NULL,												// buffer
		0,													// buffer length
		NULL);												// actual length

	if (result < B_OK) {
		TRACE(("usb BusManager::AllocateNewDevice(): error while setting device address\n"));
		return NULL;
	}

	// Wait a bit for the device to complete addressing
	snooze(10000);

	// Create a temporary pipe with the new address
	ControlPipe pipe(this, deviceAddress, Pipe::Default,
		lowSpeed ? Pipe::LowSpeed : Pipe::NormalSpeed, 0, 8);

	// Get the device descriptor
	// Just retrieve the first 8 bytes of the descriptor -> minimum supported
	// size of any device. It is enough because it includes the device type.

	size_t actualLength = 0;
	usb_device_descriptor deviceDescriptor;

	TRACE(("usb BusManager::AllocateNewDevice(): getting the device descriptor\n"));
	pipe.SendRequest(
		USB_REQTYPE_DEVICE_IN | USB_REQTYPE_STANDARD,		// type
		USB_REQUEST_GET_DESCRIPTOR,							// request
		USB_DESCRIPTOR_DEVICE << 8,							// value
		0,													// index
		8,													// length										
		(void *)&deviceDescriptor,							// buffer
		8,													// buffer length
		&actualLength);										// actual length

	if (actualLength != 8) {
		TRACE(("usb BusManager::AllocateNewDevice(): error while getting the device descriptor\n"));
		return NULL;
	}

	TRACE(("short device descriptor for device %d:\n", deviceAddress));
	TRACE(("\tlength:..............%d\n", deviceDescriptor.length));
	TRACE(("\tdescriptor_type:.....0x%04x\n", deviceDescriptor.descriptor_type));
	TRACE(("\tusb_version:.........0x%04x\n", deviceDescriptor.usb_version));
	TRACE(("\tdevice_class:........0x%02x\n", deviceDescriptor.device_class));
	TRACE(("\tdevice_subclass:.....0x%02x\n", deviceDescriptor.device_subclass));
	TRACE(("\tdevice_protocol:.....0x%02x\n", deviceDescriptor.device_protocol));
	TRACE(("\tmax_packet_size_0:...%d\n", deviceDescriptor.max_packet_size_0));

	// Create a new instance based on the type (Hub or Device)
	if (deviceDescriptor.device_class == 0x09) {
		TRACE(("usb BusManager::AllocateNewDevice(): creating new hub\n"));
		Device *ret = new Hub(this, parent, deviceDescriptor, deviceAddress,
			lowSpeed);

		if (parent == NULL) {
			// root hub
			fRootHub = ret;
		}

		return ret;
	}

	TRACE(("usb BusManager::AllocateNewDevice(): creating new device\n"));
	return new Device(this, parent, deviceDescriptor, deviceAddress, lowSpeed);
}


int8
BusManager::AllocateAddress()
{
	acquire_sem_etc(fLock, 1, B_CAN_INTERRUPT, 0);

	int8 deviceAddress = -1;
	for (int32 i = 1; i < 128; i++) {
		if (fDeviceMap[i] == false) {
			deviceAddress = i;
			fDeviceMap[i] = true;
			break;
		}
	}

	release_sem(fLock);
	return deviceAddress;
}


status_t
BusManager::Start()
{
	TRACE(("usb BusManager::Start()\n"));
	if (InitCheck() != B_OK)
		return InitCheck();

	if (fExploreThread < 0) {
		fExploreThread = spawn_kernel_thread(ExploreThread,
			"usb busmanager explore", B_LOW_PRIORITY, (void *)fRootHub);
	}

	return resume_thread(fExploreThread);
}


status_t
BusManager::Stop()
{
	// ToDo: implement (should end the explore thread)
	return B_OK;
}


status_t
BusManager::SubmitTransfer(Transfer *transfer)
{
	// virtual function to be overridden
	return B_ERROR;
}


status_t
BusManager::SubmitRequest(Transfer *transfer)
{
	// virtual function to be overridden
	return B_ERROR;
}
