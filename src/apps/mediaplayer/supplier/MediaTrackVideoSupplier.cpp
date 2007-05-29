/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "MediaTrackVideoSupplier.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <MediaTrack.h>

using std::nothrow;

#define DEBUG_DECODED_FRAME 0
#if DEBUG_DECODED_FRAME
#  include <Bitmap.h>
#  include <BitmapStream.h>
#  include <File.h>
#  include <TranslatorRoster.h>
#endif // DEBUG_DECODED_FRAME

static const char* string_for_color_space(color_space format);


// constructor
MediaTrackVideoSupplier::MediaTrackVideoSupplier(BMediaTrack* track,
		color_space format)
	: VideoSupplier()
	, fVideoTrack(track)

	, fPerformanceTime(0)
	, fDuration(0)
{
	if (!fVideoTrack) {
		printf("MediaTrackVideoSupplier() - no video track\n");
		return;
	}

	// get the encoded format
	memset(&fFormat, 0, sizeof(media_format));
	status_t ret = fVideoTrack->EncodedFormat(&fFormat);
	if (ret < B_OK) {
		printf("MediaTrackVideoSupplier::InitCheck() - "
			"fVideoTrack->EncodedFormat(): %s\n", strerror(ret));
		return;
	}

	// get ouput video frame size
	uint32 width = fFormat.u.encoded_video.output.display.line_width;
	uint32 height = fFormat.u.encoded_video.output.display.line_count;

	// specifiy the decoded format. we derive this information from
	// the encoded format (width & height).
	memset(&fFormat, 0, sizeof(media_format));
//	fFormat.u.raw_video.last_active = height - 1;
//	fFormat.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
//	fFormat.u.raw_video.pixel_width_aspect = 1;
//	fFormat.u.raw_video.pixel_height_aspect = 1;
	fFormat.u.raw_video.display.format = format;
	fFormat.u.raw_video.display.line_width = width;
	fFormat.u.raw_video.display.line_count = height;
	if (format == B_RGB32 || format == B_RGBA32)
		fFormat.u.raw_video.display.bytes_per_row = width * 4;
	else if (format == B_YCbCr422)
		fFormat.u.raw_video.display.bytes_per_row = ((width * 2 + 3) / 4) * 4;

	ret = fVideoTrack->DecodedFormat(&fFormat);

	if (ret < B_OK) {
		printf("MediaTrackVideoSupplier() - "
			"fVideoTrack->DecodedFormat(): %s\n", strerror(ret));
		return;
	}

	if (fFormat.u.raw_video.display.format != format) {
		printf("MediaTrackVideoSupplier() - "
			" codec changed colorspace of decoded format (%s -> %s)!\n"
			"    this is bad for performance, since colorspace conversion\n"
			"    needs to happen during playback.\n",
			string_for_color_space(format),
			string_for_color_space(fFormat.u.raw_video.display.format));
		// check if the codec forgot to adjust bytes_per_row
		uint32 minBPR;
		format = fFormat.u.raw_video.display.format;
		if (format == B_YCbCr422)
			minBPR = ((width * 2 + 3) / 4) * 4;
		else
			minBPR = width * 4;
		if (minBPR != fFormat.u.raw_video.display.bytes_per_row) {
			printf("  -> stupid codec forgot to adjust bytes_per_row!\n");
			fFormat.u.raw_video.display.bytes_per_row = minBPR;
			fVideoTrack->DecodedFormat(&fFormat);
		}
	}	

	fDuration = fVideoTrack->Duration();

//	for (bigtime_t time = 0; time < fDuration; time += 10000) {
//		bigtime_t keyFrameTime = time;
//		fVideoTrack->FindKeyFrameForTime(&keyFrameTime,
//			B_MEDIA_SEEK_CLOSEST_BACKWARD);
//		printf("keyframe time for time: %lld -> %lld\n", time, keyFrameTime);
//	}
}

// destructor
MediaTrackVideoSupplier::~MediaTrackVideoSupplier()
{
}


media_format
MediaTrackVideoSupplier::Format() const
{
	return fFormat;
}


status_t
MediaTrackVideoSupplier::ReadFrame(void* buffer, bigtime_t* performanceTime)
{
	if (!fVideoTrack)
		return B_NO_INIT;
	if (!buffer)
		return B_BAD_VALUE;

	// read a frame
	int64 frameCount = 1;
	// TODO: how does this work for interlaced video (field count > 1)?
	media_header mediaHeader;
	status_t ret = fVideoTrack->ReadFrames(buffer, &frameCount, &mediaHeader);

	if (ret < B_OK) {
		printf("MediaTrackVideoSupplier::ReadFrame() - "
			"error while reading frame of track: %s\n", strerror(ret));
	} else {
		fPerformanceTime = mediaHeader.start_time;
	}

	if (performanceTime)
		*performanceTime = fPerformanceTime;

#if DEBUG_DECODED_FRAME
if (modifiers() & B_SHIFT_KEY) {
BFile fileStream("/boot/home/Desktop/decoded.png", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
BTranslatorRoster* roster = BTranslatorRoster::Default();
BBitmap* bitmap = new BBitmap(Bounds(), 0, ColorSpace(), BytesPerRow());
memcpy(bitmap->Bits(), buffer, bitmap->BitsLength());
BBitmapStream bitmapStream(bitmap);
roster->Translate(&bitmapStream, NULL, NULL, &fileStream, B_PNG_FORMAT, 0);
bitmapStream.DetachBitmap(&bitmap);
delete bitmap;
}
#endif // DEBUG_DECODED_FRAME

	return ret;
}


status_t
MediaTrackVideoSupplier::SeekToTime(bigtime_t* performanceTime)
{
	if (!fVideoTrack)
		return B_NO_INIT;

bigtime_t _performanceTime = *performanceTime;
	status_t ret = fVideoTrack->SeekToTime(performanceTime);
	if (ret == B_OK) {
printf("seeked: %lld -> %lld\n", _performanceTime, *performanceTime);
		fPerformanceTime = *performanceTime;
	}

	return ret;
}


// #pragma mark -


BRect
MediaTrackVideoSupplier::Bounds() const
{
	return BRect(0, 0, 	fFormat.u.raw_video.display.line_width - 1,
		fFormat.u.raw_video.display.line_count - 1);
}


color_space
MediaTrackVideoSupplier::ColorSpace() const
{
	return fFormat.u.raw_video.display.format;
}


uint32
MediaTrackVideoSupplier::BytesPerRow() const
{
	return fFormat.u.raw_video.display.bytes_per_row;
}


// #pragma mark -


const char*
string_for_color_space(color_space format)
{
	const char* name = "<unkown format>";
	switch (format) {
		case B_RGB32:
			name = "B_RGB32";
			break;
		case B_RGBA32:
			name = "B_RGBA32";
			break;
		case B_RGB32_BIG:
			name = "B_RGB32_BIG";
			break;
		case B_RGBA32_BIG:
			name = "B_RGBA32_BIG";
			break;
		case B_RGB24:
			name = "B_RGB24";
			break;
		case B_RGB24_BIG:
			name = "B_RGB24_BIG";
			break;
		case B_CMAP8:
			name = "B_CMAP8";
			break;
		case B_GRAY8:
			name = "B_GRAY8";
			break;
		case B_GRAY1:
			name = "B_GRAY1";
			break;

		// YCbCr
		case B_YCbCr422:
			name = "B_YCbCr422";
			break;
		case B_YCbCr411:
			name = "B_YCbCr411";
			break;
		case B_YCbCr444:
			name = "B_YCbCr444";
			break;
		case B_YCbCr420:
			name = "B_YCbCr420";
			break;

		// YUV
		case B_YUV422:
			name = "B_YUV422";
			break;
		case B_YUV411:
			name = "B_YUV411";
			break;
		case B_YUV444:
			name = "B_YUV444";
			break;
		case B_YUV420:
			name = "B_YUV420";
			break;

		case B_YUV9:
			name = "B_YUV9";
			break;
		case B_YUV12:
			name = "B_YUV12";
			break;

		default:
			break;
	}
	return name;
}
