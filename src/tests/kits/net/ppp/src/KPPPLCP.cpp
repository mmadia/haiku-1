//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "KPPPLCP.h"


#define PPP_PROTOCOL_OVERHEAD				2

// TODO:
// - add LCP extension handlers


PPPLCP::PPPLCP(PPPInterface& interface)
	: PPPProtocol("LCP", PPP_ESTABLISHMENT_PHASE, PPP_LCP_PROTOCOL,
		AF_UNSPEC, &interface, NULL, PPP_ALWAYS_ALLOWED),
	fTarget(NULL)
{
	SetUpRequested(false);
		// the state machine does everything for us
}


PPPLCP::~PPPLCP()
{
	while(CountOptionHandlers())
		delete OptionHandlerAt(0);
}


bool
PPPLCP::AddOptionHandler(PPPOptionHandler *handler)
{
	if(!handler)
		return false
	
	LockerHelper locker(StateMachine().Locker());
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	fOptionHandlers.AddItem(handler);
}


bool
PPPLCP::RemoveOptionHandler(PPPOptionHandler *handler)
{
	LockerHelper locker(StateMachine().Locker());
	
	if(Phase() != PPP_DOWN_PHASE)
		return false;
			// a running connection may not change
	
	return fOptionHandlers.RemoveItem(handler);
}


PPPOptionHandler*
PPPLCP::OptionHandlerAt(int32 index) const
{
	PPPOptionHandler *handler = fOptionHandlers.ItemAt(index);
	
	if(handler == fOptionHandlers.DefaultItem())
		return NULL;
	
	return handler;
}

uint32
PPPLCP::AdditionalOverhead() const
{
	uint32 overhead += PPP_PROTOCOL_OVERHEAD;
	
	if(Target())
		overhead += Target()->Overhead();
	if(Interface()->Device())
		overhead += Interface()->Device()->Overhead();
	
	return overhead;
}


bool
PPPLCP::Up()
{
}


bool
PPPLCP::Down()
{
}


status_t
PPPLCP::Send(mbuf *packet)
{
	if(!Interface())
		return B_ERROR;
	
	return Interface()->Send(packet, PPP_LCP_PROTOCOL);
}


status_t
PPPLCP::Receive(mbuf *packet, uint16 protocol)
{
	if(protocol != PPP_LCP_PROTOCOL)
		return PPP_UNHANDLED;
	
	lcp_packet *data = mtod(packet, lcp_packet*);
	
	if(ntohs(data->length) < 4)
		return B_ERROR;
	
	switch(data->code) {
		case PPP_CONFIGURE_REQUEST:
			StateMachine().RCREvent(packet);
		break;
		
		case PPP_CONFIGURE_ACK:
			StateMachine().RCAEvent(packet);
		break;
		
		case PPP_CONFIGURE_NAK:
		case PPP_CONFIGURE_REJECT:
			StateMachine().RCNEvent(packet);
		break;
		
		case PPP_TERMINATE_REQUEST:
			StateMachine().RTREvent(packet);
		break;
		
		case PPP_TERMINATE_ACK:
			StateMachine().RTAEvent(packet);
		break;
		
		case PPP_CODE_REJECT:
			StateMachine().RXJEvent(packet);
		break;
		
		case PPP_PROTOCOL_REJECT:
			StateMachine().RXJEvent(packet);
		break;
		
		case PPP_ECHO_REQUEST:
			StateMachine().RXREvent(packet);
		break;
		
		case PPP_ECHO_REPLY:
		case PPP_DISCARD_REQUEST:
			// do nothing
		break;
		
		default:
			return B_ERROR;
	}
	
	return B_OK;
}


void
PPPLCP::Pulse()
{
	StateMachine().TimerEvent();
}
