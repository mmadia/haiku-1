/*
 * multiaudio replacement media addon for BeOS
 *
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "MultiAudioDevice.h"
#include "debug.h"
#include "driver_io.h"
#include <MediaDefs.h>
#include <string.h>

float SAMPLE_RATES[] = {
			8000.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0, 44100.0,
			48000.0, 64000.0, 88200.0, 96000.0, 176400.0, 192000.0, 384000.0, 1536000.0
			};


float MultiAudioDevice::convert_multiaudio_rate_to_media_rate(uint32 rate)
{
	uint8 count = 0;
	uint8 size = sizeof(SAMPLE_RATES)/sizeof(SAMPLE_RATES[0]);
	while(count < size) {
		if(rate & 1)
			return SAMPLE_RATES[count];
		count++;
		rate >>= 1;	
	}
	return 0.0;
}

uint32 MultiAudioDevice::convert_media_rate_to_multiaudio_rate(float rate)
{
	uint8 count = 0;
	uint size = sizeof(SAMPLE_RATES)/sizeof(SAMPLE_RATES[0]);
	while(count < size) {
		if(rate <= SAMPLE_RATES[count])
			return (0x1 << count);
		count++;
	}
	return 0;
}

uint32 MultiAudioDevice::convert_multiaudio_format_to_media_format(uint32 fmt)
{
	switch(fmt) {
		case B_FMT_FLOAT:
			return media_raw_audio_format::B_AUDIO_FLOAT;
		case B_FMT_18BIT:
		case B_FMT_20BIT:
		case B_FMT_24BIT:
		case B_FMT_32BIT:
			return media_raw_audio_format::B_AUDIO_INT;
		case B_FMT_16BIT:
			return media_raw_audio_format::B_AUDIO_SHORT;
		case B_FMT_8BIT_S:
			return media_raw_audio_format::B_AUDIO_CHAR;
		case B_FMT_8BIT_U:
			return media_raw_audio_format::B_AUDIO_UCHAR;
		default:
			return 0;
	}
}

uint32 MultiAudioDevice::convert_media_format_to_multiaudio_format(uint32 fmt)
{
	switch(fmt) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			return B_FMT_FLOAT;
		case media_raw_audio_format::B_AUDIO_INT:
			return B_FMT_32BIT;
		case media_raw_audio_format::B_AUDIO_SHORT:
			return B_FMT_16BIT;
		case media_raw_audio_format::B_AUDIO_CHAR:
			return B_FMT_8BIT_S;
		case media_raw_audio_format::B_AUDIO_UCHAR:
			return B_FMT_8BIT_U;
		default:
			return 0;
	}
}

uint32 MultiAudioDevice::select_multiaudio_rate(uint32 rate)
{
	//highest rate
	uint32 crate = B_SR_1536000;
	while(crate != 0) {
		if(rate & crate)
			return crate;
		crate >>= 1;
	}
	return 0;
}

uint32 MultiAudioDevice::select_multiaudio_format(uint32 fmt)
{
	//highest format
	if(fmt & B_FMT_FLOAT) {
		return B_FMT_FLOAT;
	} else if(fmt & B_FMT_32BIT) {
		return B_FMT_32BIT;
	} else if(fmt & B_FMT_24BIT) {
		return B_FMT_24BIT;
	} else if(fmt & B_FMT_20BIT) {
		return B_FMT_20BIT;
	} else if(fmt & B_FMT_18BIT) {
		return B_FMT_18BIT;
	} else if(fmt & B_FMT_16BIT) {
		return B_FMT_16BIT;
	} else if(fmt & B_FMT_8BIT_S) {
		return B_FMT_8BIT_S;
	} else if(fmt & B_FMT_8BIT_U) {
		return B_FMT_8BIT_U;
	} else
		return 0;
}



MultiAudioDevice::~MultiAudioDevice()
{
	CALLED();
	if ( fd != 0 ) {
		close( fd );
	}
}

MultiAudioDevice::MultiAudioDevice(const char* name, const char* path)
{
	CALLED();
	fInitCheckStatus = B_NO_INIT;
	
	strcpy(fDevice_name, name);
	strcpy(fDevice_path, path);
	
	PRINT(("name : %s, path : %s\n", fDevice_name, fDevice_path));
	
	if(InitDriver()!=B_OK)
		return;
	
	fInitCheckStatus = B_OK;
}


status_t MultiAudioDevice::InitCheck(void) const
{
	CALLED();
	return fInitCheckStatus;
}


status_t MultiAudioDevice::InitDriver()
{
	multi_channel_enable 	MCE;
	uint32					mce_enable_bits;
	int rval, i, num_outputs, num_inputs, num_channels;
	
	CALLED();

	//open the device driver for output
	fd = open( fDevice_path, O_WRONLY );

	if ( fd == 0 ) {
		return B_ERROR;
	}
	
	//
	// Get description
	//
	MD.info_size = sizeof(MD);
	MD.request_channel_count = MAX_CHANNELS;
	MD.channels = MCI;
	rval = DRIVER_GET_DESCRIPTION(&MD,0);
	if (B_OK != rval)
	{
		fprintf(stderr, "Failed on B_MULTI_GET_DESCRIPTION\n");
		return B_ERROR;
	}

	printf("Friendly name:\t%s\nVendor:\t\t%s\n",
				MD.friendly_name,MD.vendor_info);
	printf("%ld outputs\t%ld inputs\n%ld out busses\t%ld in busses\n",
				MD.output_channel_count,MD.input_channel_count,
				MD.output_bus_channel_count,MD.input_bus_channel_count);
	printf("\nChannels\n"
			 "ID\tKind\tDesig\tConnectors\n");

	for (i = 0 ; i < (MD.output_channel_count + MD.input_channel_count); i++)
	{
		printf("%ld\t%d\t0x%lx\t0x%lx\n",MD.channels[i].channel_id,
											MD.channels[i].kind,
											MD.channels[i].designations,
											MD.channels[i].connectors);
	}			 
	printf("\n");
	
	printf("Output rates\t\t0x%lx\n",MD.output_rates);
	printf("Input rates\t\t0x%lx\n",MD.input_rates);
	printf("Max CVSR\t\t%.0f\n",MD.max_cvsr_rate);
	printf("Min CVSR\t\t%.0f\n",MD.min_cvsr_rate);
	printf("Output formats\t\t0x%lx\n",MD.output_formats);
	printf("Input formats\t\t0x%lx\n",MD.input_formats);
	printf("Lock sources\t\t0x%lx\n",MD.lock_sources);
	printf("Timecode sources\t0x%lx\n",MD.timecode_sources);
	printf("Interface flags\t\t0x%lx\n",MD.interface_flags);
	printf("Control panel string:\t\t%s\n",MD.control_panel);
	printf("\n");
	
	num_outputs = MD.output_channel_count;
	num_inputs = MD.input_channel_count;
	num_channels = num_outputs + num_inputs;
	
	// Get and set enabled channels
	MCE.info_size = sizeof(MCE);
	MCE.enable_bits = (uchar *) &mce_enable_bits;
	rval = DRIVER_GET_ENABLED_CHANNELS(&MCE, sizeof(MCE));
	if (B_OK != rval)
	{
		fprintf(stderr, "Failed on B_MULTI_GET_ENABLED_CHANNELS len is 0x%lx\n",sizeof(MCE));
		return B_ERROR;
	}
	
	mce_enable_bits = (1 << num_channels) - 1;
	MCE.lock_source = B_MULTI_LOCK_INTERNAL;
	rval = DRIVER_SET_ENABLED_CHANNELS(&MCE, 0);
	if (B_OK != rval)
	{
		fprintf(stderr, "Failed on B_MULTI_SET_ENABLED_CHANNELS 0x%p 0x%x\n", MCE.enable_bits, *(MCE.enable_bits));
		return B_ERROR;
	}
	
	//
	// Set the sample rate
	//
	MFI.info_size = sizeof(MFI);
	MFI.output.rate = select_multiaudio_rate(MD.output_rates);	//B_SR_48000;
	MFI.output.cvsr = 0;
	MFI.output.format = select_multiaudio_format(MD.output_formats);	//B_FMT_16BIT;
	MFI.input.rate = MFI.output.rate;
	MFI.input.cvsr = MFI.output.cvsr;
	MFI.input.format = MFI.output.format;
	rval = DRIVER_SET_GLOBAL_FORMAT(&MFI, 0);
	if (B_OK != rval)
	{
		fprintf(stderr, "Failed on B_MULTI_SET_GLOBAL_FORMAT\n");
		return B_ERROR;
	}
	
	//
	// Get the buffers
	//
	play_buffer_desc[0] = play_buffer_list0;
	play_buffer_desc[1] = play_buffer_list1;
	record_buffer_desc[0] = record_buffer_list0;
	record_buffer_desc[1] = record_buffer_list1;
	MBL.info_size = sizeof(MBL);
	MBL.request_playback_buffer_size = 0;           //use the default......
	MBL.request_playback_buffers = NB_BUFFERS;
	MBL.request_playback_channels = num_outputs;
	MBL.playback_buffers = (buffer_desc **) play_buffer_desc;	
	MBL.request_record_buffer_size = 0;           //use the default......
	MBL.request_record_buffers = NB_BUFFERS;
	MBL.request_record_channels = num_inputs;
	MBL.record_buffers = (buffer_desc **) record_buffer_desc;		
	rval = DRIVER_GET_BUFFERS(&MBL, 0);

	if (B_OK != rval)
	{
		fprintf(stderr, "Failed on B_MULTI_GET_BUFFERS\n");
		return B_ERROR;
	}
	
	for (i = 0; i < MBL.return_playback_buffers; i++)
	{
		for (int j=0; j < MBL.return_playback_channels; j++)
		{
			//fPlay[i][j] = (int32 *) MBL.playback_buffers[i][j].base;
			PRINT(("MBL.playback_buffers[%d][%d].base: %p\n",
				i,j,MBL.playback_buffers[i][j].base));
			PRINT(("MBL.playback_buffers[%d][%d].stride: %i\n",
				i,j,MBL.playback_buffers[i][j].stride));
		}
		//memset(MBL.playback_buffers[i][0].base, 0, MBL.return_playback_buffer_size * 4 /*samps to bytes*/);
		//PRINT(("Play buffers[%d] size : %li zeroed\n",i, MBL.return_playback_buffer_size * 4));
	}
	
	for (i = 0; i < MBL.return_record_buffers; i++)
	{
		for (int j=0; j < MBL.return_record_channels; j++)
		{
			/*fRecord[i][j] = (int32 *) MBL.record_buffers[i][j].base;
			PRINT(("Record buffers[%d][%d]: %p\n",i,j,fRecord[i][j]));*/
			PRINT(("MBL.record_buffers[%d][%d].base: %p\n",
				i,j,MBL.record_buffers[i][j].base));
			PRINT(("MBL.record_buffers[%d][%d].stride: %i\n",
				i,j,MBL.record_buffers[i][j].stride));
		}
		//memset(MBL.record_buffers[i][0].base, 0, MBL.return_record_buffer_size * 4 /*samps to bytes*/);
		//PRINT(("Record buffers[%d] size : %li zeroed\n",i, MBL.return_record_buffer_size * 4));
	}
	
	MMCI.info_size = sizeof(MMCI);
	MMCI.control_count = MAX_CONTROLS;
	MMCI.controls = MMC;
	
	rval = DRIVER_LIST_MIX_CONTROLS(&MMCI, 0);

	if (B_OK != rval)
	{
		fprintf(stderr, "Failed on DRIVER_LIST_MIX_CONTROLS\n");
		return B_ERROR;
	}
	
	return B_OK;
}

int 
MultiAudioDevice::DoBufferExchange(multi_buffer_info *MBI)
{
	return DRIVER_BUFFER_EXCHANGE(MBI, 0);
}

int
MultiAudioDevice::DoSetMix(multi_mix_value_info *MMVI)
{
	return DRIVER_SET_MIX(MMVI, 0);
}

int
MultiAudioDevice::DoGetMix(multi_mix_value_info *MMVI)
{
	return DRIVER_GET_MIX(MMVI, 0);
}
