//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_PRIVATE_H
#define _DISK_DEVICE_PRIVATE_H

#include <DiskDeviceVisitor.h>
#include <SupportDefs.h>

namespace BPrivate {

// PartitionFilter
class PartitionFilter {
public:
	virtual bool Filter(BPartition *partition) = 0;
};

// PartitionFilterVisitor
class PartitionFilterVisitor : public BDiskDeviceVisitor {
public:
	PartitionFilterVisitor(BDiskDeviceVisitor *visitor,
						   PartitionFilter *filter);

	virtual bool Visit(BDiskDevice *device);
	virtual bool Visit(BSession *session);
	virtual bool Visit(BPartition *partition);

private:
	BDiskDeviceVisitor	*fVisitor;
	PartitionFilter		*fFilter;
};

// IDFinderVisitor
class IDFinderVisitor : public BDiskDeviceVisitor {
public:
	IDFinderVisitor(int32 id);

	virtual bool Visit(BDiskDevice *device);
	virtual bool Visit(BSession *session);
	virtual bool Visit(BPartition *partition);

private:
	int32				fID;
};

}	// namespace BPrivate

using BPrivate::PartitionFilter;
using BPrivate::PartitionFilterVisitor;
using BPrivate::IDFinderVisitor;


#endif	// _DISK_DEVICE_PRIVATE_H
