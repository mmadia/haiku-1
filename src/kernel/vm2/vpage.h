#ifndef VPAGE_H
#define VPAGE_H
#include <vm.h>
#include <pageManager.h>
#include <swapFileManager.h>

class areaManager;
class vpage : public node
{
	private:
		page *physPage;
		vnode *backingNode;
		protectType protection;
		bool dirty;
		bool swappable;
		unsigned long start_address;
		unsigned long end_address;
	public:
		bool isMapped(void) {return (physPage);}
		bool contains(uint32 address) { return ((start_address<=address) && (end_address>=address)); }
		void flush(void); // write page to vnode, if necessary
		void refresh(void); // Read page back in from vnode
		vpage(void);
		void setup(unsigned long  start,vnode *backing, page *physMem,protectType prot,pageState state); // backing and/or physMem can be NULL/0.
		void cleanup(void);
		void setProtection(protectType prot);
		protectType getProtection(void) {return protection;}
		void *getStartAddress(void) {return (void *)start_address;}
		page *getPhysPage(void) {return physPage;}
		vnode *getBacking(void) {return backingNode;}

		bool fault(void *fault_address, bool writeError); // true = OK, false = panic.

		void pager(int desperation);
		void saver(void);
		
		void dump(void)
		{
			printf ("Dumping vpage %p, address = %lx, \n\t physPage: ",this,start_address);
			if (physPage)
				physPage->dump();
			else
				printf ("NULL\n");
		}
		char getByte(unsigned long offset,areaManager *manager); // This is for testing only
		void setByte(unsigned long offset,char value,areaManager *manager); // This is for testing only
		int getInt(unsigned long offset,areaManager *manager); // This is for testing only
		void setInt(unsigned long offset,int value,areaManager *manager); // This is for testing only
};
#endif
