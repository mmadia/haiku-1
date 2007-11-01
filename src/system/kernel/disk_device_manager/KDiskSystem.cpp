// KDiskSystem.cpp

#include <stdio.h>
#include <stdlib.h>

#include <KernelExport.h>
#include <util/kernel_cpp.h>

#include "ddm_userland_interface.h"
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KDiskSystem.h"


// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf


// constructor
KDiskSystem::KDiskSystem(const char *name)
	: fID(_NextID()),
	  fName(NULL),
	  fPrettyName(NULL),
	  fLoadCounter(0)
{
	set_string(fName, name);
}


// destructor
KDiskSystem::~KDiskSystem()
{
	free(fName);
}


// Init
status_t
KDiskSystem::Init()
{
	return (fName ? B_OK : B_NO_MEMORY);
}


// SetID
/*void
KDiskSystem::SetID(disk_system_id id)
{
	fID = id;
}*/


// ID
disk_system_id
KDiskSystem::ID() const
{
	return fID;
}


// Name
const char *
KDiskSystem::Name() const
{
	return fName;
}


// PrettyName
const char *
KDiskSystem::PrettyName()
{
	return fPrettyName;
}


// Flags
uint32
KDiskSystem::Flags() const
{
	return fFlags;
}


// IsFileSystem
bool
KDiskSystem::IsFileSystem() const
{
	return (fFlags & B_DISK_SYSTEM_IS_FILE_SYSTEM);
}


// IsPartitioningSystem
bool
KDiskSystem::IsPartitioningSystem() const
{
	return !(fFlags & B_DISK_SYSTEM_IS_FILE_SYSTEM);
}


// GetInfo
void
KDiskSystem::GetInfo(user_disk_system_info *info)
{
	if (!info)
		return;
	info->id = ID();
	strcpy(info->name, Name());
	strcpy(info->pretty_name, PrettyName());
	info->flags = Flags();
}


// Load
status_t
KDiskSystem::Load()
{
	ManagerLocker locker(KDiskDeviceManager::Default());
dprintf("KDiskSystem::Load(): %s -> %ld\n", Name(), fLoadCounter + 1);
	status_t error = B_OK;
	if (fLoadCounter == 0)
		error = LoadModule();
	if (error == B_OK)
		fLoadCounter++;
	return error;
}


// Unload
void
KDiskSystem::Unload()
{
	ManagerLocker locker(KDiskDeviceManager::Default());
dprintf("KDiskSystem::Unload(): %s -> %ld\n", Name(), fLoadCounter - 1);
	if (fLoadCounter > 0 && --fLoadCounter == 0)
		UnloadModule();
}


// IsLoaded
bool
KDiskSystem::IsLoaded() const
{
	ManagerLocker locker(KDiskDeviceManager::Default());
	return (fLoadCounter > 0);
}


// Identify
float
KDiskSystem::Identify(KPartition *partition, void **cookie)
{
	// to be implemented by derived classes
	return -1;
}


// Scan
status_t
KDiskSystem::Scan(KPartition *partition, void *cookie)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// FreeIdentifyCookie
void
KDiskSystem::FreeIdentifyCookie(KPartition *partition, void *cookie)
{
	// to be implemented by derived classes
}


// FreeCookie
void
KDiskSystem::FreeCookie(KPartition *partition)
{
	// to be implemented by derived classes
}


// FreeContentCookie
void
KDiskSystem::FreeContentCookie(KPartition *partition)
{
	// to be implemented by derived classes
}


#if 0

// Defragment
status_t
KDiskSystem::Defragment(KPartition *partition, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// Repair
status_t
KDiskSystem::Repair(KPartition *partition, bool checkOnly,
					KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// Resize
status_t
KDiskSystem::Resize(KPartition *partition, off_t size, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// ResizeChild
status_t
KDiskSystem::ResizeChild(KPartition *child, off_t size, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// Move
status_t
KDiskSystem::Move(KPartition *partition, off_t offset, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// MoveChild
status_t
KDiskSystem::MoveChild(KPartition *child, off_t offset, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// SetName
status_t
KDiskSystem::SetName(KPartition *partition, char *name, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// SetContentName
status_t
KDiskSystem::SetContentName(KPartition *partition, char *name,
							KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// SetType
status_t
KDiskSystem::SetType(KPartition *partition, char *type, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// SetParameters
status_t
KDiskSystem::SetParameters(KPartition *partition, const char *parameters,
						   KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// SetContentParameters
status_t
KDiskSystem::SetContentParameters(KPartition *partition,
								  const char *parameters, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// Initialize
status_t
KDiskSystem::Initialize(KPartition *partition, const char *name,
						const char *parameters, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// CreateChild
status_t
KDiskSystem::CreateChild(KPartition *partition, off_t offset, off_t size,
						 const char *type, const char *parameters,
						 KDiskDeviceJob *job, KPartition **child,
						 partition_id childID)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// DeleteChild
status_t
KDiskSystem::DeleteChild(KPartition *child, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

#endif	// 0


// LoadModule
status_t
KDiskSystem::LoadModule()
{
	// to be implemented by derived classes
	return B_ERROR;
}


// UnloadModule
void
KDiskSystem::UnloadModule()
{
	// to be implemented by derived classes
}


// SetPrettyName
status_t
KDiskSystem::SetPrettyName(const char *name)
{
	return set_string(fPrettyName, name);
}


// SetFlags
void
KDiskSystem::SetFlags(uint32 flags)
{
	fFlags = flags;
}


// _NextID
int32
KDiskSystem::_NextID()
{
	return atomic_add(&fNextID, 1);
}


// fNextID
int32 KDiskSystem::fNextID = 0;

