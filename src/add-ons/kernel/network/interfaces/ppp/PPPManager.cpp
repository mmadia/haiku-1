/*
 * Copyright 2003-2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include <cstdio>

#include "PPPManager.h"
#include <PPPControl.h>
#include <KPPPModule.h>
#include <KPPPUtils.h>
#include <settings_tools.h>

#include <net_stack_driver.h>

#include <LockerHelper.h>

#include <cstdlib>
#include <sys/sockio.h>


static const char sKPPPIfNameBase[] =	"ppp";


static
status_t
deleter_thread(void *data)
{
	PPPManager *manager = (PPPManager*) data;
	thread_id sender;
	int32 code;
	
	while(true) {
		manager->DeleterThreadEvent();
		
		// check if the manager is being destroyed
		if(receive_data_with_timeout(&sender, &code, NULL, 0, 500) == B_OK)
			return B_OK;
	}
	
	return B_OK;
}


static
void
pulse_timer(void *data)
{
	((PPPManager*)data)->Pulse();
}


PPPManager::PPPManager()
	: fLock("PPPManager"),
	fReportLock("PPPManagerReportLock"),
	fDefaultInterface(NULL),
	fReportManager(fReportLock),
	fNextID(1)
{
	TRACE("PPPManager: Constructor\n");
	
	fDeleterThread = spawn_kernel_thread(deleter_thread, "PPPManager: deleter_thread",
		B_NORMAL_PRIORITY, this);
	resume_thread(fDeleterThread);
	fPulseTimer = net_add_timer(pulse_timer, this, PPP_PULSE_RATE);
}


PPPManager::~PPPManager()
{
	TRACE("PPPManager: Destructor\n");
	
	int32 tmp;
	send_data(fDeleterThread, 0, NULL, 0);
		// notify deleter_thread that we are being destroyed
	wait_for_thread(fDeleterThread, &tmp);
	net_remove_timer(fPulseTimer);
	
	// now really delete the interfaces (the deleter_thread is not running)
	ppp_interface_entry *entry = NULL;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry) {
			delete entry->interface;
			free(entry->name);
			delete entry;
		}
	}
	
	// also delete all ppp_up references
	ppp_app_entry *app = NULL;
	for(int32 index = 0; index < fApps.CountItems(); index++) {
		app = fApps.ItemAt(index);
		if(app) {
			free(app->interfaceName);
			delete entry;
		}
	}
}


int32
PPPManager::Stop(ifnet *ifp)
{
	TRACE("PPPManager: Stop(%s)\n", ifp ? ifp->if_name : "Unknown");
	
	LockerHelper locker(fLock);
	
	ppp_interface_entry *entry = EntryFor(ifp);
	if(!entry) {
		ERROR("PPPManager: Stop(): Could not find interface!\n");
		return B_ERROR;
	}
	
	DeleteInterface(entry->interface->ID());
	
	return B_OK;
}


int32
PPPManager::Output(ifnet *ifp, struct mbuf *buf, struct sockaddr *dst,
	struct rtentry *rt0)
{
	TRACE("PPPManager: Output(%s)\n", ifp ? ifp->if_name : "Unknown");
	
	if(dst->sa_family == AF_UNSPEC)
		return B_ERROR;
	
	LockerHelper locker(fLock);
	ppp_interface_entry *entry = EntryFor(ifp);
	if(!entry) {
		ERROR("PPPManager: Output(): Could not find interface!\n");
		return B_ERROR;
	}
	
	atomic_add(&entry->accessing, 1);
	locker.UnlockNow();
	
	if(!entry->interface->DoesConnectOnDemand()
			&& ifp->if_flags & (IFF_UP | IFF_RUNNING) != (IFF_UP | IFF_RUNNING)) {
		m_freem(buf);
		atomic_add(&entry->accessing, -1);
		return ENETDOWN;
	}
	
	int32 result = B_ERROR;
	KPPPProtocol *protocol = entry->interface->FirstProtocol();
	for(; protocol; protocol = protocol->NextProtocol()) {
		if(!protocol->IsEnabled() || protocol->AddressFamily() != dst->sa_family)
			continue;
		
		if(protocol->Flags() & PPP_INCLUDES_NCP)
			result = protocol->Send(buf, protocol->ProtocolNumber() & 0x7FFF);
		else
			result = protocol->Send(buf, protocol->ProtocolNumber());
		if(result == PPP_UNHANDLED)
			continue;
		
		atomic_add(&entry->accessing, -1);
		return result;
	}
	
	m_freem(buf);
	atomic_add(&entry->accessing, -1);
	return B_ERROR;
}


int32
PPPManager::Control(ifnet *ifp, ulong cmd, caddr_t data)
{
	TRACE("PPPManager: Control(%s)\n", ifp ? ifp->if_name : "Unknown");
	
	LockerHelper locker(fLock);
	ppp_interface_entry *entry = EntryFor(ifp);
	if(!entry || entry->deleting) {
		ERROR("PPPManager: Control(): Could not find interface!\n");
		return B_ERROR;
	}
	
	int32 status = B_OK;
	atomic_add(&entry->accessing, 1);
	locker.UnlockNow();
	
	switch(cmd) {
		case SIOCSIFFLAGS:
			if(((ifreq*)data)->ifr_flags & IFF_DOWN) {
				if(entry->interface->DoesConnectOnDemand()
						&& entry->interface->Phase() == PPP_DOWN_PHASE)
					DeleteInterface(entry->interface->ID());
				else
					entry->interface->Down();
			} else if(((ifreq*)data)->ifr_flags & IFF_UP)
				entry->interface->Up();
		break;
		
		default:
			status = entry->interface->StackControl(cmd, data);
	}
	
	atomic_add(&entry->accessing, -1);
	return status;
}


ppp_interface_id
PPPManager::CreateInterface(const driver_settings *settings,
	ppp_interface_id parentID = PPP_UNDEFINED_INTERFACE_ID)
{
	return _CreateInterface(NULL, settings, parentID);
}


ppp_interface_id
PPPManager::CreateInterfaceWithName(const char *name,
	ppp_interface_id parentID = PPP_UNDEFINED_INTERFACE_ID)
{
	if(!name)
		return PPP_UNDEFINED_INTERFACE_ID;
	
	ppp_interface_id result = _CreateInterface(name, NULL, parentID);
	
	return result;
}


bool
PPPManager::DeleteInterface(ppp_interface_id ID)
{
	TRACE("PPPManager: DeleteInterface(%ld)\n", ID);
	
	// This only marks the interface for deletion.
	// Our deleter_thread does the real work.
	LockerHelper locker(fLock);
	
	ppp_interface_entry *entry = EntryFor(ID);
	if(!entry)
		return false;
	
	if(entry->deleting)
		return true;
			// this check prevents a dead-lock
	
	entry->deleting = true;
	atomic_add(&entry->accessing, 1);
	locker.UnlockNow();
	
	// bring interface down if needed
	if(entry->interface->State() != PPP_INITIAL_STATE
			|| entry->interface->Phase() != PPP_DOWN_PHASE)
		entry->interface->Down();
	
	atomic_add(&entry->accessing, -1);
	
	return true;
}


bool
PPPManager::RemoveInterface(ppp_interface_id ID)
{
	TRACE("PPPManager: RemoveInterface(%ld)\n", ID);
	
	LockerHelper locker(fLock);
	
	int32 index;
	ppp_interface_entry *entry = EntryFor(ID, &index);
	if(!entry || entry->deleting)
		return false;
	
	UnregisterInterface(ID);
	
	delete entry;
	fEntries.RemoveItem(index);
	
	return true;
}


ifnet*
PPPManager::RegisterInterface(ppp_interface_id ID)
{
	TRACE("PPPManager: RegisterInterface(%ld)\n", ID);
	
	LockerHelper locker(fLock);
	
	ppp_interface_entry *entry = EntryFor(ID);
	if(!entry || entry->deleting)
		return NULL;
	
	// maybe the interface is already registered
	if(entry->interface->Ifnet())
		return entry->interface->Ifnet();
	
	// TODO: allocate struct around ifnet with pointer to entry so we can optimize
	// ifnet->ppp_interface_entry translation (speeds-up Output()/Input())
	ifnet *ifp = (ifnet*) malloc(sizeof(ifnet));
	memset(ifp, 0, sizeof(ifnet));
	ifp->devid = -1;
	ifp->if_type = IFT_PPP;
	ifp->name = (char*)sKPPPIfNameBase;
	ifp->if_unit = FindUnit();
	ifp->if_flags = IFF_POINTOPOINT;
	ifp->rx_thread = ifp->tx_thread = -1;
	ifp->start = NULL;
	ifp->stop = ppp_ifnet_stop;
	ifp->output = ppp_ifnet_output;
	ifp->ioctl = ppp_ifnet_ioctl;
	
	TRACE("PPPManager::RegisterInterface(): Created new ifnet: %s%d\n",
		ifp->name, ifp->if_unit);
	
	if_attach(ifp);
	return ifp;
}


bool
PPPManager::UnregisterInterface(ppp_interface_id ID)
{
	TRACE("PPPManager: UnregisterInterface(%ld)\n", ID);
	
	LockerHelper locker(fLock);
	
	ppp_interface_entry *entry = EntryFor(ID);
	if(!entry)
		return false;
	
	if(entry->interface->Ifnet()) {
		if_detach(entry->interface->Ifnet());
		free(entry->interface->Ifnet());
	}
	
	return true;
}


status_t
PPPManager::Control(uint32 op, void *data, size_t length)
{
	TRACE("PPPManager: Control(%ld)\n", op);
	
	// this method is intended for use by userland applications
	
	switch(op) {
		case PPPC_CONTROL_MODULE: {
			if(length < sizeof(control_net_module_args) || !data)
				return B_ERROR;
			
			control_net_module_args *args = (control_net_module_args*) data;
			if(!args->name)
				return B_ERROR;
			
			char name[B_PATH_NAME_LENGTH];
			sprintf(name, "%s/%s", PPP_MODULES_PATH, args->name);
			ppp_module_info *module;
			if(get_module(name, (module_info**) &module) != B_OK
					|| !module->control)
				return B_NAME_NOT_FOUND;
			
			return module->control(args->op, args->data, args->length);
		} break;
		
		case PPPC_CREATE_INTERFACE: {
			if(length < sizeof(ppp_interface_description_info) || !data)
				return B_ERROR;
			
			ppp_interface_description_info *info
				= (ppp_interface_description_info*) data;
			if(!info->u.settings)
				return B_ERROR;
			
			info->interface = CreateInterface(info->u.settings);
				// parents cannot be set from userland
			return info->interface != PPP_UNDEFINED_INTERFACE_ID ? B_OK : B_ERROR;
		} break;
		
		case PPPC_CREATE_INTERFACE_WITH_NAME: {
			if(length < sizeof(ppp_interface_description_info) || !data)
				return B_ERROR;
			
			ppp_interface_description_info *info
				= (ppp_interface_description_info*) data;
			if(!info->u.name)
				return B_ERROR;
			
			info->interface = CreateInterfaceWithName(info->u.name);
				// parents cannot be set from userland
			return info->interface != PPP_UNDEFINED_INTERFACE_ID ? B_OK : B_ERROR;
		} break;
		
		case PPPC_DELETE_INTERFACE:
			if(length != sizeof(ppp_interface_id) || !data)
				return B_ERROR;
			
			if(!DeleteInterface(*(ppp_interface_id*)data))
				return B_ERROR;
		break;
		
		case PPPC_BRING_INTERFACE_UP: {
			if(length != sizeof(ppp_interface_id) || !data)
				return B_ERROR;
			
			LockerHelper locker(fLock);
			
			ppp_interface_entry *entry = EntryFor(*(ppp_interface_id*)data);
			if(!entry || entry->deleting)
				return B_BAD_INDEX;
			
			atomic_add(&entry->accessing, 1);
			locker.UnlockNow();
			
			entry->interface->Up();
			atomic_add(&entry->accessing, -1);
		} break;
		
		case PPPC_BRING_INTERFACE_DOWN: {
			if(length != sizeof(ppp_interface_id) || !data)
				return B_ERROR;
			
			LockerHelper locker(fLock);
			
			ppp_interface_entry *entry = EntryFor(*(ppp_interface_id*)data);
			if(!entry || entry->deleting)
				return B_BAD_INDEX;
			
			atomic_add(&entry->accessing, 1);
			locker.UnlockNow();
			
			entry->interface->Down();
			atomic_add(&entry->accessing, -1);
		} break;
		
		case PPPC_CONTROL_INTERFACE: {
			if(length < sizeof(ppp_control_info) || !data)
				return B_ERROR;
			
			ppp_control_info *control = (ppp_control_info*) data;
			
			return ControlInterface(control->index, control->op, control->data,
				control->length);
		} break;
		
		case PPPC_GET_INTERFACES: {
			if(length < sizeof(ppp_get_interfaces_info) || !data)
				return B_ERROR;
			
			ppp_get_interfaces_info *info = (ppp_get_interfaces_info*) data;
			if(!info->interfaces)
				return B_ERROR;
			
			info->resultCount = GetInterfaces(info->interfaces, info->count,
				info->filter);
			return info->resultCount >= 0 ? B_OK : B_ERROR;
		} break;
		
		case PPPC_COUNT_INTERFACES:
			if(length != sizeof(ppp_interface_filter) || !data)
				return B_ERROR;
			
			return CountInterfaces(*(ppp_interface_filter*)data);
		break;
		
		case PPPC_FIND_INTERFACE_WITH_SETTINGS: {
			if(length < sizeof(ppp_interface_description_info) || !data)
				return B_ERROR;
			
			ppp_interface_description_info *info
				= (ppp_interface_description_info*) data;
			if(!info->u.settings)
				return B_ERROR;
			
			LockerHelper locker(fLock);
			
			ppp_interface_entry *entry = EntryFor(info->u.settings);
			if(entry)
				info->interface = entry->interface->ID();
			else {
				info->interface = PPP_UNDEFINED_INTERFACE_ID;
				return B_ERROR;
			}
		} break;
		
		case PPPC_ENABLE_REPORTS: {
			if(length < sizeof(ppp_report_request) || !data)
				return B_ERROR;
			
			ppp_report_request *request = (ppp_report_request*) data;
			ReportManager().EnableReports(request->type, request->thread,
				request->flags);
		} break;
		
		case PPPC_DISABLE_REPORTS: {
			if(length < sizeof(ppp_report_request) || !data)
				return B_ERROR;
			
			ppp_report_request *request = (ppp_report_request*) data;
			ReportManager().DisableReports(request->type, request->thread);
		} break;
		
		default:
			return B_BAD_VALUE;
	}
	
	return B_OK;
}


status_t
PPPManager::ControlInterface(ppp_interface_id ID, uint32 op, void *data, size_t length)
{
	TRACE("PPPManager: ControlInterface(%ld)\n", ID);
	
	LockerHelper locker(fLock);
	
	status_t result = B_BAD_INDEX;
	ppp_interface_entry *entry = EntryFor(ID);
	if(entry && !entry->deleting) {
		atomic_add(&entry->accessing, 1);
		locker.UnlockNow();
		result = entry->interface->Control(op, data, length);
		atomic_add(&entry->accessing, -1);
	}
	
	return result;
}


int32
PPPManager::GetInterfaces(ppp_interface_id *interfaces, int32 count,
	ppp_interface_filter filter = PPP_REGISTERED_INTERFACES)
{
	TRACE("PPPManager: GetInterfaces()\n");
	
	LockerHelper locker(fLock);
	
	int32 item = 0;
		// the current item in 'interfaces' (also the number of ids copied)
	ppp_interface_entry *entry;
	
	for(int32 index = 0; item < count && index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(!entry || entry->deleting)
			continue;
		
		switch(filter) {
			case PPP_ALL_INTERFACES:
				interfaces[item++] = entry->interface->ID();
			break;
			
			case PPP_REGISTERED_INTERFACES:
				if(entry->interface->Ifnet())
					interfaces[item++] = entry->interface->ID();
			break;
			
			case PPP_UNREGISTERED_INTERFACES:
				if(!entry->interface->Ifnet())
					interfaces[item++] = entry->interface->ID();
			break;
			
			default:
				return B_ERROR;
					// the filter is unknown
		}
	}
	
	return item;
}


int32
PPPManager::CountInterfaces(ppp_interface_filter filter = PPP_REGISTERED_INTERFACES)
{
	TRACE("PPPManager: CountInterfaces()\n");
	
	LockerHelper locker(fLock);
	
	if(filter == PPP_ALL_INTERFACES)
		return fEntries.CountItems();
	
	int32 count = 0;
	ppp_interface_entry *entry;
	
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(!entry || entry->deleting)
			continue;
		
		switch(filter) {
			case PPP_REGISTERED_INTERFACES:
				if(entry->interface->Ifnet())
					++count;
			break;
			
			case PPP_UNREGISTERED_INTERFACES:
				if(!entry->interface->Ifnet())
					++count;
			break;
			
			default:
				return B_ERROR;
					// the filter is unknown
		}
	}
	
	return count;
}


ppp_interface_entry*
PPPManager::EntryFor(ppp_interface_id ID, int32 *saveIndex = NULL) const
{
	TRACE("PPPManager: EntryFor(%ld)\n", ID);
	
	if(ID == PPP_UNDEFINED_INTERFACE_ID)
		return NULL;
	
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry && entry->interface->ID() == ID) {
			if(saveIndex)
				*saveIndex = index;
			return entry;
		}
	}
	
	return NULL;
}


ppp_interface_entry*
PPPManager::EntryFor(ifnet *ifp, int32 *saveIndex = NULL) const
{
	if(!ifp)
		return NULL;
	
	TRACE("PPPManager: EntryFor(%s%d)\n", ifp->name, ifp->if_unit);
	
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry && entry->interface->Ifnet() == ifp) {
			if(saveIndex)
				*saveIndex = index;
			return entry;
		}
	}
	
	return NULL;
}


ppp_interface_entry*
PPPManager::EntryFor(const char *name, int32 *saveIndex = NULL) const
{
	if(!name)
		return NULL;
	
	TRACE("PPPManager: EntryFor(%s)\n", name);
	
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry && entry->name && !strcmp(entry->name, name)) {
			if(saveIndex)
				*saveIndex = index;
			return entry;
		}
	}
	
	return NULL;
}


ppp_interface_entry*
PPPManager::EntryFor(const driver_settings *settings) const
{
	if(!settings)
		return NULL;
	
	TRACE("PPPManager: EntryFor(settings)\n");
	
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry && equal_interface_settings(entry->interface->Settings(), settings))
			return entry;
	}
	
	return NULL;
}


ppp_app_entry*
PPPManager::AppFor(const char *name) const
{
	if(!name)
		return NULL;
	
	ppp_app_entry *app;
	for(int32 index = 0; index < fApps.CountItems(); index++) {
		app = fApps.ItemAt(index);
		if(app && !strcmp(app->interfaceName, name))
			return app;
	}
	
	return NULL;
}


void
PPPManager::SettingsChanged()
{
	// get default interface name and update interfaces if it changed
	void *handle = load_driver_settings("ptpnet.settings");
	const char *name = get_driver_parameter(handle, "default", NULL, NULL);
	
	if((!fDefaultInterface && !name) || (fDefaultInterface && name
			&& !strcmp(name, fDefaultInterface))) {
		unload_driver_settings(handle);
		return;
	}
	
	ppp_interface_entry *entry = EntryFor(fDefaultInterface);
	if(entry && entry->interface->StateMachine().Phase() == PPP_DOWN_PHASE)
		DeleteInterface(entry->interface->ID());
	
	free(fDefaultInterface);
	if(!name) {
		fDefaultInterface = NULL;
		unload_driver_settings(handle);
		return;
	}
	
	fDefaultInterface = strdup(name);
	unload_driver_settings(handle);
	ppp_interface_id id = CreateInterfaceWithName(fDefaultInterface);
	entry = EntryFor(id);
	if(entry)
		entry->interface->SetConnectOnDemand(true);
}


static
int
greater(const void *a, const void *b)
{
	return (*(const int*)a - *(const int*)b);
}


// used by the public CreateInterface() methods
ppp_interface_id
PPPManager::_CreateInterface(const char *name,
	const driver_settings *settings, ppp_interface_id parentID)
{
	TRACE("PPPManager: CreateInterface(%s)\n", name ? name : "Unnamed");
	
	LockerHelper locker(fLock);
	
	ppp_interface_entry *parentEntry = EntryFor(parentID);
	if(parentID != PPP_UNDEFINED_INTERFACE_ID && !parentEntry)
		return PPP_UNDEFINED_INTERFACE_ID;
	
	// the day when NextID() returns 0 will come, be prepared! ;)
	ppp_interface_id id = NextID();
	if(id == PPP_UNDEFINED_INTERFACE_ID)
		id = NextID();
	
	// check if we already have an entry for the named interface
	ppp_interface_entry *entry = EntryFor(name);
	if(entry)
		return entry->interface->ID();
	
	entry = new ppp_interface_entry;
	entry->name = name ? strdup(name) : NULL;
	entry->accessing = 1;
	entry->deleting = false;
	fEntries.AddItem(entry);
		// nothing bad can happen because we are in a locked section
	
	new KPPPInterface(name, entry, id, settings,
		parentEntry ? parentEntry->interface : NULL);
			// KPPPInterface will add itself to the entry (no need to do it here)
	if(entry->interface->InitCheck() != B_OK) {
		delete entry->interface;
		free(entry->name);
		fEntries.RemoveItem(entry);
		delete entry;
		return PPP_UNDEFINED_INTERFACE_ID;
	}
	
	// find an existing ppp_up entry or create a new one otherwise
	if(entry->name) {
		ppp_app_entry *app = AppFor(entry->name);
		thread_info info;
		if(!app || get_thread_info(app->thread, &info) != B_OK) {
			if(!app) {
				app = new ppp_app_entry;
				app->interfaceName = strdup(entry->name);
				fApps.AddItem(app);
			}
			
			// run new ppp_up instance
			const char *argv[] = { "/boot/beos/bin/ppp_up", entry->name, NULL };
			const char *env[] = { NULL };
			app->thread = load_image(2, argv, env);
			if(app->thread < 0)
				ERROR("KPPPInterface::Up(): Error: could not load ppp_up!\n");
			resume_thread(app->thread);
		}
	} else
		entry->interface->SetAskBeforeConnecting(false);
	
	locker.UnlockNow();
		// it is safe to access the manager from userland now
	
	Report(PPP_MANAGER_REPORT, PPP_REPORT_INTERFACE_CREATED, &id,
		sizeof(ppp_interface_id));
	
	// notify handlers that interface has been created and they can initialize it now
	entry->interface->StateMachine().DownProtocols();
	entry->interface->StateMachine().ResetLCPHandlers();
	
	atomic_add(&entry->accessing, -1);
	
	return id;
}


int32
PPPManager::FindUnit() const
{
	// Find the smallest unused unit.
	int32 *units = new int32[fEntries.CountItems()];
	
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry && entry->interface->Ifnet())
			units[index] = entry->interface->Ifnet()->if_unit;
		else
			units[index] = -1;
	}
	
	qsort(units, fEntries.CountItems(), sizeof(int32), greater);
	
	int32 unit = 0;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		if(units[index] > unit)
			return unit;
		else if(units[index] == unit)
			++unit;
	}
	
	return unit;
}


void
PPPManager::DeleterThreadEvent()
{
	LockerHelper locker(fLock);
	
	SettingsChanged();
		// TODO: Use Haiku's kernel node monitoring capabilities
	
	// delete and remove marked interfaces
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry && entry->deleting && entry->accessing <= 0) {
			delete entry->interface;
			fEntries.RemoveItem(index);
			--index;
			
			// recreate default interface
			if(entry->name && fDefaultInterface &&
					!strcmp(entry->name, fDefaultInterface)) {
				free(fDefaultInterface);
				fDefaultInterface = NULL;
				SettingsChanged();
			}
			
			free(entry->name);
			delete entry;
		}
	}
	
	// remove dead ppp_up entries
	ppp_app_entry *app;
	thread_info info;
	for(int32 index = 0; index < fApps.CountItems(); index++) {
		app = fApps.ItemAt(index);
		if(app && get_thread_info(app->thread, &info) != B_OK) {
			fApps.RemoveItem(index);
			--index;
			free(app->interfaceName);
			delete app;
		}
	}
}


void
PPPManager::Pulse()
{
	LockerHelper locker(fLock);
	
	ppp_interface_entry *entry;
	for(int32 index = 0; index < fEntries.CountItems(); index++) {
		entry = fEntries.ItemAt(index);
		if(entry)
			entry->interface->Pulse();
	}
}
