//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_MANAGER__H
#define _K_PPP_MANAGER__H

#include <net_module.h>


#define PPP_MANAGER_MODULE_NAME "network/interfaces/ppp"

// this allows you to ask for specific interface_ids
enum PPP_INTERFACE_FILTER {
	PPP_ALL_INTERFACES,
	PPP_REGISTERED_INTERFACES,
	PPP_UNREGISTERED_INTERFACES
};

typedef struct ppp_manager_info {
	kernel_net_module_info knminfo;
	
	uint32 (*create_interface)(driver_settings *settings, interface_id parent);
		// you should always create interfaces using this function
	void (*delete_interface)(interface_id ID);
		// this marks the interface for deletion
	
	ifnet* (*register_interface)(interface_id ID);
	bool (*unregister_interface)(interface_id ID);
	
	status_t (*control)(interface_id ID, uint32 op, void *data, size_t length);
	
	status_t (*get_interfaces)(interface_id **interfaces, uint32 *count,
		PPP_INTERFACE_FILTER filter = PPP_REGISTERED_INTERFACES);
		// the user is responsible for free()'ing the interface_id array
} ppp_manager_info;


#endif
