/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EncoderTable.h"

extern "C" {
	#include "avcodec.h"
}


const EncoderDescription gEncoderTable[] = {
	{
		{
			"MPEG4 Video",
			"mpeg4",
			0,
			CODEC_ID_MPEG4,
			{ 0 }
		},
		B_ANY_FORMAT_FAMILY, // TODO: Hm, actually not really /any/ family...
		B_MEDIA_RAW_VIDEO,
		B_MEDIA_ENCODED_VIDEO,
		23
	},
	{
		{
			"MPEG1 Video",
			"mpeg1video",
			0,
			CODEC_ID_MPEG1VIDEO,
			{ 0 }
		},
		B_MPEG_FORMAT_FAMILY,
		B_MEDIA_RAW_VIDEO,
		B_MEDIA_ENCODED_VIDEO,
		10
	},
	{
		{
			"MPEG2 Video",
			"mpeg2video",
			0,
			CODEC_ID_MPEG2VIDEO,
			{ 0 }
		},
		B_MPEG_FORMAT_FAMILY,
		B_MEDIA_RAW_VIDEO,
		B_MEDIA_ENCODED_VIDEO,
		15
	},
	{
		{
			"Raw Audio",
			"pcm",
			0,
			CODEC_ID_PCM_S16LE,
			{ 0 }
		},
		B_ANY_FORMAT_FAMILY,
		B_MEDIA_RAW_AUDIO,
		B_MEDIA_ENCODED_AUDIO,
		1
	},
	{
		{
			"Dolby Digital (AC-3)",
			"ac3",
			0,
			CODEC_ID_AC3,
			{ 0 }
		},
		B_ANY_FORMAT_FAMILY,
		B_MEDIA_RAW_AUDIO,
		B_MEDIA_ENCODED_AUDIO,
		10
	}
};

const size_t gEncoderCount = sizeof(gEncoderTable) / sizeof(EncoderDescription);

