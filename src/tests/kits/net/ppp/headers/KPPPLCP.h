//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_LCP__H
#define _K_PPP_LCP__H

#include "KPPPProtocol.h"


typedef struct lcp_packet {
	uint8 code;
	uint8 id;
	uint16 length;
	int8 data[0];
} lcp_packet;


class PPPLCP : public PPPProtocol {
		friend class PPPInterface;

	private:
		// may only be constructed/destructed by PPPInterface
		PPPLCP(PPPInterface& interface);
		~PPPLCP();
		
		// copies are not allowed!
		PPPLCP(const PPPLCP& copy);
		PPPLCP& operator= (const PPPLCP& copy);

	public:
		PPPStateMachine& StateMachine() const
			{ return Interface()->StateMachine(); }
		
		bool AddOptionHandler(PPPOptionHandler *handler);
		bool RemoveOptionHandler(PPPOptionHandler *handler);
		int32 CountOptionHandlers() const
			{ return fOptionHandlers.CountItems(); }
		PPPOptionHandler *OptionHandlerAt(int32 index) const;
		
		PPPEncapsulator *Target() const
			{ return fTarget; }
		void SetTarget(PPPEncapsulator *target)
			{ fTarget = target; }
			// if target != all packtes will be passed to the encapsulator
			// instead of the interface/device
		
		uint32 AdditionalOverhead() const;
			// the overhead caused by the target, the device, and the interface

	private:
		List<PPPOptionHandler*> fOptionHandlers;
		
		PPPEncapsulator *fTarget;
};


#endif
