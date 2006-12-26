/*
 * Copyright 2006, Marcus Overhagen <marcus@overhagen.de. All rights reserved.
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Distributed under the terms of the MIT License.
 */

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>
#include <KernelExport.h>

#include <boot/net/Ethernet.h>
#include <boot/net/NetStack.h>

#include "pxe_undi.h"

#define TRACE_NETWORK
#ifdef TRACE_NETWORK
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif

#ifdef TRACE_NETWORK

static void
hex_dump(const void *_data, int length)
{
	uint8 *data = (uint8*)_data;
	for (int i = 0; i < length; i++) {
		if (i % 4 == 0) {
			if (i % 32 == 0) {
				if (i != 0)
					TRACE("\n");
				TRACE("%03x: ", i);
			} else
				TRACE(" ");
		}

		TRACE("%02x", data[i]);
	}
	TRACE("\n");
}

#else	// !TRACE_NETWORK

#define hex_dump(data, length)

#endif	// !TRACE_NETWORK


class UNDI : public EthernetInterface
{
public:
						UNDI();
	virtual 			~UNDI();

	status_t 			Init();

	virtual mac_addr_t	MACAddress() const;

	virtual	void *		AllocateSendReceiveBuffer(size_t size);
	virtual	void 		FreeSendReceiveBuffer(void *buffer);

	virtual ssize_t		Send(const void *buffer, size_t size);
	virtual ssize_t		Receive(void *buffer, size_t size);

private:
	mac_addr_t			fMACAddress;
	bool				fRxFinished;
	PXE_STRUCT *		fPxeData;
};


UNDI::UNDI()
 :	fMACAddress()
 ,	fRxFinished(true)
 ,	fPxeData(NULL)
{
	TRACE("UNDI::UNDI\n");

	fPxeData = pxe_undi_find_data();
	if (!fPxeData)
		panic("can't find !PXE structure");

	TRACE("PXE API entrypoint at %04x:%04x\n", fPxeData->EntryPointSP.seg, fPxeData->EntryPointSP.ofs);
}


UNDI::~UNDI()
{
	TRACE("UNDI::~UNDI\n");
}


status_t
UNDI::Init()
{
	TRACE("UNDI::Init\n");

	PXENV_UNDI_GET_INFORMATION get_info;
	PXENV_UNDI_GET_STATE get_state;
	PXENV_UNDI_OPEN undi_open;
	uint16 res;

	undi_open.OpenFlag = 0;
	undi_open.PktFilter = FLTR_DIRECTED | FLTR_BRDCST | FLTR_PRMSCS;
	undi_open.R_Mcast_Buf.MCastAddrCount = 0;

	res = call_pxe_bios(fPxeData, UNDI_OPEN, &undi_open);
	if (res != 0 || get_state.Status != 0) {
		TRACE("PXENV_UNDI_OPEN failed, res %x, status %x\n", res, undi_open.Status);
	}

	res = call_pxe_bios(fPxeData, UNDI_GET_STATE, &get_state);
	if (res != 0 || get_state.Status != 0) {
		TRACE("PXENV_UNDI_GET_STATE failed, res %x, status %x\n", res, get_state.Status);
	} else {
		switch (get_state.UNDIstate) {
			case PXE_UNDI_GET_STATE_STARTED: 
				TRACE("PXE_UNDI_GET_STATE_STARTED\n");
				break;
			case PXE_UNDI_GET_STATE_INITIALIZED: 
				TRACE("PXE_UNDI_GET_STATE_INITIALIZED\n");
				break;
			case PXE_UNDI_GET_STATE_OPENED: 
				TRACE("PXE_UNDI_GET_STATE_OPENED\n");
				break;
			default: 
				TRACE("unknown undi state 0x%02x\n", get_state.UNDIstate);
				break;
		}
	}

	res = call_pxe_bios(fPxeData, UNDI_GET_INFORMATION, &get_info);
	if (res != 0 || get_info.Status != 0) {
		TRACE("PXENV_UNDI_GET_INFORMATION failed, res %x, status %x\n", res, get_info.Status);
		return B_ERROR;
	}

	TRACE("Status = %x\n", get_info.Status);
	TRACE("BaseIo = %x\n", get_info.BaseIo);
	TRACE("IntNumber = %x\n", get_info.IntNumber);
	TRACE("MaxTranUnit = %x\n", get_info.MaxTranUnit);
	TRACE("HwType = %x\n", get_info.HwType);
	TRACE("HwAddrLen = %x\n", get_info.HwAddrLen);
	TRACE("RxBufCt = %x\n", get_info.RxBufCt);
	TRACE("TxBufCt = %x\n", get_info.TxBufCt);

	fMACAddress = get_info.CurrentNodeAddress;
	
	TRACE("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", fMACAddress[0], fMACAddress[1], fMACAddress[2], fMACAddress[3], fMACAddress[4], fMACAddress[5]);

	return B_OK;
}


mac_addr_t
UNDI::MACAddress() const
{
	return fMACAddress;
}


void *
UNDI::AllocateSendReceiveBuffer(size_t size)
{
	TRACE("UNDI::AllocateSendReceiveBuffer, size %ld\n", size);
	if (size > 0x3000)
		return NULL;

	return (void *)0x500;
}


void
UNDI::FreeSendReceiveBuffer(void *buffer)
{
	TRACE("UNDI::FreeSendReceiveBuffer\n");
}


ssize_t
UNDI::Send(const void *buffer, size_t size)
{
	TRACE("UNDI::Send, buffer %p, size %ld\n", buffer, size);

	hex_dump(buffer, size);

	PXENV_UNDI_TRANSMIT undi_tx;
	PXENV_UNDI_TBD undi_tbd;

	undi_tx.Protocol = P_UNKNOWN;
	undi_tx.XmitFlag = XMT_DESTADDR;
	undi_tx.DestAddr.seg = SEG((char *)buffer + 16);
	undi_tx.DestAddr.ofs = OFS((char *)buffer + 16);
	undi_tx.TBD.seg = SEG(&undi_tbd);
	undi_tx.TBD.ofs = OFS(&undi_tbd);

	undi_tbd.ImmedLength = size;
	undi_tbd.Xmit.seg = SEG(buffer);
	undi_tbd.Xmit.ofs = OFS(buffer);
	undi_tbd.DataBlkCount = 1;
	undi_tbd.DataBlock[0].TDPtrType = 1;
	undi_tbd.DataBlock[0].TDRsvdByte = 0;
	undi_tbd.DataBlock[0].TDDataLen = size;
	undi_tbd.DataBlock[0].TDDataPtr.seg = SEG(buffer);
	undi_tbd.DataBlock[0].TDDataPtr.ofs = OFS(buffer);

	uint16 res = call_pxe_bios(fPxeData, UNDI_TRANSMIT, &undi_tx);
	if (res != 0 || undi_tx.Status != 0) {
		TRACE("UNDI_TRANSMIT failed, res %x, status %x\n", res, undi_tx.Status);
		return 0;
	}

	TRACE("UNDI_TRANSMIT success\n");

	return size;
}


ssize_t
UNDI::Receive(void *buffer, size_t size)
{
	//TRACE("UNDI::Receive, buffer %p, size %ld\n", buffer, size);

	PXENV_UNDI_ISR undi_isr;
	uint16 res;

	if (!fRxFinished) {
		TRACE("continue receive...\n");

		undi_isr.FuncFlag = PXENV_UNDI_ISR_IN_GET_NEXT;
		res = call_pxe_bios(fPxeData, UNDI_ISR, &undi_isr);
		if (res != 0 || undi_isr.Status != 0) {
			TRACE("PXENV_UNDI_ISR_IN_GET_NEXT failed, res %x, status %x\n", res, undi_isr.Status);
			fRxFinished = true;
			return 0;
		}
	
	} else {

		undi_isr.FuncFlag = PXENV_UNDI_ISR_IN_START;

		res = call_pxe_bios(fPxeData, UNDI_ISR, &undi_isr);
		if (res != 0 || undi_isr.Status != 0) {
			TRACE("PXENV_UNDI_ISR_IN_START failed, res %x, status %x\n", res, undi_isr.Status);
			return 0;
		}

		if (undi_isr.FuncFlag != PXENV_UNDI_ISR_OUT_OURS) {
//			TRACE("not ours\n");
			return 0;
		}

		// send EOI to pic ?

		TRACE("PXENV_UNDI_ISR_OUT_OURS\n");
	
		undi_isr.FuncFlag = PXENV_UNDI_ISR_IN_PROCESS;
		res = call_pxe_bios(fPxeData, UNDI_ISR, &undi_isr);
		if (res != 0 || undi_isr.Status != 0) {
			TRACE("PXENV_UNDI_ISR_IN_PROCESS failed, res %x, status %x\n", res, undi_isr.Status);
			return 0;
		}
	}

	switch (undi_isr.FuncFlag) {
		case PXENV_UNDI_ISR_OUT_TRANSMIT:
			TRACE("PXENV_UNDI_ISR_OUT_TRANSMIT\n");
			fRxFinished = false;
			return 0;

		case PXENV_UNDI_ISR_OUT_RECEIVE:
			TRACE("PXENV_UNDI_ISR_OUT_RECEIVE\n");
			TRACE("BufferLength %d\n", undi_isr.BufferLength);
			TRACE("FrameLength %d\n", undi_isr.FrameLength);
			TRACE("FrameHeaderLength %d\n", undi_isr.FrameHeaderLength);
			if (size > undi_isr.BufferLength)
				size = undi_isr.BufferLength;
			memcpy(buffer, (const void *)(undi_isr.Frame.seg * 16 + undi_isr.Frame.ofs), size);
			hex_dump(buffer, size);
			fRxFinished = false;
			return size;

		case PXENV_UNDI_ISR_OUT_BUSY:
			TRACE("PXENV_UNDI_ISR_OUT_BUSY\n");
			fRxFinished = true;
			return 0;

		case PXENV_UNDI_ISR_OUT_DONE:
			TRACE("PXENV_UNDI_ISR_OUT_DONE\n");
			fRxFinished = true;
			return 0;

		default:
			TRACE("default!!!\n");
			return 0;
	}
}


status_t
platform_net_stack_init()
{
	TRACE("platform_net_stack_init\n");

	UNDI *interface = new(nothrow) UNDI;
	if (!interface)
		return B_NO_MEMORY;

	status_t error = interface->Init();
	if (error != B_OK) {
		TRACE("platform_net_stack_init: interface init failed\n");
		delete interface;
		return error;
	}

	error = NetStack::Default()->AddEthernetInterface(interface);
	if (error != B_OK) {
		delete interface;
		return error;
	}

	return B_OK;
}
