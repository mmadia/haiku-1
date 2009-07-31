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
		B_ANY_FORMAT_FAMILY,
		B_MEDIA_RAW_VIDEO,
		B_MEDIA_ENCODED_VIDEO
	},
	{
		{
			"MP3 Audio",
			"mp3",
			0,
			CODEC_ID_MP3,
			{ 0 }
		},
		B_ANY_FORMAT_FAMILY,
		B_MEDIA_RAW_AUDIO,
		B_MEDIA_ENCODED_AUDIO
	}
};

const size_t gEncoderCount = sizeof(gEncoderTable) / sizeof(EncoderDescription);
