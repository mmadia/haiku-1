/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef INODE_H
#define INODE_H


#include "system_dependencies.h"

#include "Volume.h"
#include "Journal.h"
#include "Chain.h"
#include "Debug.h"
#include "CachedBlock.h"


class BPlusTree;
class TreeIterator;
class AttributeIterator;
class Index;
class InodeAllocator;
class NodeGetter;


enum inode_type {
	S_DIRECTORY		= S_IFDIR,
	S_FILE			= S_IFREG,
	S_SYMLINK		= S_IFLNK,

	S_INDEX_TYPES	= (S_STR_INDEX | S_INT_INDEX | S_UINT_INDEX | S_LONG_LONG_INDEX
						| S_ULONG_LONG_INDEX | S_FLOAT_INDEX | S_DOUBLE_INDEX)
};

class Inode {
	public:
		Inode(Volume *volume, ino_t id);
		Inode(Volume *volume, Transaction &transaction, ino_t id,
			mode_t mode, block_run &run);
		//Inode(CachedBlock *cached);
		~Inode();

		//bfs_inode *Node() const { return (bfs_inode *)fBlock; }
		ino_t ID() const { return fID; }
		off_t BlockNumber() const { return fVolume->VnodeToBlock(fID); }

		rw_lock &Lock() { return fLock; }
		recursive_lock &SmallDataLock() { return fSmallDataLock; }
		status_t WriteBack(Transaction &transaction);

		bool IsContainer() const
			{ return S_ISDIR(Mode()); }
		bool IsDirectory() const
			{ return (Mode() & (S_INDEX_DIR | S_ATTR_DIR | S_IFDIR))
					== S_IFDIR; }
		bool IsIndex() const
			{ return (Mode() & (S_INDEX_DIR | 0777)) == S_INDEX_DIR; }
				// that's a stupid check, but AFAIK the only possible method...

		bool IsAttributeDirectory() const
			{ return (Mode() & S_ATTR_DIR) != 0; }
		bool IsAttribute() const
			{ return (Mode() & S_ATTR) != 0; }
		bool IsFile() const
			{ return (Mode() & (S_IFMT | S_ATTR)) == S_FILE; }
		bool IsRegularNode() const
			{ return (Mode() & (S_ATTR_DIR | S_INDEX_DIR | S_ATTR)) == 0; }
				// a regular node in the standard namespace
				// (i.e. not an index or attribute)
		bool IsSymLink() const { return S_ISLNK(Mode()); }
		bool HasUserAccessableStream() const { return IsFile(); }
			// currently only files can be accessed with bfs_read()/bfs_write()

		bool IsDeleted() const { return (Flags() & INODE_DELETED) != 0; }

		mode_t Mode() const { return fNode.Mode(); }
		uint32 Type() const { return fNode.Type(); }
		int32 Flags() const { return fNode.Flags(); }

		off_t Size() const { return fNode.data.Size(); }
		off_t LastModified() const { return fNode.last_modified_time; }

		const block_run &BlockRun() const { return fNode.inode_num; }
		block_run &Parent() { return fNode.parent; }
		block_run &Attributes() { return fNode.attributes; }

		Volume *GetVolume() const { return fVolume; }

		status_t InitCheck(bool checkNode = true);

		status_t CheckPermissions(int accessMode) const;

		// small_data access methods
		small_data *FindSmallData(const bfs_inode *node,
			const char *name) const;
		const char *Name(const bfs_inode *node) const;
		status_t GetName(char *buffer,
			size_t bufferSize = B_FILE_NAME_LENGTH) const;
		status_t SetName(Transaction &transaction, const char *name);

		// high-level attribute methods
		status_t ReadAttribute(const char *name, int32 type, off_t pos,
			uint8 *buffer, size_t *_length);
		status_t WriteAttribute(Transaction &transaction, const char *name,
			int32 type, off_t pos, const uint8 *buffer, size_t *_length);
		status_t RemoveAttribute(Transaction &transaction, const char *name);

		// attribute methods
		status_t GetAttribute(const char *name, Inode **attribute);
		void ReleaseAttribute(Inode *attribute);
		status_t CreateAttribute(Transaction &transaction, const char *name,
			uint32 type, Inode **attribute);

		// for directories only:
		status_t GetTree(BPlusTree **_tree);
		bool IsEmpty();

		// manipulating the data stream
		status_t FindBlockRun(off_t pos, block_run &run, off_t &offset);

		status_t ReadAt(off_t pos, uint8 *buffer, size_t *length);
		status_t WriteAt(Transaction &transaction, off_t pos,
			const uint8 *buffer, size_t *length);
		status_t FillGapWithZeros(off_t oldSize, off_t newSize);

		status_t SetFileSize(Transaction &transaction, off_t size);
		status_t Append(Transaction &transaction, off_t bytes);
		status_t TrimPreallocation(Transaction &transaction);
		bool NeedsTrimming();

		status_t Free(Transaction &transaction);
		status_t Sync();

		bfs_inode &Node() { return fNode; }

		// create/remove inodes
		status_t Remove(Transaction &transaction, const char *name,
			ino_t *_id = NULL, bool isDirectory = false);
		static status_t Create(Transaction &transaction, Inode *parent,
			const char *name, int32 mode, int openMode, uint32 type,
			bool *_created = NULL, ino_t *_id = NULL, Inode **_inode = NULL,
			fs_vnode_ops *vnodeOps = NULL, uint32 publishFlags = 0);

		// index maintaining helper
		void UpdateOldSize()
			{ fOldSize = Size(); }
		void UpdateOldLastModified()
			{ fOldLastModified = Node().LastModifiedTime(); }
		off_t OldSize()
			{ return fOldSize; }
		off_t OldLastModified()
			{ return fOldLastModified; }

		// file cache
		void *FileCache() const { return fCache; }
		void SetFileCache(void *cache) { fCache = cache; }
		void *Map() const { return fMap; }
		void SetMap(void *map) { fMap = map; }

	private:
		Inode(const Inode &);
		Inode &operator=(const Inode &);
			// no implementation

		friend class AttributeIterator;
		friend class InodeAllocator;

		// small_data access methods
		status_t _MakeSpaceForSmallData(Transaction &transaction,
			bfs_inode *node, const char *name, int32 length);
		status_t _RemoveSmallData(Transaction &transaction, NodeGetter &node,
			const char *name);
		status_t _AddSmallData(Transaction &transaction, NodeGetter &node,
			const char *name, uint32 type, const uint8 *data, size_t length,
			bool force = false);
		status_t _GetNextSmallData(bfs_inode *node,
			small_data **_smallData) const;
		status_t _RemoveSmallData(bfs_inode *node, small_data *item,
			int32 index);
		status_t _RemoveAttribute(Transaction &transaction, const char *name,
			bool hasIndex, Index *index);

		void _AddIterator(AttributeIterator *iterator);
		void _RemoveIterator(AttributeIterator *iterator);

		status_t _FreeStaticStreamArray(Transaction &transaction, int32 level,
					block_run run, off_t size, off_t offset, off_t &max);
		status_t _FreeStreamArray(Transaction &transaction, block_run *array,
					uint32 arrayLength, off_t size, off_t &offset, off_t &max);
		status_t _AllocateBlockArray(Transaction &transaction, block_run &run);
		status_t _GrowStream(Transaction &transaction, off_t size);
		status_t _ShrinkStream(Transaction &transaction, off_t size);

	private:
		rw_lock			fLock;
		Volume			*fVolume;
		ino_t			fID;
		BPlusTree		*fTree;
		Inode			*fAttributes;
		void			*fCache;
		void			*fMap;
		bfs_inode		fNode;

		off_t			fOldSize;
		off_t			fOldLastModified;
			// we need those values to ensure we will remove
			// the correct keys from the indices

		mutable recursive_lock fSmallDataLock;
		Chain<AttributeIterator> fIterators;
};


class NodeGetter : public CachedBlock {
	public:
		NodeGetter(Volume *volume)
			: CachedBlock(volume)
		{
		}

		NodeGetter(Volume *volume, const Inode *inode)
			: CachedBlock(volume)
		{
			SetTo(volume->VnodeToBlock(inode->ID()));
		}

		NodeGetter(Volume *volume, Transaction &transaction,
				const Inode *inode, bool empty = false)
			: CachedBlock(volume)
		{
			SetToWritable(transaction, volume->VnodeToBlock(inode->ID()), empty);
		}

		~NodeGetter()
		{
		}

		const bfs_inode *
		SetToNode(const Inode *inode)
		{
			return (const bfs_inode *)SetTo(fVolume->VnodeToBlock(inode->ID()));
		}

		const bfs_inode *Node() const { return (const bfs_inode *)Block(); }
		bfs_inode *WritableNode() const { return (bfs_inode *)Block();  }
};


// The Vnode class provides a convenience layer upon get_vnode(), so that
// you don't have to call put_vnode() anymore, which may make code more
// readable in some cases

class Vnode {
	public:
		Vnode(Volume* volume, ino_t id)
			:
			fInode(NULL)
		{
			SetTo(volume, id);
		}

		Vnode(Volume* volume, block_run run)
			:
			fInode(NULL)
		{
			SetTo(volume, run);
		}

		Vnode()
			:
			fStatus(B_NO_INIT),
			fInode(NULL)
		{
		}

		~Vnode()
		{
			Unset();
		}

		status_t InitCheck()
		{
			return fStatus;
		}

		void Unset()
		{
			if (fInode != NULL) {
				put_vnode(fInode->GetVolume()->FSVolume(), fInode->ID());
				fInode = NULL;
				fStatus = B_NO_INIT;
			}
		}

		status_t SetTo(Volume* volume, ino_t id)
		{
			Unset();

			return fStatus = get_vnode(volume->FSVolume(), id, (void**)&fInode);
		}

		status_t SetTo(Volume* volume, block_run run)
		{
			return SetTo(volume, volume->ToVnode(run));
		}

		status_t Get(Inode** _inode)
		{
			*_inode = fInode;
			return fStatus;
		}

		void Keep()
		{
			fInode = NULL;
		}

	private:
		status_t	fStatus;
		Inode*		fInode;
};


class AttributeIterator {
	public:
		AttributeIterator(Inode *inode);
		~AttributeIterator();

		status_t Rewind();
		status_t GetNext(char *name, size_t *length, uint32 *type, ino_t *id);

	private:
		friend class Chain<AttributeIterator>;
		friend class Inode;

		void Update(uint16 index, int8 change);
		AttributeIterator *fNext;

	private:
		int32		fCurrentSmallData;
		Inode		*fInode, *fAttributes;
		TreeIterator *fIterator;
		void		*fBuffer;
};


/*!
	Converts the open mode, the open flags given to bfs_open(), into
	access modes, e.g. since O_RDONLY requires read access to the
	file, it will be converted to R_OK.
*/
inline int
openModeToAccess(int openMode)
{
	openMode &= O_RWMASK;
	if (openMode == O_RDONLY)
		return R_OK;
	else if (openMode == O_WRONLY)
		return W_OK;

	return R_OK | W_OK;
}

#endif	/* INODE_H */
