/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "RootFileSystem.h"

#include <boot/partitions.h>
#include <boot/vfs.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <ddm_modules.h>
#include <util/kernel_cpp.h>

#include <unistd.h>
#include <string.h>

using namespace boot;

//#define TRACE_PARTITIONS
#ifdef TRACE_PARTITIONS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/* supported partition modules */

static const partition_module_info *sPartitionModules[] = {
#ifdef BOOT_SUPPORT_PARTITION_AMIGA
	&gAmigaPartitionModule,
#endif
#ifdef BOOT_SUPPORT_PARTITION_INTEL
	&gIntelPartitionMapModule,
	&gIntelExtendedPartitionModule,
#endif
#ifdef BOOT_SUPPORT_PARTITION_APPLE
	&gApplePartitionModule,
#endif
};
static const int32 sNumPartitionModules = sizeof(sPartitionModules) / sizeof(partition_module_info *);

/* supported file system modules */

static file_system_module_info *sFileSystemModules[] = {
#ifdef BOOT_SUPPORT_FILE_SYSTEM_BFS
	&gBFSFileSystemModule,
#endif
#ifdef BOOT_SUPPORT_FILE_SYSTEM_AMIGA_FFS
	&gAmigaFFSFileSystemModule,
#endif
#ifdef BOOT_SUPPORT_FILE_SYSTEM_FAT
	&gFATFileSystemModule,
#endif
#ifdef BOOT_SUPPORT_FILE_SYSTEM_HFS_PLUS
	&gHFSPlusFileSystemModule,
#endif
};
static const int32 sNumFileSystemModules = sizeof(sFileSystemModules) / sizeof(file_system_module_info *);

extern NodeList gPartitions;


namespace boot {

/** A convenience class to automatically close a
 *	file descriptor upon deconstruction.
 */

class NodeOpener {
	public:
		NodeOpener(Node *node, int mode)
		{
			fFD = open_node(node, mode);
		}

		~NodeOpener()
		{
			close(fFD);
		}

		int Descriptor() const { return fFD; }

	private:
		int		fFD;
};


//	#pragma mark -


Partition::Partition(int fd)
	:
	fParent(NULL),
	fIsFileSystem(false),
	fIsPartitioningSystem(false)
{
	memset((partition_data *)this, 0, sizeof(partition_data));
	id = (partition_id)this;

	// it's safe to close the file
	fFD = dup(fd);
}


Partition::~Partition()
{
	close(fFD);
}


ssize_t 
Partition::ReadAt(void *cookie, off_t position, void *buffer, size_t bufferSize)
{
	if (position > this->size)
		return 0;
	if (position < 0)
		return B_BAD_VALUE;

	if (position + bufferSize > this->size)
		bufferSize = this->size - position;

	return read_pos(fFD, this->offset + position, buffer, bufferSize);
}


ssize_t 
Partition::WriteAt(void *cookie, off_t position, const void *buffer, size_t bufferSize)
{
	if (position > this->size)
		return 0;
	if (position < 0)
		return B_BAD_VALUE;

	if (position + bufferSize > this->size)
		bufferSize = this->size - position;

	return write_pos(fFD, this->offset + position, buffer, bufferSize);
}


off_t
Partition::Size() const
{
	struct stat stat;
	if (fstat(fFD, &stat) == B_OK)
		return stat.st_size;

	return Node::Size();
}


int32 
Partition::Type() const
{
	struct stat stat;
	if (fstat(fFD, &stat) == B_OK)
		return stat.st_mode;

	return Node::Type();
}


Partition *
Partition::AddChild()
{
	Partition *child = new Partition(fFD);
	if (child == NULL)
		return NULL;

	child->SetParent(this);
	child_count++;
	fChildren.Add(child);

	return child;
}


status_t
Partition::Mount(Directory **_fileSystem)
{
	for (int32 i = 0; i < sNumFileSystemModules; i++) {
		file_system_module_info *module = sFileSystemModules[i];

		TRACE(("check for file_system: %s\n", module->pretty_name));

		Directory *fileSystem;
		if (module->get_file_system(this, &fileSystem) == B_OK) {
			gRoot->AddVolume(fileSystem, this);
			if (_fileSystem)
				*_fileSystem = fileSystem;

			// remember the module name that mounted us
			fModuleName = module->module_name;

			fIsFileSystem = true;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t 
Partition::Scan(bool mountFileSystems)
{
	// scan for partitions first (recursively all eventual children as well)
	
	TRACE(("Partition::Scan()\n"));

	for (int32 i = 0; i < sNumPartitionModules; i++) {
		const partition_module_info *module = sPartitionModules[i];
		void *cookie;
		NodeOpener opener(this, O_RDONLY);

		TRACE(("check for partitioning_system: %s\n", module->pretty_name));

		if (module->identify_partition(opener.Descriptor(), this, &cookie) <= 0.0)
			continue;

		status_t status = module->scan_partition(opener.Descriptor(), this, cookie);
		module->free_identify_partition_cookie(this, cookie);

		if (status == B_OK) {
			fIsPartitioningSystem = true;

			// now that we've found something, check our children
			// out as well!

			NodeIterator iterator = fChildren.GetIterator();
			Partition *child = NULL;

			while ((child = (Partition *)iterator.Next()) != NULL) {
				TRACE(("*** scan child %p (start = %Ld, size = %Ld, parent = %p)!\n",
					child, child->offset, child->size, child->Parent()));

				child->Scan(mountFileSystems);

				if (!mountFileSystems || child->IsFileSystem()) {
					// move the partitions containing file systems to the partition list
					fChildren.Remove(child);
					gPartitions.Add(child);
				}
			}

			// remove all unused children (we keep only file systems)

			while ((child = (Partition *)fChildren.Head()) != NULL) {
				fChildren.Remove(child);
				delete child;
			}

			// remember the module name that identified us
			fModuleName = module->module.name;

			return B_OK;
		}
	}

	// scan for file systems

	if (mountFileSystems)
		return Mount();

	return B_ENTRY_NOT_FOUND; 
}

}	// namespace boot


//	#pragma mark -


status_t
add_partitions_for(int fd, bool mountFileSystems)
{
	TRACE(("add_partitions_for(fd = %d, mountFS = %s)\n", fd, mountFileSystems ? "yes" : "no"));

	Partition *partition = new Partition(fd);

	// set some magic/default values
	partition->block_size = 512;
	partition->size = partition->Size();

	// add this partition to the list of partitions, if it contains
	// or might contain a file system
	if ((partition->Scan(mountFileSystems) == B_OK && partition->IsFileSystem())
		|| (!partition->IsPartitioningSystem() && !mountFileSystems)) {
		gPartitions.Add(partition);
		return B_OK;
	}

	// if not, we'll need to tell the children that their parent is gone

	NodeIterator iterator = gPartitions.GetIterator();
	Partition *child = NULL;

	while ((child = (Partition *)iterator.Next()) != NULL) {
		if (child->Parent() == partition)
			child->SetParent(NULL);
	}

	delete partition;
	return B_OK;
}


status_t
add_partitions_for(Node *device, bool mountFileSystems)
{
	TRACE(("add_partitions_for(%p, mountFS = %s)\n", device, mountFileSystems ? "yes" : "no"));

	int fd = open_node(device, O_RDONLY);
	if (fd < B_OK)
		return fd;

	status_t status = add_partitions_for(fd, mountFileSystems);
	if (status < B_OK)
		dprintf("add_partitions_for(%d) failed: %ld\n", fd, status);

	close(fd);
	return B_OK;
}


partition_data *
create_child_partition(partition_id id, int32 index, partition_id childID)
{
	Partition &partition = *(Partition *)id;
	Partition *child = partition.AddChild();
	if (child == NULL) {
		dprintf("creating partition failed: no memory\n");
		return NULL;
	}

	// we cannot do anything with the child here, because it was not
	// yet initialized by the partition module.
	TRACE(("new child partition!\n"));

	return child;
}


partition_data *
get_child_partition(partition_id id, int32 index)
{
	//Partition &partition = *(Partition *)id;

	// ToDo: do we really have to implement this?
	//	The intel partition module doesn't really needs this for our mission...

	return NULL;
}


partition_data *
get_parent_partition(partition_id id)
{
	Partition &partition = *(Partition *)id;

	return partition.Parent();
}

