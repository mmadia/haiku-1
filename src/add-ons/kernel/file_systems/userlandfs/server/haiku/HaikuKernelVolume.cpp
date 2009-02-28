// HaikuKernelVolume.cpp

#include "HaikuKernelVolume.h"

#include <new>

#include <fcntl.h>
#include <unistd.h>

#include "Debug.h"

#include "../kernel_emu.h"

#include "HaikuKernelNode.h"


// constructor
HaikuKernelVolume::HaikuKernelVolume(FileSystem* fileSystem, dev_t id,
	file_system_module_info* fsModule)
	: Volume(fileSystem, id),
	  fFSModule(fsModule)
{
	fVolume.id = id;
	fVolume.partition = -1;
	fVolume.layer = 0;
	fVolume.private_volume = NULL;		// filled in by the FS
	fVolume.ops = NULL;					// filled in by the FS
	fVolume.sub_volume = NULL;
	fVolume.super_volume = NULL;
	fVolume.file_system = fFSModule;
	fVolume.file_system_name = "dummy";	// TODO: Init correctly!
	fVolume.haikuVolume = this;
}

// destructor
HaikuKernelVolume::~HaikuKernelVolume()
{
}


// NewVNode
status_t
HaikuKernelVolume::NewVNode(ino_t vnodeID, void* privateNode, fs_vnode_ops* ops,
	HaikuKernelNode** node)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}


// PublishVNode
status_t
HaikuKernelVolume::PublishVNode(ino_t vnodeID, void* privateNode,
	fs_vnode_ops* ops, int type, uint32 flags, HaikuKernelNode** node)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}


// UndoNewVNode
status_t
HaikuKernelVolume::UndoNewVNode(HaikuKernelNode* node)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}


// UndoPublishVNode
status_t
HaikuKernelVolume::UndoPublishVNode(HaikuKernelNode* node)
{
	// TODO: Implement!
	return B_NOT_SUPPORTED;
}


// #pragma mark -
// #pragma mark ----- FS -----

// Mount
status_t
HaikuKernelVolume::Mount(const char* device, uint32 flags,
	const char* parameters, ino_t* rootID)
{
	if (!fFSModule->mount)
		return B_BAD_VALUE;

	// mount
	status_t error = fFSModule->mount(&fVolume, device, flags, parameters,
		rootID);
	if (error != B_OK)
		return error;

	_InitCapabilities();

	return B_OK;
}

// Unmount
status_t
HaikuKernelVolume::Unmount()
{
	if (!fVolume.ops->unmount)
		return B_BAD_VALUE;

	return fVolume.ops->unmount(&fVolume);
}

// Sync
status_t
HaikuKernelVolume::Sync()
{
	if (!fVolume.ops->sync)
		return B_BAD_VALUE;
	return fVolume.ops->sync(&fVolume);
}

// ReadFSInfo
status_t
HaikuKernelVolume::ReadFSInfo(fs_info* info)
{
	if (!fVolume.ops->read_fs_info)
		return B_BAD_VALUE;
	return fVolume.ops->read_fs_info(&fVolume, info);
}

// WriteFSInfo
status_t
HaikuKernelVolume::WriteFSInfo(const struct fs_info* info, uint32 mask)
{
	if (!fVolume.ops->write_fs_info)
		return B_BAD_VALUE;
	return fVolume.ops->write_fs_info(&fVolume, info, mask);
}


// #pragma mark - file cache


// GetFileMap
status_t
HaikuKernelVolume::GetFileMap(void* _node, off_t offset, size_t size,
	struct file_io_vec* vecs, size_t* count)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->get_file_map)
		return B_BAD_VALUE;
	return node->ops->get_file_map(&fVolume, node, offset, size, vecs,
		count);
}


// #pragma mark - vnodes


// Lookup
status_t
HaikuKernelVolume::Lookup(void* _dir, const char* entryName, ino_t* vnid)
{
	HaikuKernelNode* dir = (HaikuKernelNode*)_dir;

	if (!dir->ops->lookup)
		return B_BAD_VALUE;
	return dir->ops->lookup(&fVolume, dir, entryName, vnid);

}

// GetVNodeName
status_t
HaikuKernelVolume::GetVNodeName(void* _node, char* buffer, size_t bufferSize)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	// If not implemented by the client file system, we invoke our super class
	// version, which emulates the functionality.
	if (!node->ops->get_vnode_name)
		return Volume::GetVNodeName(_node, buffer, bufferSize);
	return node->ops->get_vnode_name(&fVolume, node, buffer, bufferSize);
}

// ReadVNode
status_t
HaikuKernelVolume::ReadVNode(ino_t vnid, bool reenter, void** _node, int* type,
	uint32* flags)
{
	if (!fVolume.ops->get_vnode)
		return B_BAD_VALUE;

	// create a new wrapper node
	HaikuKernelNode* node = new(std::nothrow) HaikuKernelNode;
	if (node == NULL)
		return B_NO_MEMORY;
	node->volume = this;

	// get the node
	status_t error = fVolume.ops->get_vnode(&fVolume, vnid, node, type, flags,
		reenter);
	if (error != B_OK) {
		delete node;
		return error;
	}

	*_node = node;
	return B_OK;
}

// WriteVNode
status_t
HaikuKernelVolume::WriteVNode(void* _node, bool reenter)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->put_vnode)
		return B_BAD_VALUE;
	status_t error = node->ops->put_vnode(&fVolume, node, reenter);

	delete node;

	return error;
}

// RemoveVNode
status_t
HaikuKernelVolume::RemoveVNode(void* _node, bool reenter)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->remove_vnode)
		return B_BAD_VALUE;
	return node->ops->remove_vnode(&fVolume, node, reenter);
}


// #pragma mark - nodes


// IOCtl
status_t
HaikuKernelVolume::IOCtl(void* _node, void* cookie, uint32 command,
	void* buffer, size_t size)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->ioctl)
		return B_BAD_VALUE;
	return node->ops->ioctl(&fVolume, node, cookie, command, buffer,
		size);
}

// SetFlags
status_t
HaikuKernelVolume::SetFlags(void* _node, void* cookie, int flags)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->set_flags)
		return B_BAD_VALUE;
	return node->ops->set_flags(&fVolume, node, cookie, flags);
}

// Select
status_t
HaikuKernelVolume::Select(void* _node, void* cookie, uint8 event,
	selectsync* sync)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->select) {
		UserlandFS::KernelEmu::notify_select_event(sync, event, false);
		return B_OK;
	}
	return node->ops->select(&fVolume, node, cookie, event, sync);
}

// Deselect
status_t
HaikuKernelVolume::Deselect(void* _node, void* cookie, uint8 event,
	selectsync* sync)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->select || !node->ops->deselect)
		return B_OK;
	return node->ops->deselect(&fVolume, node, cookie, event, sync);
}

// FSync
status_t
HaikuKernelVolume::FSync(void* _node)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->fsync)
		return B_BAD_VALUE;
	return node->ops->fsync(&fVolume, node);
}

// ReadSymlink
status_t
HaikuKernelVolume::ReadSymlink(void* _node, char* buffer, size_t bufferSize,
	size_t* bytesRead)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->read_symlink)
		return B_BAD_VALUE;

	*bytesRead = bufferSize;

	return node->ops->read_symlink(&fVolume, node, buffer, bytesRead);
}

// CreateSymlink
status_t
HaikuKernelVolume::CreateSymlink(void* _dir, const char* name,
	const char* target, int mode)
{
	HaikuKernelNode* dir = (HaikuKernelNode*)_dir;

	if (!dir->ops->create_symlink)
		return B_BAD_VALUE;
	return dir->ops->create_symlink(&fVolume, dir, name, target, mode);
}

// Link
status_t
HaikuKernelVolume::Link(void* _dir, const char* name, void* _node)
{
	HaikuKernelNode* dir = (HaikuKernelNode*)_dir;
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!dir->ops->link)
		return B_BAD_VALUE;
	return dir->ops->link(&fVolume, dir, name, node);
}

// Unlink
status_t
HaikuKernelVolume::Unlink(void* _dir, const char* name)
{
	HaikuKernelNode* dir = (HaikuKernelNode*)_dir;

	if (!dir->ops->unlink)
		return B_BAD_VALUE;
	return dir->ops->unlink(&fVolume, dir, name);
}

// Rename
status_t
HaikuKernelVolume::Rename(void* _oldDir, const char* oldName, void* _newDir,
	const char* newName)
{
	HaikuKernelNode* oldDir = (HaikuKernelNode*)_oldDir;
	HaikuKernelNode* newDir = (HaikuKernelNode*)_newDir;

	if (!oldDir->ops->rename)
		return B_BAD_VALUE;
	return oldDir->ops->rename(&fVolume, oldDir, oldName, newDir, newName);
}

// Access
status_t
HaikuKernelVolume::Access(void* _node, int mode)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->access)
		return B_OK;
	return node->ops->access(&fVolume, node, mode);
}

// ReadStat
status_t
HaikuKernelVolume::ReadStat(void* _node, struct stat* st)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->read_stat)
		return B_BAD_VALUE;
	return node->ops->read_stat(&fVolume, node, st);
}

// WriteStat
status_t
HaikuKernelVolume::WriteStat(void* _node, const struct stat *st, uint32 mask)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->write_stat)
		return B_BAD_VALUE;
	return node->ops->write_stat(&fVolume, node, st, mask);
}


// #pragma mark - files


// Create
status_t
HaikuKernelVolume::Create(void* _dir, const char* name, int openMode, int mode,
	void** cookie, ino_t* vnid)
{
	HaikuKernelNode* dir = (HaikuKernelNode*)_dir;

	if (!dir->ops->create)
		return B_BAD_VALUE;
	return dir->ops->create(&fVolume, dir, name, openMode, mode, cookie,
		vnid);
}

// Open
status_t
HaikuKernelVolume::Open(void* _node, int openMode, void** cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->open)
		return B_BAD_VALUE;
	return node->ops->open(&fVolume, node, openMode, cookie);
}

// Close
status_t
HaikuKernelVolume::Close(void* _node, void* cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->close)
		return B_OK;
	return node->ops->close(&fVolume, node, cookie);
}

// FreeCookie
status_t
HaikuKernelVolume::FreeCookie(void* _node, void* cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->free_cookie)
		return B_OK;
	return node->ops->free_cookie(&fVolume, node, cookie);
}

// Read
status_t
HaikuKernelVolume::Read(void* _node, void* cookie, off_t pos, void* buffer,
	size_t bufferSize, size_t* bytesRead)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->read)
		return B_BAD_VALUE;

	*bytesRead = bufferSize;

	return node->ops->read(&fVolume, node, cookie, pos, buffer, bytesRead);
}

// Write
status_t
HaikuKernelVolume::Write(void* _node, void* cookie, off_t pos,
	const void* buffer, size_t bufferSize, size_t* bytesWritten)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->write)
		return B_BAD_VALUE;

	*bytesWritten = bufferSize;

	return node->ops->write(&fVolume, node, cookie, pos, buffer,
		bytesWritten);
}


// #pragma mark -  directories


// CreateDir
status_t
HaikuKernelVolume::CreateDir(void* _dir, const char* name, int mode,
	ino_t *newDir)
{
	HaikuKernelNode* dir = (HaikuKernelNode*)_dir;

	if (!dir->ops->create_dir)
		return B_BAD_VALUE;
	return dir->ops->create_dir(&fVolume, dir, name, mode, newDir);
}

// RemoveDir
status_t
HaikuKernelVolume::RemoveDir(void* _dir, const char* name)
{
	HaikuKernelNode* dir = (HaikuKernelNode*)_dir;

	if (!dir->ops->remove_dir)
		return B_BAD_VALUE;
	return dir->ops->remove_dir(&fVolume, dir, name);
}

// OpenDir
status_t
HaikuKernelVolume::OpenDir(void* _node, void** cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->open_dir)
		return B_BAD_VALUE;
	return node->ops->open_dir(&fVolume, node, cookie);
}

// CloseDir
status_t
HaikuKernelVolume::CloseDir(void* _node, void* cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->close_dir)
		return B_OK;
	return node->ops->close_dir(&fVolume, node, cookie);
}

// FreeDirCookie
status_t
HaikuKernelVolume::FreeDirCookie(void* _node, void* cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->free_dir_cookie)
		return B_OK;
	return node->ops->free_dir_cookie(&fVolume, node, cookie);
}

// ReadDir
status_t
HaikuKernelVolume::ReadDir(void* _node, void* cookie, void* buffer,
	size_t bufferSize, uint32 count, uint32* countRead)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->read_dir)
		return B_BAD_VALUE;

	*countRead = count;

	return node->ops->read_dir(&fVolume, node, cookie,
		(struct dirent*)buffer, bufferSize, countRead);
}

// RewindDir
status_t
HaikuKernelVolume::RewindDir(void* _node, void* cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->rewind_dir)
		return B_BAD_VALUE;
	return node->ops->rewind_dir(&fVolume, node, cookie);
}


// #pragma mark - attribute directories


// OpenAttrDir
status_t
HaikuKernelVolume::OpenAttrDir(void* _node, void** cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->open_attr_dir)
		return B_BAD_VALUE;
	return node->ops->open_attr_dir(&fVolume, node, cookie);
}

// CloseAttrDir
status_t
HaikuKernelVolume::CloseAttrDir(void* _node, void* cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->close_attr_dir)
		return B_OK;
	return node->ops->close_attr_dir(&fVolume, node, cookie);
}

// FreeAttrDirCookie
status_t
HaikuKernelVolume::FreeAttrDirCookie(void* _node, void* cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->free_attr_dir_cookie)
		return B_OK;
	return node->ops->free_attr_dir_cookie(&fVolume, node, cookie);
}

// ReadAttrDir
status_t
HaikuKernelVolume::ReadAttrDir(void* _node, void* cookie, void* buffer,
	size_t bufferSize, uint32 count, uint32* countRead)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->read_attr_dir)
		return B_BAD_VALUE;

	*countRead = count;

	return node->ops->read_attr_dir(&fVolume, node, cookie,
		(struct dirent*)buffer, bufferSize, countRead);
}

// RewindAttrDir
status_t
HaikuKernelVolume::RewindAttrDir(void* _node, void* cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->rewind_attr_dir)
		return B_BAD_VALUE;
	return node->ops->rewind_attr_dir(&fVolume, node, cookie);
}


// #pragma mark - attributes


// CreateAttr
status_t
HaikuKernelVolume::CreateAttr(void* _node, const char* name, uint32 type,
	int openMode, void** cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->create_attr)
		return B_BAD_VALUE;
	return node->ops->create_attr(&fVolume, node, name, type, openMode,
		cookie);
}

// OpenAttr
status_t
HaikuKernelVolume::OpenAttr(void* _node, const char* name, int openMode,
	void** cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->open_attr)
		return B_BAD_VALUE;
	return node->ops->open_attr(&fVolume, node, name, openMode, cookie);
}

// CloseAttr
status_t
HaikuKernelVolume::CloseAttr(void* _node, void* cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->close_attr)
		return B_OK;
	return node->ops->close_attr(&fVolume, node, cookie);
}

// FreeAttrCookie
status_t
HaikuKernelVolume::FreeAttrCookie(void* _node, void* cookie)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->free_attr_cookie)
		return B_OK;
	return node->ops->free_attr_cookie(&fVolume, node, cookie);
}

// ReadAttr
status_t
HaikuKernelVolume::ReadAttr(void* _node, void* cookie, off_t pos,
	void* buffer, size_t bufferSize, size_t* bytesRead)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->read_attr)
		return B_BAD_VALUE;

	*bytesRead = bufferSize;

	return node->ops->read_attr(&fVolume, node, cookie, pos, buffer,
		bytesRead);
}

// WriteAttr
status_t
HaikuKernelVolume::WriteAttr(void* _node, void* cookie, off_t pos,
	const void* buffer, size_t bufferSize, size_t* bytesWritten)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->write_attr)
		return B_BAD_VALUE;

	*bytesWritten = bufferSize;

	return node->ops->write_attr(&fVolume, node, cookie, pos, buffer,
		bytesWritten);
}

// ReadAttrStat
status_t
HaikuKernelVolume::ReadAttrStat(void* _node, void* cookie,
	struct stat *st)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->read_attr_stat)
		return B_BAD_VALUE;
	return node->ops->read_attr_stat(&fVolume, node, cookie, st);
}

// WriteAttrStat
status_t
HaikuKernelVolume::WriteAttrStat(void* _node, void* cookie,
	const struct stat* st, int statMask)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->write_attr_stat)
		return B_BAD_VALUE;
	return node->ops->write_attr_stat(&fVolume, node, cookie, st,
		statMask);
}

// RenameAttr
status_t
HaikuKernelVolume::RenameAttr(void* _oldNode, const char* oldName,
	void* _newNode, const char* newName)
{
	HaikuKernelNode* oldNode = (HaikuKernelNode*)_oldNode;
	HaikuKernelNode* newNode = (HaikuKernelNode*)_newNode;

	if (!oldNode->ops->rename_attr)
		return B_BAD_VALUE;
	return oldNode->ops->rename_attr(&fVolume, oldNode, oldName, newNode,
		newName);
}

// RemoveAttr
status_t
HaikuKernelVolume::RemoveAttr(void* _node, const char* name)
{
	HaikuKernelNode* node = (HaikuKernelNode*)_node;

	if (!node->ops->remove_attr)
		return B_BAD_VALUE;
	return node->ops->remove_attr(&fVolume, node, name);
}


// #pragma mark - indices


// OpenIndexDir
status_t
HaikuKernelVolume::OpenIndexDir(void** cookie)
{
	if (!fVolume.ops->open_index_dir)
		return B_BAD_VALUE;
	return fVolume.ops->open_index_dir(&fVolume, cookie);
}

// CloseIndexDir
status_t
HaikuKernelVolume::CloseIndexDir(void* cookie)
{
	if (!fVolume.ops->close_index_dir)
		return B_OK;
	return fVolume.ops->close_index_dir(&fVolume, cookie);
}

// FreeIndexDirCookie
status_t
HaikuKernelVolume::FreeIndexDirCookie(void* cookie)
{
	if (!fVolume.ops->free_index_dir_cookie)
		return B_OK;
	return fVolume.ops->free_index_dir_cookie(&fVolume, cookie);
}

// ReadIndexDir
status_t
HaikuKernelVolume::ReadIndexDir(void* cookie, void* buffer,
	size_t bufferSize, uint32 count, uint32* countRead)
{
	if (!fVolume.ops->read_index_dir)
		return B_BAD_VALUE;

	*countRead = count;

	return fVolume.ops->read_index_dir(&fVolume, cookie,
		(struct dirent*)buffer, bufferSize, countRead);
}

// RewindIndexDir
status_t
HaikuKernelVolume::RewindIndexDir(void* cookie)
{
	if (!fVolume.ops->rewind_index_dir)
		return B_BAD_VALUE;
	return fVolume.ops->rewind_index_dir(&fVolume, cookie);
}

// CreateIndex
status_t
HaikuKernelVolume::CreateIndex(const char* name, uint32 type, uint32 flags)
{
	if (!fVolume.ops->create_index)
		return B_BAD_VALUE;
	return fVolume.ops->create_index(&fVolume, name, type, flags);
}

// RemoveIndex
status_t
HaikuKernelVolume::RemoveIndex(const char* name)
{
	if (!fVolume.ops->remove_index)
		return B_BAD_VALUE;
	return fVolume.ops->remove_index(&fVolume, name);
}

// StatIndex
status_t
HaikuKernelVolume::ReadIndexStat(const char *name, struct stat *st)
{
	if (!fVolume.ops->read_index_stat)
		return B_BAD_VALUE;
	return fVolume.ops->read_index_stat(&fVolume, name, st);
}


// #pragma mark - queries


// OpenQuery
status_t
HaikuKernelVolume::OpenQuery(const char* queryString, uint32 flags,
	port_id port, uint32 token, void** cookie)
{
	if (!fVolume.ops->open_query)
		return B_BAD_VALUE;
	return fVolume.ops->open_query(&fVolume, queryString, flags, port,
		token, cookie);
}

// CloseQuery
status_t
HaikuKernelVolume::CloseQuery(void* cookie)
{
	if (!fVolume.ops->close_query)
		return B_OK;
	return fVolume.ops->close_query(&fVolume, cookie);
}

// FreeQueryCookie
status_t
HaikuKernelVolume::FreeQueryCookie(void* cookie)
{
	if (!fVolume.ops->free_query_cookie)
		return B_OK;
	return fVolume.ops->free_query_cookie(&fVolume, cookie);
}

// ReadQuery
status_t
HaikuKernelVolume::ReadQuery(void* cookie, void* buffer, size_t bufferSize,
	uint32 count, uint32* countRead)
{
	if (!fVolume.ops->read_query)
		return B_BAD_VALUE;

	*countRead = count;

	return fVolume.ops->read_query(&fVolume, cookie, (struct dirent*)buffer,
		bufferSize, countRead);
}

// RewindQuery
status_t
HaikuKernelVolume::RewindQuery(void* cookie)
{
	if (!fVolume.ops->rewind_query)
		return B_BAD_VALUE;
	return fVolume.ops->rewind_query(&fVolume, cookie);
}

// _InitCapabilities
void
HaikuKernelVolume::_InitCapabilities()
{
	fCapabilities.ClearAll();

	// FS operations
	fCapabilities.Set(FS_VOLUME_CAPABILITY_UNMOUNT, fVolume.ops->unmount);

	fCapabilities.Set(FS_VOLUME_CAPABILITY_READ_FS_INFO,
		fVolume.ops->read_fs_info);
	fCapabilities.Set(FS_VOLUME_CAPABILITY_WRITE_FS_INFO,
		fVolume.ops->write_fs_info);
	fCapabilities.Set(FS_VOLUME_CAPABILITY_SYNC, fVolume.ops->sync);

	// vnode operations
	fCapabilities.Set(FS_VOLUME_CAPABILITY_GET_VNODE, fVolume.ops->get_vnode);

	// index directory & index operations
	fCapabilities.Set(FS_VOLUME_CAPABILITY_OPEN_INDEX_DIR,
		fVolume.ops->open_index_dir);
	fCapabilities.Set(FS_VOLUME_CAPABILITY_CLOSE_INDEX_DIR,
		fVolume.ops->close_index_dir);
	fCapabilities.Set(FS_VOLUME_CAPABILITY_FREE_INDEX_DIR_COOKIE,
		fVolume.ops->free_index_dir_cookie);
	fCapabilities.Set(FS_VOLUME_CAPABILITY_READ_INDEX_DIR,
		fVolume.ops->read_index_dir);
	fCapabilities.Set(FS_VOLUME_CAPABILITY_REWIND_INDEX_DIR,
		fVolume.ops->rewind_index_dir);

	fCapabilities.Set(FS_VOLUME_CAPABILITY_CREATE_INDEX,
		fVolume.ops->create_index);
	fCapabilities.Set(FS_VOLUME_CAPABILITY_REMOVE_INDEX,
		fVolume.ops->remove_index);
	fCapabilities.Set(FS_VOLUME_CAPABILITY_READ_INDEX_STAT,
		fVolume.ops->read_index_stat);

	// query operations
	fCapabilities.Set(FS_VOLUME_CAPABILITY_OPEN_QUERY, fVolume.ops->open_query);
	fCapabilities.Set(FS_VOLUME_CAPABILITY_CLOSE_QUERY,
		fVolume.ops->close_query);
	fCapabilities.Set(FS_VOLUME_CAPABILITY_FREE_QUERY_COOKIE,
		fVolume.ops->free_query_cookie);
	fCapabilities.Set(FS_VOLUME_CAPABILITY_READ_QUERY, fVolume.ops->read_query);
	fCapabilities.Set(FS_VOLUME_CAPABILITY_REWIND_QUERY,
		fVolume.ops->rewind_query);


#if 0
	// vnode operations
	fCapabilities.Set(FS_VNODE_CAPABILITY_LOOKUP, fFSModule->lookup);
	fCapabilities.Set(FS_VNODE_CAPABILITY_GET_VNODE_NAME, fFSModule->get_vnode_name);

	fCapabilities.Set(FS_VNODE_CAPABILITY_GET_VNODE, fFSModule->get_vnode);
	fCapabilities.Set(FS_VNODE_CAPABILITY_PUT_VNODE, fFSModule->put_vnode);
	fCapabilities.Set(FS_VNODE_CAPABILITY_REMOVE_VNODE, fFSModule->remove_vnode);

	// VM file access
	fCapabilities.Set(FS_VNODE_CAPABILITY_CAN_PAGE, fFSModule->can_page);
	fCapabilities.Set(FS_VNODE_CAPABILITY_READ_PAGES, fFSModule->read_pages);
	fCapabilities.Set(FS_VNODE_CAPABILITY_WRITE_PAGES, fFSModule->write_pages);

	// cache file access
	fCapabilities.Set(FS_VNODE_CAPABILITY_GET_FILE_MAP, fFSModule->get_file_map);

	// common operations
	fCapabilities.Set(FS_VNODE_CAPABILITY_IOCTL, fFSModule->ioctl);
	fCapabilities.Set(FS_VNODE_CAPABILITY_SET_FLAGS, fFSModule->set_flags);
	fCapabilities.Set(FS_VNODE_CAPABILITY_SELECT, fFSModule->select);
	fCapabilities.Set(FS_VNODE_CAPABILITY_DESELECT, fFSModule->deselect);
	fCapabilities.Set(FS_VNODE_CAPABILITY_FSYNC, fFSModule->fsync);

	fCapabilities.Set(FS_VNODE_CAPABILITY_READ_SYMLINK, fFSModule->read_symlink);
	fCapabilities.Set(FS_VNODE_CAPABILITY_CREATE_SYMLINK, fFSModule->create_symlink);

	fCapabilities.Set(FS_VNODE_CAPABILITY_LINK, fFSModule->link);
	fCapabilities.Set(FS_VNODE_CAPABILITY_UNLINK, fFSModule->unlink);
	fCapabilities.Set(FS_VNODE_CAPABILITY_RENAME, fFSModule->rename);

	fCapabilities.Set(FS_VNODE_CAPABILITY_ACCESS, fFSModule->access);
	fCapabilities.Set(FS_VNODE_CAPABILITY_READ_STAT, fFSModule->read_stat);
	fCapabilities.Set(FS_VNODE_CAPABILITY_WRITE_STAT, fFSModule->write_stat);

	// file operations
	fCapabilities.Set(FS_VNODE_CAPABILITY_CREATE, fFSModule->create);
	fCapabilities.Set(FS_VNODE_CAPABILITY_OPEN, fFSModule->open);
	fCapabilities.Set(FS_VNODE_CAPABILITY_CLOSE, fFSModule->close);
	fCapabilities.Set(FS_VNODE_CAPABILITY_FREE_COOKIE, fFSModule->free_cookie);
	fCapabilities.Set(FS_VNODE_CAPABILITY_READ, fFSModule->read);
	fCapabilities.Set(FS_VNODE_CAPABILITY_WRITE, fFSModule->write);

	// directory operations
	fCapabilities.Set(FS_VNODE_CAPABILITY_CREATE_DIR, fFSModule->create_dir);
	fCapabilities.Set(FS_VNODE_CAPABILITY_REMOVE_DIR, fFSModule->remove_dir);
	fCapabilities.Set(FS_VNODE_CAPABILITY_OPEN_DIR, fFSModule->open_dir);
	fCapabilities.Set(FS_VNODE_CAPABILITY_CLOSE_DIR, fFSModule->close_dir);
	fCapabilities.Set(FS_VNODE_CAPABILITY_FREE_DIR_COOKIE, fFSModule->free_dir_cookie);
	fCapabilities.Set(FS_VNODE_CAPABILITY_READ_DIR, fFSModule->read_dir);
	fCapabilities.Set(FS_VNODE_CAPABILITY_REWIND_DIR, fFSModule->rewind_dir);

	// attribute directory operations
	fCapabilities.Set(FS_VNODE_CAPABILITY_OPEN_ATTR_DIR, fFSModule->open_attr_dir);
	fCapabilities.Set(FS_VNODE_CAPABILITY_CLOSE_ATTR_DIR, fFSModule->close_attr_dir);
	fCapabilities.Set(FS_VNODE_CAPABILITY_FREE_ATTR_DIR_COOKIE,
		fFSModule->free_attr_dir_cookie);
	fCapabilities.Set(FS_VNODE_CAPABILITY_READ_ATTR_DIR, fFSModule->read_attr_dir);
	fCapabilities.Set(FS_VNODE_CAPABILITY_REWIND_ATTR_DIR, fFSModule->rewind_attr_dir);

	// attribute operations
	fCapabilities.Set(FS_VNODE_CAPABILITY_CREATE_ATTR, fFSModule->create_attr);
	fCapabilities.Set(FS_VNODE_CAPABILITY_OPEN_ATTR, fFSModule->open_attr);
	fCapabilities.Set(FS_VNODE_CAPABILITY_CLOSE_ATTR, fFSModule->close_attr);
	fCapabilities.Set(FS_VNODE_CAPABILITY_FREE_ATTR_COOKIE,
		fFSModule->free_attr_cookie);
	fCapabilities.Set(FS_VNODE_CAPABILITY_READ_ATTR, fFSModule->read_attr);
	fCapabilities.Set(FS_VNODE_CAPABILITY_WRITE_ATTR, fFSModule->write_attr);

	fCapabilities.Set(FS_VNODE_CAPABILITY_READ_ATTR_STAT, fFSModule->read_attr_stat);
	fCapabilities.Set(FS_VNODE_CAPABILITY_WRITE_ATTR_STAT,
		fFSModule->write_attr_stat);
	fCapabilities.Set(FS_VNODE_CAPABILITY_RENAME_ATTR, fFSModule->rename_attr);
	fCapabilities.Set(FS_VNODE_CAPABILITY_REMOVE_ATTR, fFSModule->remove_attr);
#endif
}
