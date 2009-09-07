/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 *  All rights reserved. Distributed under the terms of the MIT License.
 */

#include "UnzipEngine.h"

#include <new>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

#include <Directory.h>
#include <fs_attr.h>
#include <NodeInfo.h>
#include <Path.h>
#include <String.h>
#include <SymLink.h>

#include "CommandPipe.h"
#include "SemaphoreLocker.h"
#include "ProgressReporter.h"


using std::nothrow;


UnzipEngine::UnzipEngine(ProgressReporter* reporter,
		sem_id cancelSemaphore)
	:
	fPackage(""),
	fRetrievingListing(false),

	fBytesToUncompress(0),
	fBytesUncompressed(0),
	fLastBytesUncompressed(0),
	fItemsToUncompress(0),
	fItemsUncompressed(0),
	fLastItemsUncompressed(0),

	fProgressReporter(reporter),
	fCancelSemaphore(cancelSemaphore)
{
}


UnzipEngine::~UnzipEngine()
{
}


status_t
UnzipEngine::SetTo(const char* pathToPackage, const char* destinationFolder)
{
	fPackage = pathToPackage;
	fDestinationFolder = destinationFolder;

	fEntrySizeMap.Clear();

	fBytesToUncompress = 0;
	fBytesUncompressed = 0;
	fLastBytesUncompressed = 0;
	fItemsToUncompress = 0;
	fItemsUncompressed = 0;
	fLastItemsUncompressed = 0;

	BPrivate::BCommandPipe commandPipe;
	status_t ret = commandPipe.AddArg("unzip");
	if (ret == B_OK)
		ret = commandPipe.AddArg("-l");
	if (ret == B_OK)
		ret = commandPipe.AddArg(fPackage.String());
	if (ret != B_OK)
		return ret;

	// Launch the unzip thread and start reading the stdout and stderr output.
	FILE* stdOutAndErrPipe = NULL;
	thread_id unzipThread = commandPipe.PipeInto(&stdOutAndErrPipe);
	if (unzipThread < 0)
		return (status_t)unzipThread;

	fRetrievingListing = true;
	ret = commandPipe.ReadLines(stdOutAndErrPipe, this);
	fRetrievingListing = false;

	printf("%llu items in %llu bytes\n", fItemsToUncompress,
		fBytesToUncompress);

	return ret;
}


status_t
UnzipEngine::UnzipPackage()
{
	if (fItemsToUncompress == 0)
		return B_NO_INIT;

	BPrivate::BCommandPipe commandPipe;
	status_t ret = commandPipe.AddArg("unzip");
	if (ret == B_OK)
		ret = commandPipe.AddArg("-o");
	if (ret == B_OK)
		ret = commandPipe.AddArg(fPackage.String());
	if (ret == B_OK)
		ret = commandPipe.AddArg("-d");
	if (ret == B_OK)
		ret = commandPipe.AddArg(fDestinationFolder.String());
	if (ret == B_OK)
		ret = commandPipe.AddArg("-x");
	if (ret == B_OK)
		ret = commandPipe.AddArg(".OptionalPackageDescription");
	if (ret != B_OK)
		return ret;

	// Launch the unzip thread and start reading the stdout and stderr output.
	FILE* stdOutAndErrPipe = NULL;
	thread_id unzipThread = commandPipe.PipeInto(&stdOutAndErrPipe);
	if (unzipThread < 0)
		return (status_t)unzipThread;

	return commandPipe.ReadLines(stdOutAndErrPipe, this);
}


// #pragma mark -


bool
UnzipEngine::IsCanceled()
{
	if (fCancelSemaphore < 0)
		return false;

	SemaphoreLocker locker(fCancelSemaphore);
	return !locker.IsLocked();
}


status_t
UnzipEngine::ReadLine(const BString& line)
{
	if (fRetrievingListing)
		return _ReadLineListing(line);
	else
		return _ReadLineExtract(line);
}


status_t
UnzipEngine::_ReadLineListing(const BString& line)
{
	static const char* kListingFormat = "%llu  %s %s   %s\n";

	const char* string = line.String();
	while (string[0] == ' ')
		string++;

	uint64 bytes;
	char date[16];
	char time[16];
	char path[1024];
	if (sscanf(string, kListingFormat, &bytes, &date, &time, &path) == 4) {
		fBytesToUncompress += bytes;

		BString itemPath(path);
		BString itemName(path);
		int leafPos = itemPath.FindLast('/');
		if (leafPos >= 0)
			itemName = itemPath.String() + leafPos + 1;

		// We check if the target folder exists and don't increment
		// the item count in that case. Unzip won't report on folders that did
		// not need to be created. This may mess up our current item count.
		uint32 itemCount = 1;
		if (bytes == 0 && itemName.Length() == 0) {
			// a folder?
			BPath destination(fDestinationFolder.String());
			if (destination.Append(itemPath.String()) == B_OK) {
				BEntry test(destination.Path());
				if (test.Exists() && test.IsDirectory()) {
//					printf("ignoring %s\n", itemPath.String());
					itemCount = 0;
				}
			}
		}

		fItemsToUncompress += itemCount;

//		printf("item %s with %llu bytes to %s\n", itemName.String(),
//			bytes, itemPath.String());

		fEntrySizeMap.Put(itemName.String(), bytes);
	} else {
//		printf("listing not understood: %s", string);
	}

	return B_OK;
}


status_t
UnzipEngine::_ReadLineExtract(const BString& line)
{
	char item[1024];
	char linkTarget[256];
	const char* kCreatingFormat = "    creating: %s\n";
	const char* kInflatingFormat = "   inflating: %s\n";
	const char* kLinkingFormat = "     linking: %s -> %s\n";
	if (sscanf(line.String(), kCreatingFormat, &item) == 1
		|| sscanf(line.String(), kInflatingFormat, &item) == 1
		|| sscanf(line.String(), kLinkingFormat, &item,
			&linkTarget) == 2) {

		fItemsUncompressed++;

		BString itemPath(item);
		int pos = itemPath.FindLast('/');
		BString itemName = itemPath.String() + pos + 1;
		itemPath.Truncate(pos);

		off_t bytes = 0;
		if (fEntrySizeMap.ContainsKey(itemName.String())) {
			bytes = fEntrySizeMap.Get(itemName.String());
			fBytesUncompressed += bytes;
		}

//		printf("%llu extracted %s to %s (%llu)\n", fItemsUncompressed,
//			itemName.String(), itemPath.String(), bytes);

		_UpdateProgress(itemName.String(), itemPath.String());
	} else {
//		printf("ignored: %s", line.String());
	}

	return B_OK;
}


void
UnzipEngine::_UpdateProgress(const char* item, const char* targetFolder)
{
	if (fProgressReporter == NULL)
		return;

	uint64 items = 0;
	if (fLastItemsUncompressed < fItemsUncompressed) {
		items = fItemsUncompressed - fLastItemsUncompressed;
		fLastItemsUncompressed = fItemsUncompressed;
	}

	off_t bytes = 0;
	if (fLastBytesUncompressed < fBytesUncompressed) {
		bytes = fBytesUncompressed - fLastBytesUncompressed;
		fLastBytesUncompressed = fBytesUncompressed;
	}

	fProgressReporter->ItemsWritten(items, bytes, item, targetFolder);
}