//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_VIRTUAL_PARTITION_H
#define _UDF_VIRTUAL_PARTITION_H

/*! \file VirtualPartition.h
*/

#include <kernel_cpp.h>

#include "Partition.h"
#include "PhysicalPartition.h"
#include "UdfDebug.h"

namespace Udf {

/*! \brief Type 2 virtual partition

	VirtualPartitions add an extra layer of indirection between logical
	block numbers and physical block numbers, allowing the underlying
	physical block numbers to be changed without changing the original
	references to (virtual) logical block numbers.
	
	Note that VirtualPartitions should be found only on sequentially written
	media such as CD-R, per UDF-2.01 2.2.10.

	See also UDF-2.01 2.2.8, UDF-2.01 2.2.10
*/ 
class VirtualPartition : public Partition {
public:
	VirtualPartition(PhysicalPartition &physicalPartition);
	virtual ~VirtualPartition();
	virtual status_t MapBlock(uint32 logicalBlock, uint32 &physicalBlock);
	
	status_t InitCheck();
private:
	PhysicalPartition fPhysicalPartition;
};

};	// namespace Udf

#endif	// _UDF_VIRTUAL_PARTITION_H
