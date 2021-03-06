#ifndef _OGG_SPEEX_STREAM_H
#define _OGG_SPEEX_STREAM_H

#include "OggStream.h"

namespace BPrivate { namespace media {

class OggSpeexStream : public OggStream {
public:
	static bool	IsValidHeader(const ogg_packet & packet);
public:
				OggSpeexStream(long serialno);
	virtual		~OggSpeexStream();

	virtual	status_t	GetStreamInfo(int64 *frameCount, bigtime_t *duration,
						              media_format *format);
	virtual status_t	GetNextChunk(const void **chunkBuffer, size_t *chunkSize,
						             media_header *mediaHeader);

};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_SPEEX_STREAM_H
