#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <DataIO.h>
#include <ByteOrder.h>
#include <InterfaceDefs.h>
#include <MediaFormats.h>
#include "RawFormats.h"
#include "avi_reader.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

// http://www.microsoft.com/Developer/PRODINFO/directx/dxm/help/ds/FiltDev/DV_Data_AVI_File_Format.htm

struct avi_cookie
{
	int 	stream;
	char *	buffer;
	int		buffer_size;

	bool	audio;
	
	int64	byte_pos;
	uint32	bytes_per_sec_rate;
	uint32	bytes_per_sec_scale;

	uint32	frame_pos;
};


aviReader::aviReader()
 :	fFile(0)
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
	
	fFile = new OpenDMLFile();
	if (B_OK != fFile->SetTo(pos_io_source)) {
		TRACE("aviReader::Sniff: can't setup OpenDMLFile\n");
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

	return B_OK;
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
						 media_format *format, void **infoBuffer, int32 *infoSize)
{
	avi_cookie *cookie = (avi_cookie *)_cookie;

	*duration = fFile->Duration();
	*infoBuffer = 0;
	*infoSize = 0;

	BMediaFormats formats;
	media_format_description description;
	
	const avi_stream_header *stream_header;
	stream_header = fFile->StreamFormat(cookie->stream);
	if (!stream_header) {
		TRACE("aviReader::GetStreamInfo: stream %d has no header\n", cookie->stream);
		return B_ERROR;
	}

	if (fFile->IsAudio(cookie->stream)) {
		const wave_format_ex *audio_format = fFile->AudioFormat(cookie->stream);
		if (!audio_format) {
			TRACE("aviReader::GetStreamInfo: audio stream %d has no format\n", cookie->stream);
			return B_ERROR;
		}
		
		if (audio_format->format_tag == 0x0001) // PCM
			*frameCount = stream_header->length / ((stream_header->sample_size + 7) / 8);
		else // not PCM
			*frameCount = (stream_header->length * audio_format->frames_per_sec) / (stream_header->sample_size * audio_format->avg_bytes_per_sec);
		
		cookie->audio = true;
		cookie->byte_pos = 0;
		
		cookie->bytes_per_sec_rate = audio_format->avg_bytes_per_sec;
		cookie->bytes_per_sec_scale = 1;

		if (audio_format->format_tag == 0x0001) {
			// a raw PCM format
			description.family = B_BEOS_FORMAT_FAMILY;
			description.u.beos.format = B_BEOS_FORMAT_RAW_AUDIO;
			if (B_OK != formats.GetFormatFor(description, format)) 
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
				TRACE("WavReader::AllocateCookie: unhandled bits per sample %d\n", audio_format->bits_per_sample);
				return B_ERROR;
			}
			format->u.raw_audio.format |= B_AUDIO_FORMAT_CHANNEL_ORDER_WAVE;
			format->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
			format->u.raw_audio.buffer_size = stream_header->suggested_buffer_size;
		} else {
			// some encoded format
			description.family = B_WAV_FORMAT_FAMILY;
			description.u.wav.codec = audio_format->format_tag;
			if (B_OK != formats.GetFormatFor(description, format)) 
				format->type = B_MEDIA_ENCODED_AUDIO;
			format->u.encoded_audio.output.frame_rate = audio_format->frames_per_sec;
			format->u.encoded_audio.output.channel_count = audio_format->channels;
		}
		return B_OK;
	}

	if (fFile->IsVideo(cookie->stream)) {
		const bitmap_info_header *video_format = fFile->VideoFormat(cookie->stream);
		if (!video_format) {
			TRACE("aviReader::GetStreamInfo: video stream %d has no format\n", cookie->stream);
			return B_ERROR;
		}
		
		*frameCount = fFile->FrameCount();
		*duration = fFile->Duration();

		cookie->audio = false;
		cookie->frame_pos = 0;

		description.family = B_AVI_FORMAT_FAMILY;
		description.u.avi.codec = 0;
		if (B_OK != formats.GetFormatFor(description, format)) 
			format->type = B_MEDIA_ENCODED_VIDEO;
		format->user_data_type = B_CODEC_TYPE_INFO;
		strncpy((char*)format->user_data, "DivX", 4);

		return B_OK;
	}

	return B_ERROR;
}


status_t
aviReader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{

	return B_OK;
}


status_t
aviReader::GetNextChunk(void *_cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
	avi_cookie *cookie = (avi_cookie *)_cookie;

	int64 start; uint32 size; bool keyframe;
	if (!fFile->GetNextChunkInfo(cookie->stream, &start, &size, &keyframe))
		return B_LAST_BUFFER_ERROR;

	if (cookie->buffer_size < size) {
		delete [] cookie->buffer;
		cookie->buffer_size = (size + 15) & ~15;
		cookie->buffer = new char [cookie->buffer_size];
	}
	
	if (cookie->audio) {
		mediaHeader->start_time = (cookie->byte_pos * 1000000ULL * cookie->bytes_per_sec_scale) / cookie->bytes_per_sec_rate;
		
		cookie->byte_pos += size;
	} else {
		cookie->frame_pos += 1;
	}
	
	printf("stream %d: start_time %.6f\n", cookie->stream, mediaHeader->start_time / 1000000.0);

	*chunkBuffer = cookie->buffer;
	*chunkSize = size;
	return size == fFile->Source()->ReadAt(start, cookie->buffer, size) ? B_OK : B_LAST_BUFFER_ERROR;
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
