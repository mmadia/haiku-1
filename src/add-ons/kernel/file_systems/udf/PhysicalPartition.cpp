#include "PhysicalPartition.h"

#define B_NOT_IMPLEMENTED B_ERROR

using namespace Udf;

/*! \brief Creates a new PhysicalPartition object.
*/
PhysicalPartition::PhysicalPartition(uint16 number, uint32 start, uint32 length)
	: fNumber(number)
	, fStart(start)
	, fLength(length)
{
}

/*! \brief Destroys the PhysicalPartition object.
*/
PhysicalPartition::~PhysicalPartition()
{
}

/*! \brief Maps the given logical block to a physical block on disc.

	The given logical block is simply treated as an offset from the
	start of the physical partition.
*/
status_t
PhysicalPartition::MapBlock(uint32 logicalBlock, uint32 &physicalBlock)
{
	if (logicalBlock >= fLength)
		return B_BAD_ADDRESS;
	else {
		physicalBlock = fStart + logicalBlock;
		return B_OK;
	}
}
