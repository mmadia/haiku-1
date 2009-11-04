/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ServerBitmap.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BitmapManager.h"
#include "ClientMemoryAllocator.h"
#include "ColorConversion.h"
#include "HWInterface.h"
#include "InterfacePrivate.h"
#include "Overlay.h"
#include "ServerApp.h"


using std::nothrow;
using namespace BPrivate;


/*!
	A word about memory housekeeping and why it's implemented this way:

	The reason why this looks so complicated is to optimize the most common
	path (bitmap creation from the application), and don't cause any further
	memory allocations for maintaining memory in that case.
	If a bitmap was allocated this way, both, the fAllocator and
	fAllocationCookie members are used.

	For overlays, the allocator only allocates a small piece of client memory
	for use with the overlay_client_data structure - the actual buffer will be
	placed in the graphics frame buffer and is allocated by the graphics driver.

	If the memory was allocated on the app_server heap, neither fAllocator, nor
	fAllocationCookie are used, and the buffer is just freed in that case when
	the bitmap is destructed. This method is mainly used for cursors.
*/


/*!
	\brief Constructor called by the BitmapManager (only).
	\param rect Size of the bitmap.
	\param space Color space of the bitmap
	\param flags Various bitmap flags to tweak the bitmap as defined in Bitmap.h
	\param bytesperline Number of bytes in each row. -1 implies the default
		value. Any value less than the the default will less than the default
		will be overridden, but any value greater than the default will result
		in the number of bytes specified.
	\param screen Screen assigned to the bitmap.
*/
ServerBitmap::ServerBitmap(BRect rect, color_space space, uint32 flags,
		int32 bytesPerRow, screen_id screen)
	:
	fAllocator(NULL),
	fAllocationCookie(NULL),
	fOverlay(NULL),
	fBuffer(NULL),
	fReferenceCount(1),
	// WARNING: '1' is added to the width and height.
	// Same is done in FBBitmap subclass, so if you
	// modify here make sure to do the same under
	// FBBitmap::SetSize(...)
	fWidth(rect.IntegerWidth() + 1),
	fHeight(rect.IntegerHeight() + 1),
	fBytesPerRow(0),
	fSpace(space),
	fFlags(flags),
	fOwner(NULL)
	// fToken is initialized (if used) by the BitmapManager
{
	int32 minBytesPerRow = get_bytes_per_row(space, fWidth);

	fBytesPerRow = max_c(bytesPerRow, minBytesPerRow);
}


//! Copy constructor does not copy the buffer.
ServerBitmap::ServerBitmap(const ServerBitmap* bitmap)
	:
	fAllocator(NULL),
	fAllocationCookie(NULL),
	fOverlay(NULL),
	fBuffer(NULL),
	fReferenceCount(1)
{
	if (bitmap) {
		fWidth = bitmap->fWidth;
		fHeight = bitmap->fHeight;
		fBytesPerRow = bitmap->fBytesPerRow;
		fSpace = bitmap->fSpace;
		fFlags = bitmap->fFlags;
		fOwner = bitmap->fOwner;
	} else {
		fWidth = 0;
		fHeight = 0;
		fBytesPerRow = 0;
		fSpace = B_NO_COLOR_SPACE;
		fFlags = 0;
		fOwner = NULL;
	}
}


ServerBitmap::~ServerBitmap()
{
	if (fAllocator != NULL)
		fAllocator->Free(AllocationCookie());
	else
		delete[] fBuffer;

	delete fOverlay;
		// deleting the overlay will also free the overlay buffer
}


void
ServerBitmap::Acquire()
{
	atomic_add(&fReferenceCount, 1);
}


void
ServerBitmap::Release()
{
	gBitmapManager->DeleteBitmap(this);
}


bool
ServerBitmap::_Release()
{
	if (atomic_add(&fReferenceCount, -1) == 1)
		return true;

	return false;
}


/*!	\brief Internal function used by subclasses

	Subclasses should call this so the buffer can automagically
	be allocated on the heap.
*/
void
ServerBitmap::_AllocateBuffer(void)
{
	uint32 length = BitsLength();
	if (length > 0) {
		delete[] fBuffer;
		fBuffer = new(nothrow) uint8[length];
	}
}


status_t
ServerBitmap::ImportBits(const void *bits, int32 bitsLength, int32 bytesPerRow,
	color_space colorSpace)
{
	if (!bits || bitsLength < 0 || bytesPerRow <= 0)
		return B_BAD_VALUE;

	return BPrivate::ConvertBits(bits, fBuffer, bitsLength, BitsLength(),
		bytesPerRow, fBytesPerRow, colorSpace, fSpace, fWidth, fHeight);
}


status_t
ServerBitmap::ImportBits(const void *bits, int32 bitsLength, int32 bytesPerRow,
	color_space colorSpace, BPoint from, BPoint to, int32 width, int32 height)
{
	if (!bits || bitsLength < 0 || bytesPerRow <= 0 || width < 0 || height < 0)
		return B_BAD_VALUE;

	return BPrivate::ConvertBits(bits, fBuffer, bitsLength, BitsLength(),
		bytesPerRow, fBytesPerRow, colorSpace, fSpace, from, to, width,
		height);
}


area_id
ServerBitmap::Area() const
{
	if (fAllocator != NULL)
		return fAllocator->Area(AllocationCookie());

	return B_ERROR;
}


uint32
ServerBitmap::AreaOffset() const
{
	if (fAllocator != NULL)
		return fAllocator->AreaOffset(AllocationCookie());

	return 0;
}


void
ServerBitmap::SetOverlay(::Overlay* overlay)
{
	fOverlay = overlay;
}


::Overlay*
ServerBitmap::Overlay() const
{
	return fOverlay;
}


bool
ServerBitmap::SetOwner(ServerApp* owner)
{
	if (fOwner != NULL)
		fOwner->BitmapRemoved(this);

	if (owner != NULL && owner->BitmapAdded(this)) {
		fOwner = owner;
		return true;
	}

	return false;
}


ServerApp*
ServerBitmap::Owner() const
{
	return fOwner;
}


void
ServerBitmap::PrintToStream()
{
	printf("Bitmap@%p: (%ld:%ld), space %ld, bpr %ld, buffer %p\n",
		this, fWidth, fHeight, (int32)fSpace, fBytesPerRow, fBuffer);
}


//	#pragma mark -


UtilityBitmap::UtilityBitmap(BRect rect, color_space space, uint32 flags,
		int32 bytesperline, screen_id screen)
	: ServerBitmap(rect, space, flags, bytesperline, screen)
{
	_AllocateBuffer();
}


UtilityBitmap::UtilityBitmap(const ServerBitmap* bitmap)
	: ServerBitmap(bitmap)
{
	_AllocateBuffer();

	if (bitmap->Bits())
		memcpy(Bits(), bitmap->Bits(), bitmap->BitsLength());
}


UtilityBitmap::UtilityBitmap(const uint8* alreadyPaddedData, uint32 width,
		uint32 height, color_space format)
	: ServerBitmap(BRect(0, 0, width - 1, height - 1), format, 0)
{
	_AllocateBuffer();
	if (Bits())
		memcpy(Bits(), alreadyPaddedData, BitsLength());
}


UtilityBitmap::~UtilityBitmap()
{
}
