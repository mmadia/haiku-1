#ifndef _VORBIS_CODEC_PLUGIN_H_
#define _VORBIS_CODEC_PLUGIN_H_

#include "DecoderPlugin.h"

#include "libvorbis/vorbis/codec.h"

class VorbisDecoder : public Decoder
{
public:
				VorbisDecoder();
				~VorbisDecoder();
	
	void		GetCodecInfo(media_codec_info *info);
	status_t	Setup(media_format *inputFormat,
					  const void *infoBuffer, size_t infoSize);

	status_t	NegotiateOutputFormat(media_format *ioDecodedFormat);

	status_t	Seek(uint32 seekTo,
					 int64 seekFrame, int64 *frame,
					 bigtime_t seekTime, bigtime_t *time);

							 
	status_t	Decode(void *buffer, int64 *frameCount,
					   media_header *mediaHeader, media_decode_info *info);
					   
private:
	vorbis_info			fInfo;
	vorbis_comment		fComment;
	vorbis_dsp_state	fDspState;
	vorbis_block		fBlock;
	bigtime_t		fStartTime;
	int				fFrameSize;
	int				fOutputBufferSize;
};


class VorbisDecoderPlugin : public DecoderPlugin
{
public:
	Decoder *	NewDecoder(uint index);
	status_t	GetSupportedFormats(media_format ** formats, size_t * count);
};

#endif //_VORBIS_CODEC_PLUGIN_H_
