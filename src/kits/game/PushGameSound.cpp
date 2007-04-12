//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		GameSound.cpp
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	BPushGameSound class
//------------------------------------------------------------------------------

#include <string.h>

#include <List.h>
#include <PushGameSound.h>

#include "GSUtility.h"


BPushGameSound::BPushGameSound(size_t inBufferFrameCount, const gs_audio_format *format,
			size_t inBufferCount, BGameSoundDevice *device)
 	:	BStreamingGameSound(inBufferFrameCount, format, inBufferCount, device)
{
	if (InitCheck() != B_OK)
		return;
	
	status_t error = SetParameters(inBufferFrameCount, format, inBufferCount);
	if (error == B_OK)
		fPageLocked = new BList;
	else
		SetInitError(error);
}


BPushGameSound::BPushGameSound(BGameSoundDevice * device)
		:	BStreamingGameSound(device),
			fLockPos(0),
			fPlayPos(0),
			fBuffer(NULL),
			fPageSize(0),
			fPageCount(0),
			fBufferSize(0)
{
	fPageLocked = new BList;
}
		
BPushGameSound::~BPushGameSound()
{
	delete [] fBuffer;
	delete fPageLocked;
}


BPushGameSound::lock_status
BPushGameSound::LockNextPage(void **out_pagePtr, size_t *out_pageSize)
{	
	// the user can not lock every page
	if (fPageLocked->CountItems() > fPageCount - 3) 
		return lock_failed;
	
	// the user cann't lock a page being played
	if (fLockPos < fPlayPos
		&& fLockPos + fPageSize > fPlayPos) 
		return lock_failed;
	
	// lock the page
	char * lockPage = &fBuffer[fLockPos];
	fPageLocked->AddItem(lockPage);
	
	// move the locker to the next page
	fLockPos += fPageSize;
	if (fLockPos > fBufferSize) 
		fLockPos = 0;
	
	*out_pagePtr = lockPage;
	*out_pageSize = fPageSize;
	
	return lock_ok;
}


status_t
BPushGameSound::UnlockPage(void *in_pagePtr)
{
	return (fPageLocked->RemoveItem(in_pagePtr)) ? B_OK : B_ERROR;
}


BPushGameSound::lock_status
BPushGameSound::LockForCyclic(void **out_basePtr, size_t *out_size)
{
	*out_basePtr = fBuffer;
	*out_size = fBufferSize;
	return lock_ok;
}


status_t
BPushGameSound::UnlockCyclic()
{
	return B_OK;
}


size_t
BPushGameSound::CurrentPosition()
{
	return fPlayPos;
}


BGameSound *
BPushGameSound::Clone() const
{
	gs_audio_format format = Format();
	size_t frameSize = get_sample_size(format.format) * format.channel_count;
	size_t bufferFrameCount = fPageSize / frameSize;
	
	return new BPushGameSound(bufferFrameCount, &format, fPageCount, Device());
}


status_t
BPushGameSound::Perform(int32 selector, void *data)
{
	return B_ERROR;
}


status_t
BPushGameSound::SetParameters(size_t inBufferFrameCount,
							  const gs_audio_format *format,
							  size_t inBufferCount)
{
	status_t error = BStreamingGameSound::SetParameters(inBufferFrameCount, format, inBufferCount);
	if (error != B_OK)
		return error;
	
	size_t frameSize = get_sample_size(format->format) * format->channel_count;
	
	fPageCount = inBufferCount;
	fPageSize = frameSize * inBufferFrameCount;	
	fBufferSize = fPageSize * fPageCount;
	
	fBuffer = new char[fBufferSize];
	
	return B_OK;
}


status_t
BPushGameSound::SetStreamHook(void (*hook)(void * inCookie, void * inBuffer, size_t inByteCount, BStreamingGameSound * me),
							  void * cookie)
{
	return B_ERROR;
}


void
BPushGameSound::FillBuffer(void *inBuffer, size_t inByteCount)
{	
	size_t bytes = inByteCount;
	
	if (!BytesReady(&bytes))
		return;
	
	if (fPlayPos + bytes > fBufferSize) {
		size_t remainder = fPlayPos + bytes - fBufferSize;
		char * buffer = (char*)inBuffer;
		
		// fill the buffer with the samples left at the end of our buffer
		memcpy(buffer, &fBuffer[fPlayPos], remainder);
		fPlayPos = 0;
		
		// fill the remainder of the buffer by looping to the start
		// of the buffer if it isn't locked
		bytes -= remainder;
		if (BytesReady(&bytes)) {
			memcpy(&buffer[remainder], fBuffer, bytes);
			fPlayPos += bytes;
		}	
	} else {
		memcpy(inBuffer, &fBuffer[fPlayPos], bytes);
		fPlayPos += bytes;
	}
	
	BStreamingGameSound::FillBuffer(inBuffer, inByteCount);
}


bool
BPushGameSound::BytesReady(size_t * bytes)
{	
	if (fPageLocked->CountItems() <= 0)
		return true;
	
	size_t start = fPlayPos;
	size_t ready = fPlayPos;
	int32 page = int32(start / fPageSize);
	
	// return if there is nothing to do
	if (fPageLocked->HasItem(&fBuffer[page * fPageSize])) 
		return false;
	
	while (ready < *bytes) {
		ready += fPageSize;
		page = int32(ready / fPageSize);
		
		if (fPageLocked->HasItem(&fBuffer[page * fPageSize])) {
			// we have found a locked page
			*bytes = ready - start - (ready - page * fPageSize);
			return true;
		}
	}	
	
	// all of the bytes are ready
	return true;
}

/* unimplemented for protection of the user:
 *
 * BPushGameSound::BPushGameSound()
 * BPushGameSound::BPushGameSound(const BPushGameSound &)
 * BPushGameSound &BPushGameSound::operator=(const BPushGameSound &)
 */


status_t
BPushGameSound::_Reserved_BPushGameSound_0(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_1(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_2(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_3(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_4(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_5(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_6(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_7(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_8(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_9(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_10(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_11(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_12(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_13(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_14(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_15(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_16(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_17(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_18(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_19(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_20(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_21(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_22(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_23(int32 arg, ...)
{
	return B_ERROR;
}


