/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "MusePackDecoder.h"
#include "mpc/in_mpc.h"


MusePackDecoder::MusePackDecoder()
{
}


MusePackDecoder::~MusePackDecoder()
{
	delete fDecoder;
}


status_t 
MusePackDecoder::Setup(media_format *ioEncodedFormat, const void *infoBuffer, int32 infoSize)
{
	if (infoBuffer == NULL || infoSize != sizeof(MPC_decoder))
		return B_BAD_VALUE;

	fDecoder = (MPC_decoder *)infoBuffer;
	return B_OK;
}


status_t 
MusePackDecoder::NegotiateOutputFormat(media_format *format)
{
	// ToDo: rework the underlying MPC engine to be more flexible

	format->type = B_MEDIA_RAW_AUDIO;
	format->u.raw_audio.frame_rate = fDecoder->fInfo->simple.SampleFreq;
	format->u.raw_audio.channel_count = fDecoder->fInfo->simple.Channels;
	format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	format->u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
	format->u.raw_audio.buffer_size = sizeof(MPC_SAMPLE_FORMAT) * FRAMELEN * 2 * 2;
		// the buffer size is hardcoded in the MPC engine...

	if (format->u.raw_audio.channel_mask == 0) {
		format->u.raw_audio.channel_mask = (format->u.raw_audio.channel_count == 1) ?
			B_CHANNEL_LEFT : B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
	}

	return B_OK;
}


status_t 
MusePackDecoder::Seek(uint32 seekTo, int64 seekFrame, int64 *frame, bigtime_t seekTime, bigtime_t *time)
{
	return B_OK;
}


status_t 
MusePackDecoder::Decode(void *buffer, int64 *_frameCount, media_header *mediaHeader, media_decode_info *info)
{
	// ToDo: add error checking (check for broken frames)

	uint32 ring = fDecoder->Zaehler;
	*_frameCount = fDecoder->DECODE((MPC_SAMPLE_FORMAT *)buffer);
	fDecoder->UpdateBuffer(ring);

	return B_OK;
}

