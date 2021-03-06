/*
 * Copyright © 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */
#include "ProxyVideoSupplier.h"

#include <stdio.h>

#include <Autolock.h>

#include "VideoTrackSupplier.h"


ProxyVideoSupplier::ProxyVideoSupplier()
	: fSupplierLock("video supplier lock")
	, fSupplier(NULL)
{
}


ProxyVideoSupplier::~ProxyVideoSupplier()
{
}


status_t
ProxyVideoSupplier::FillBuffer(int64 startFrame, void* buffer,
	const media_format* format, bool& wasCached)
{
	BAutolock _(fSupplierLock);
//printf("ProxyVideoSupplier::FillBuffer(%lld)\n", startFrame);
	if (fSupplier == NULL)
		return B_NO_INIT;

	bigtime_t performanceTime = 0;
	if (fSupplier->CurrentFrame() == startFrame + 1) {
		printf("ProxyVideoSupplier::FillBuffer(%lld) - Could re-use previous "
			"buffer!\n", startFrame);
	}
	if (fSupplier->CurrentFrame() != startFrame) {
		int64 frame = startFrame;
		status_t ret = fSupplier->SeekToFrame(&frame);
		if (ret != B_OK)
			return ret;
		while (frame < startFrame) {
			ret = fSupplier->ReadFrame(buffer, &performanceTime, format,
				wasCached);
			if (ret != B_OK)
				return ret;
			frame++;
		}
	}

	// TODO: cache into intermediate buffer to handle the
	// currentFrame = startFrame + 1 case!

	return fSupplier->ReadFrame(buffer, &performanceTime, format, wasCached);
}


void
ProxyVideoSupplier::DeleteCaches()
{
}


void
ProxyVideoSupplier::SetSupplier(VideoTrackSupplier* supplier)
{
	BAutolock _(fSupplierLock);

	fSupplier = supplier;
}

