/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ModelLoader.h"

#include <stdio.h>
#include <string.h>

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <DebugEventStream.h>

#include <system_profiler_defs.h>

#include "DataSource.h"
#include "MessageCodes.h"
#include "Model.h"


ModelLoader::ModelLoader(DataSource* dataSource,
	const BMessenger& target, void* targetCookie)
	:
	fLock("main model loader"),
	fModel(NULL),
	fDataSource(dataSource),
	fTarget(target),
	fTargetCookie(targetCookie),
	fLoaderThread(-1),
	fLoading(false),
	fAborted(false)
{
}


ModelLoader::~ModelLoader()
{
	if (fLoaderThread >= 0) {
		Abort();
		wait_for_thread(fLoaderThread, NULL);
	}

	delete fDataSource;
	delete fModel;
}


status_t
ModelLoader::StartLoading()
{
	// check initialization
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	AutoLocker<BLocker> locker(fLock);

	if (fModel != NULL || fLoading || fDataSource == NULL)
		return B_BAD_VALUE;

	// create a model
	Model* model = new(std::nothrow) Model;
	if (model == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<Model> modelDeleter(model);

	// spawn the loader thread
	fLoaderThread = spawn_thread(&_LoaderEntry, "main model loader",
		B_NORMAL_PRIORITY, this);
	if (fLoaderThread < 0)
		return fLoaderThread;

	modelDeleter.Detach();
	fModel = model;

	fLoading = true;
	fAborted = false;

	resume_thread(fLoaderThread);

	return B_OK;
}


void
ModelLoader::Abort()
{
	AutoLocker<BLocker> locker(fLock);

	if (!fLoading || fAborted)
		return;

	fAborted = true;
}


Model*
ModelLoader::DetachModel()
{
	AutoLocker<BLocker> locker(fLock);

	if (fModel == NULL || fLoading)
		return NULL;

	Model* model = fModel;
	fModel = NULL;

	return model;
}


/*static*/ status_t
ModelLoader::_LoaderEntry(void* data)
{
	return ((ModelLoader*)data)->_Loader();
}


status_t
ModelLoader::_Loader()
{
	status_t error;
	try {
		error = _Load();
	} catch(...) {
		error = B_ERROR;
	}
	
	// clean up and notify the target
	AutoLocker<BLocker> locker(fLock);

	BMessage message;
	if (error == B_OK) {
		message.what = MSG_MODEL_LOADED_SUCCESSFULLY;
	} else {
		delete fModel;
		fModel = NULL;

		message.what = fAborted
			? MSG_MODEL_LOADED_ABORTED : MSG_MODEL_LOADED_FAILED;
	}

	message.AddPointer("loader", this);
	message.AddPointer("targetCookie", fTargetCookie);
	fTarget.SendMessage(&message);

	fLoading = false;

	return B_OK;
}


status_t
ModelLoader::_Load()
{
	// get a BDataIO from the data source
	BDataIO* io;
	status_t error = fDataSource->CreateDataIO(&io);
	if (error != B_OK)
		return error;
	ObjectDeleter<BDataIO> dataIOtDeleter(io);

	// create a debug input stream
	BDebugEventInputStream* input = new(std::nothrow) BDebugEventInputStream;
	if (input == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<BDebugEventInputStream> inputDeleter(input);

	error = input->SetTo(io);
	if (error != B_OK)
		return error;

	// process the events
	uint32 count = 0;

	while (true) {
		// get next event
		uint32 event;
		uint32 cpu;
		const void* buffer;
		ssize_t bufferSize = input->ReadNextEvent(&event, &cpu, &buffer);
		if (bufferSize < 0)
			return bufferSize;
		if (buffer == NULL)
			return B_OK;

		// process the event
		status_t error = _ProcessEvent(event, cpu, buffer, bufferSize);
		if (error != B_OK)
			return error;

		// periodically check whether we're supposed to abort
		if (++count % 32 == 0) {
			AutoLocker<BLocker> locker(fLock);
			if (fAborted)
				return B_ERROR;
		}
	}
}


status_t
ModelLoader::_ProcessEvent(uint32 event, uint32 cpu, const void* buffer,
	size_t size)
{
	switch (event) {
		case B_SYSTEM_PROFILER_TEAM_ADDED:
printf("B_SYSTEM_PROFILER_TEAM_ADDED: %lu\n", size);
			break;
		case B_SYSTEM_PROFILER_TEAM_REMOVED:
printf("B_SYSTEM_PROFILER_TEAM_REMOVED: %lu\n", size);
			break;
		case B_SYSTEM_PROFILER_TEAM_EXEC:
printf("B_SYSTEM_PROFILER_TEAM_EXEC: %lu\n", size);
			break;
		case B_SYSTEM_PROFILER_THREAD_ADDED:
printf("B_SYSTEM_PROFILER_THREAD_ADDED: %lu\n", size);
			break;
		case B_SYSTEM_PROFILER_THREAD_REMOVED:
printf("B_SYSTEM_PROFILER_THREAD_REMOVED: %lu\n", size);
			break;
		case B_SYSTEM_PROFILER_THREAD_SCHEDULED:
printf("B_SYSTEM_PROFILER_THREAD_SCHEDULED: %lu\n", size);
			break;
		case B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE:
printf("B_SYSTEM_PROFILER_THREAD_ENQUEUED_IN_RUN_QUEUE: %lu\n", size);
			break;
		case B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE:
printf("B_SYSTEM_PROFILER_THREAD_REMOVED_FROM_RUN_QUEUE: %lu\n", size);
			break;
		case B_SYSTEM_PROFILER_WAIT_OBJECT_INFO:
printf("B_SYSTEM_PROFILER_WAIT_OBJECT_INFO: %lu\n", size);
			break;
		default:
printf("unsupported event type %lu, size: %lu\n", event, size);
return B_BAD_DATA;
			break;
	}


	// TODO: Implement!
	return B_OK;
}