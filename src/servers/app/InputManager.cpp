/*
 * Copyright 2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */

// TODO: introduce means to define event stream features (like local vs. net)
// TODO: introduce the possibility to identify a stream by a unique name


#include "EventStream.h"
#include "InputManager.h"

#include <Autolock.h>


InputManager* gInputManager;
	// the global input manager will be created by the AppServer


InputManager::InputManager()
	: BLocker("input manager"),
	fFreeStreams(2, true),
	fUsedStreams(2, true)
{
}


InputManager::~InputManager()
{
}


bool
InputManager::AddStream(EventStream* stream)
{
	BAutolock _(this);
printf("got stream: %p\n", stream);
	return fFreeStreams.AddItem(stream);
}


void
InputManager::RemoveStream(EventStream* stream)
{
	BAutolock _(this);
	fFreeStreams.RemoveItem(stream);
}


EventStream*
InputManager::GetStream()
{
	BAutolock _(this);
	
	EventStream* stream = NULL;
	do {
printf("remove invalid stream: %p\n", stream);
		delete stream;
			// this deletes the previous invalid stream

		stream = fFreeStreams.RemoveItemAt(0);
	} while (stream != NULL && !stream->IsValid());

	if (stream == NULL)
		return NULL;

	fUsedStreams.AddItem(stream);
printf("return stream: %p\n", stream);
	return stream;
}


void
InputManager::PutStream(EventStream* stream)
{
	if (stream == NULL)
		return;

	BAutolock _(this);

	fUsedStreams.RemoveItem(stream, false);
	if (stream->IsValid())
		fFreeStreams.AddItem(stream);
	else
		delete stream;
}

