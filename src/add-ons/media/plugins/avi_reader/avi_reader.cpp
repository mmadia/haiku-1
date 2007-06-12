/*
 * Copyright (c) 2004-2007, Marcus Overhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <DataIO.h>
#include <ByteOrder.h>
#include <InterfaceDefs.h>
#include <MediaFormats.h>
#include "RawFormats.h"
#include "avi_reader.h"

#define TRACE_AVI_READER
#ifdef TRACE_AVI_READER
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)

// http://web.archive.org/web/20030618161228/http://www.microsoft.com/Developer/PRODINFO/directx/dxm/help/ds/FiltDev/DV_Data_AVI_File_Format.htm
// http://mediaxw.sourceforge.net/files/doc/Video%20for%20Windows%20Reference%20-%20Chapter%204%20-%20AVI%20Files.pdf


struct avi_cookie
{
	unsigned	stream;
	char *		buffer;
	unsigned	buffer_size;

	bool		is_audio;
	bool		is_video;

	media_format format;

	bigtime_t 	duration;
	int64		frame_count;
	int64		frame_pos;
	uint32		frames_per_sec_rate;
	uint32		frames_per_sec_scale;
	
	// video only:
	uint32		line_count;
};


aviReader::aviReader()
 :	fFile(NULL)
{
	TRACE("aviReader::aviReader\n");
}


aviReader::~aviReader()
{
 	delete fFile;
}

      
const char *
aviReader::Copyright()
{
	return "AVI & OpenDML reader, " B_UTF8_COPYRIGHT " by Marcus Overhagen";
}

	
status_t
aviReader::Sniff(int32 *streamCount)
{
	TRACE("aviReader::Sniff\n");
	
	BPositionIO *pos_io_source;

	pos_io_source = dynamic_cast<BPositionIO *>(Reader::Source());
	if (!pos_io_source) {
		TRACE("aviReader::Sniff: not a BPositionIO\n");
		return B_ERROR;
	}
	
	if (!OpenDMLFile::IsSupported(pos_io_source)) {
		TRACE("aviReader::Sniff: unsupported file type\n");
		return B_ERROR;
	}
	
	TRACE("aviReader::Sniff: this stream seems to be supported\n");
	
	fFile = new OpenDMLFile(pos_io_source);
	if (fFile->Init() < B_OK) {
		ERROR("aviReader::Sniff: can't setup OpenDMLFile\n");
		return B_ERROR;
	}
	
	*streamCount = fFile->StreamCount();
	return B_OK;
}

void
aviReader::GetFileFormatInfo(media_file_format *mff)
{
	mff->capabilities =   media_file_format::B_READABLE
						| media_file_format::B_KNOWS_ENCODED_VIDEO
						| media_file_format::B_KNOWS_ENCODED_AUDIO
						| media_file_format::B_IMPERFECTLY_SEEKABLE;
	mff->family = B_MISC_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "audio/x-avi");
	strcpy(mff->file_extension, "avi");
	strcpy(mff->short_name,  "AVI");
	strcpy(mff->pretty_name, "Audio/Video Interleaved (AVI) file format");
}

status_t
aviReader::AllocateCookie(int32 streamNumber, void **_cookie)
{
	avi_cookie *cookie = new avi_cookie;
	*_cookie = cookie;
	
	cookie->stream = streamNumber;
	cookie->buffer = 0;
	cookie->buffer_size = 0;
	cookie->is_audio = false;
	cookie->is_video = false;

	BMediaFormats formats;
	media_format *format = &cookie->format;
	media_format_description description;
	
	const avi_stream_header *stream_header;
	stream_header = fFile->StreamFormat(cookie->stream);
	if (!stream_header) {
		ERROR("aviReader::GetStreamInfo: stream %d has no header\n", cookie->stream);
		delete cookie;
		return B_ERROR;
	}
	
	TRACE("aviReader::AllocateCookie: stream %ld (%s)\n", streamNumber, fFile->IsAudio(cookie->stream) ? "audio" : fFile->IsVideo(cookie->stream)  ? "video" : "unknown");

	if (fFile->IsAudio(cookie->stream)) {
		const wave_format_ex *audio_format = fFile->AudioFormat(cookie->stream);
		if (!audio_format) {
			ERROR("aviReader::GetStreamInfo: audio stream %d has no format\n", cookie->stream);
			delete cookie;
			return B_ERROR;
		}
		
		cookie->is_audio = true;
		cookie->duration = fFile->StreamInfo(streamNumber)->duration;
		cookie->frame_count = fFile->StreamInfo(streamNumber)->frame_count;
		cookie->frame_pos = 0;
		cookie->frames_per_sec_rate = fFile->StreamInfo(streamNumber)->frames_per_sec_rate;
		cookie->frames_per_sec_scale = fFile->StreamInfo(streamNumber)->frames_per_sec_scale;

		TRACE("audio frame_count %Ld\n", cookie->frame_count);
		TRACE("audio duration %.6f (%Ld)\n", cookie->duration / 1E6, cookie->duration);

		if (audio_format->format_tag == 0x0001) {
			// a raw PCM format
			description.family = B_BEOS_FORMAT_FAMILY;
			description.u.beos.format = B_BEOS_FORMAT_RAW_AUDIO;
			if (formats.GetFormatFor(description, format) < B_OK)
				format->type = B_MEDIA_RAW_AUDIO;
			format->u.raw_audio.frame_rate = audio_format->frames_per_sec;
			format->u.raw_audio.channel_count = audio_format->channels;
			if (audio_format->bits_per_sample <= 8)
				format->u.raw_audio.format = B_AUDIO_FORMAT_UINT8;
			else if (audio_format->bits_per_sample <= 16)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT16;
			else if (audio_format->bits_per_sample <= 24)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT24;
			else if (audio_format->bits_per_sample <= 32)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT32;
			else {
				ERROR("aviReader::AllocateCookie: unhandled bits per sample %d\n", audio_format->bits_per_sample);
				return B_ERROR;
			}
			format->u.raw_audio.format |= B_AUDIO_FORMAT_CHANNEL_ORDER_WAVE;
			format->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
			format->u.raw_audio.buffer_size = stream_header->suggested_buffer_size;
		} else {
			// some encoded format
			description.family = B_WAV_FORMAT_FAMILY;
			description.u.wav.codec = audio_format->format_tag;
			if (formats.GetFormatFor(description, format) < B_OK)
				format->type = B_MEDIA_ENCODED_AUDIO;
			format->u.encoded_audio.bit_rate = 8 * audio_format->avg_bytes_per_sec;
			TRACE("bit_rate %.3f\n", format->u.encoded_audio.bit_rate);
			format->u.encoded_audio.output.frame_rate = audio_format->frames_per_sec;
			format->u.encoded_audio.output.channel_count = audio_format->channels;
		}
		// TODO: this doesn't seem to work (it's not even a fourcc)
		format->user_data_type = B_CODEC_TYPE_INFO;
		*(uint32 *)format->user_data = audio_format->format_tag; format->user_data[4] = 0;
		
		// put the wave_format_ex struct, including extra data, into the format meta data.
		size_t size;
		const void *data = fFile->AudioFormat(cookie->stream, &size);
		format->SetMetaData(data, size);

#ifdef TRACE_AVI_READER
		uint8 *p = 18 + (uint8 *)data;
		TRACE("extra_data: %ld: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			size - 18, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
#endif
	
		return B_OK;
	}

	if (fFile->IsVideo(cookie->stream)) {
		const bitmap_info_header *video_format = fFile->VideoFormat(cookie->stream);
		if (!video_format) {
			ERROR("aviReader::GetStreamInfo: video stream %d has no format\n", cookie->stream);
			delete cookie;
			return B_ERROR;
		}
		
		cookie->is_video = true;
		cookie->duration = fFile->StreamInfo(streamNumber)->duration;
		cookie->frame_count = fFile->StreamInfo(streamNumber)->frame_count;
		cookie->frame_pos = 0;
		cookie->frames_per_sec_rate = fFile->StreamInfo(streamNumber)->frames_per_sec_rate;
		cookie->frames_per_sec_scale =  fFile->StreamInfo(streamNumber)->frames_per_sec_scale;
		cookie->line_count = fFile->AviMainHeader()->height;
		
		TRACE("video frame_count %Ld\n", cookie->frame_count);
		TRACE("video duration %.6f (%Ld)\n", cookie->duration / 1E6, cookie->duration);

		description.family = B_AVI_FORMAT_FAMILY;
		if (stream_header->fourcc_handler == 'ekaf' || stream_header->fourcc_handler == 0) // 'fake' or 0 fourcc => used compression id
			description.u.avi.codec = video_format->compression;
		else
			description.u.avi.codec = stream_header->fourcc_handler;
		if (formats.GetFormatFor(description, format) < B_OK)
			format->type = B_MEDIA_ENCODED_VIDEO;
			
		format->user_data_type = B_CODEC_TYPE_INFO;
		*(uint32 *)format->user_data = description.u.avi.codec; format->user_data[4] = 0;
		format->u.encoded_video.max_bit_rate = 8 * fFile->AviMainHeader()->max_bytes_per_sec;
		format->u.encoded_video.avg_bit_rate = (format->u.encoded_video.max_bit_rate * 3 / 4); // XXX fix this
		format->u.encoded_video.output.field_rate = cookie->frames_per_sec_rate / (float)cookie->frames_per_sec_scale;
		format->u.encoded_video.output.interlace = 1; // 1: progressive
		format->u.encoded_video.output.first_active = 0;
		format->u.encoded_video.output.last_active = cookie->line_count - 1;
		format->u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		format->u.encoded_video.output.pixel_width_aspect = 1;
		format->u.encoded_video.output.pixel_height_aspect = 1;
		// format->u.encoded_video.output.display.format = 0;
		format->u.encoded_video.output.display.line_width = fFile->AviMainHeader()->width;
		format->u.encoded_video.output.display.line_count = cookie->line_count;
		format->u.encoded_video.output.display.bytes_per_row = 0; // format->u.encoded_video.output.display.line_width * 4;
		format->u.encoded_video.output.display.pixel_offset = 0;
		format->u.encoded_video.output.display.line_offset = 0;
		format->u.encoded_video.output.display.flags = 0;
		
		TRACE("max_bit_rate %.3f\n", format->u.encoded_video.max_bit_rate);
		TRACE("field_rate   %.3f\n", format->u.encoded_video.output.field_rate);

		return B_OK;
	}

	delete cookie;
	return B_ERROR;
}


status_t
aviReader::FreeCookie(void *_cookie)
{
	avi_cookie *cookie = (avi_cookie *)_cookie;

	delete [] cookie->buffer;

	delete cookie;
	return B_OK;
}


status_t
aviReader::GetStreamInfo(void *_cookie, int64 *frameCount, bigtime_t *duration,
						 media_format *format, const void **infoBuffer, size_t *infoSize)
{
	avi_cookie *cookie = (avi_cookie *)_cookie;

	*frameCount = cookie->frame_count;
	*duration = cookie->duration;
	*format = cookie->format;
	*infoBuffer = 0;
	*infoSize = 0;
	return B_OK;
}


status_t
aviReader::Seek(void *_cookie, uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	avi_cookie *cookie = (avi_cookie *)_cookie;

	TRACE("aviReader::Seek: stream %d, seekTo%s%s%s%s, time %Ld, frame %Ld\n",
		cookie->stream,
		(seekTo & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(seekTo & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);

	status_t rv = fFile->Seek(cookie->stream, seekTo, frame, time);
	if (rv == B_OK) {
		cookie->frame_pos = *frame;
		TRACE("aviReader::Seek: stream %d, success, setting frame_pos to %lld\n", cookie->stream, cookie->frame_pos);
	}
	return rv;
}


status_t
aviReader::GetNextChunk(void *_cookie,
						const void **chunkBuffer, size_t *chunkSize,
						media_header *mediaHeader)
{
	avi_cookie *cookie = (avi_cookie *)_cookie;

	int64 start; uint32 size; bool keyframe;
	if (fFile->GetNextChunkInfo(cookie->stream, &start, &size, &keyframe) < B_OK)
		return B_LAST_BUFFER_ERROR;

	if (size > 0x200000) { // 2 MB
		ERROR("stream %d: frame too big: %u byte\n", size);
		return B_NO_MEMORY;
	}

	if (cookie->buffer_size < size) {
		delete [] cookie->buffer;
		cookie->buffer_size = (size + 15) & ~15;
		cookie->buffer = new char [cookie->buffer_size];
	}

	mediaHeader->start_time = (cookie->frame_pos * 1000000 * cookie->frames_per_sec_scale) / cookie->frames_per_sec_rate;
	
	if (cookie->is_audio) {
		mediaHeader->type = B_MEDIA_ENCODED_AUDIO;
		mediaHeader->u.encoded_audio.buffer_flags = keyframe ? B_MEDIA_KEY_FRAME : 0;
		cookie->frame_pos += size;
	} else if (cookie->is_video) {
		mediaHeader->type = B_MEDIA_ENCODED_VIDEO;
		mediaHeader->u.encoded_video.field_flags = keyframe ? B_MEDIA_KEY_FRAME : 0;
		mediaHeader->u.encoded_video.first_active_line = 0;
		mediaHeader->u.encoded_video.line_count = cookie->line_count;	
		cookie->frame_pos += 1;
	} else {
		return B_BAD_VALUE;
	}
	
	TRACE("stream %d (%s): start_time %.6f, pos %.3f %%\n", 
		cookie->stream, cookie->is_audio ? "A" : cookie->is_video ? "V" : "?", 
		mediaHeader->start_time / 1000000.0, cookie->frame_pos * 100.0 / cookie->frame_count);

	*chunkBuffer = cookie->buffer;
	*chunkSize = size;
	return (int)size == fFile->Source()->ReadAt(start, cookie->buffer, size) ? B_OK : B_LAST_BUFFER_ERROR;
}


Reader *
aviReaderPlugin::NewReader()
{
	return new aviReader;
}


MediaPlugin *instantiate_plugin()
{
	return new aviReaderPlugin;
}
