/*
 * Copyright (c) 2002-2003 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>

#include "debug.h"
#include "MidiConsumer.h"
#include "MidiRoster.h"
#include "protocol.h"

//------------------------------------------------------------------------------

int32 _midi_event_thread(void* data)
{
	return ((BMidiLocalConsumer*) data)->EventThread();
}

//------------------------------------------------------------------------------

BMidiLocalConsumer::BMidiLocalConsumer(const char* name)
	: BMidiConsumer(name)
{
	TRACE(("BMidiLocalConsumer::BMidiLocalConsumer"))

	isLocal = true;
	refCount = 1;
	timeout = (bigtime_t) -1;
	timeoutData = NULL;

	port = create_port(1, "MidiEventPort");
	thread = spawn_thread(
		_midi_event_thread, "MidiEventThread", B_REAL_TIME_PRIORITY, this);
	resume_thread(thread);

	BMidiRoster::MidiRoster()->CreateLocal(this);
}

//------------------------------------------------------------------------------

BMidiLocalConsumer::~BMidiLocalConsumer()
{
	TRACE(("BMidiLocalConsumer::~BMidiLocalConsumer"))

	BMidiRoster::MidiRoster()->DeleteLocal(this);

	delete_port(port);

	status_t result;
	wait_for_thread(thread, &result);
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::SetLatency(bigtime_t latency_)
{
	if (latency_ < 0)
	{
		WARN("SetLatency() does not accept negative values");
		return;
	}
	else if (!IsValid())
	{
		return;
	}
	else if (latency != latency_)
	{
		BMessage msg;
		msg.AddInt64("midi:latency", latency_);

		if (SendChangeRequest(&msg) == B_OK)
		{
			if (LockLooper())
			{
				latency = latency_;
				UnlockLooper();
			}
		}
	}
}

//------------------------------------------------------------------------------

int32 BMidiLocalConsumer::GetProducerID(void)
{
	return currentProducer;
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::SetTimeout(bigtime_t when, void* data)
{
	timeout = when;
	timeoutData = data;
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::Timeout(void* data)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::Data(
	uchar* data, size_t length, bool atomic, bigtime_t time)
{
	if (atomic)
	{
		switch (data[0] & 0xF0)
		{
			case B_NOTE_OFF:
			{
				if (length == 3)
				{
					NoteOff(data[0] & 0x0F, data[1], data[2], time);
				}
				break;
			}

			case B_NOTE_ON:
			{
				if (length == 3)
				{
					NoteOn(data[0] & 0x0F, data[1], data[2], time);
				}
				break;
			}

			case B_KEY_PRESSURE:
			{
				if (length == 3)
				{
					KeyPressure(data[0] & 0x0F, data[1], data[2], time);
				}
				break;
			}

			case B_CONTROL_CHANGE:
			{
				if (length == 3)
				{
					ControlChange(data[0] & 0x0F, data[1], data[2], time);
				}
				break;
			}

			case B_PROGRAM_CHANGE:
			{
				if (length == 2)
				{
					ProgramChange(data[0] & 0x0F, data[1], time);
				}
				break;
			}

			case B_CHANNEL_PRESSURE:
			{
				if (length == 2)
				{
					ChannelPressure(data[0] & 0x0F, data[1], time);
				}
				break;
			}

			case B_PITCH_BEND:
			{
				if (length == 3)
				{
					PitchBend(data[0] & 0x0F, data[1], data[2], time);
				}
				break;
			}

			case 0xF0:
			{
				switch (data[0])
				{
					case B_SYS_EX_START:
					{
						if (data[length - 1] == B_SYS_EX_END)
						{
							SystemExclusive(data + 1, length - 2, time);
						}
						else // sysex-end is not required
						{
							SystemExclusive(data + 1, length - 1, time);
						}
						break;
					}

					case B_TUNE_REQUEST:
					case B_SYS_EX_END:
					{
						if (length == 1)
						{
							SystemCommon(data[0], 0, 0, time);
						}
						break;
					}

					case B_CABLE_MESSAGE:
					case B_MIDI_TIME_CODE:
					case B_SONG_SELECT:
					{
						if (length == 2)
						{
							SystemCommon(data[0], data[1], 0, time);
						}
						break;
					}

					case B_SONG_POSITION:
					{
						if (length == 3)
						{
							SystemCommon(data[0], data[1], data[2], time);
						}
						break;
					}
	
					case B_TIMING_CLOCK:
					case B_START:
					case B_CONTINUE:
					case B_STOP:
					case B_ACTIVE_SENSING:
					{
						if (length == 1)
						{
							SystemRealTime(data[0], time);
						}
						break;
					}

					case B_SYSTEM_RESET:
					{
						if (length == 1)
						{
							SystemRealTime(data[0], time);
						}
						else if ((length == 6) && (data[1] == 0x51) 
											   && (data[2] == 0x03))
						{
							int32 tempo = 
								(data[3] << 16) | (data[4] << 8) | data[5];

							TempoChange(60000000/tempo, time);
						}
					}
				}
				break;
			}
		}
	}
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::NoteOff(
	uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::NoteOn(
	uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::KeyPressure(
	uchar channel, uchar note, uchar pressure, bigtime_t time)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::ProgramChange(
	uchar channel, uchar programNumber, bigtime_t time)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::ChannelPressure(
	uchar channel, uchar pressure, bigtime_t time)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::PitchBend(
	uchar channel, uchar lsb, uchar msb, bigtime_t time)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::SystemExclusive(
	void* data, size_t length, bigtime_t time)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::SystemCommon(
	uchar statusByte, uchar data1, uchar data2, bigtime_t time)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::SystemRealTime(uchar statusByte, bigtime_t time)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::TempoChange(int32 beatsPerMinute, bigtime_t time)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::AllNotesOff(bool justChannel, bigtime_t time)
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::_Reserved1() { }
void BMidiLocalConsumer::_Reserved2() { }
void BMidiLocalConsumer::_Reserved3() { }
void BMidiLocalConsumer::_Reserved4() { }
void BMidiLocalConsumer::_Reserved5() { }
void BMidiLocalConsumer::_Reserved6() { }
void BMidiLocalConsumer::_Reserved7() { }
void BMidiLocalConsumer::_Reserved8() { }

//------------------------------------------------------------------------------

int32 BMidiLocalConsumer::EventThread()
{
	int32 msg_code;
	ssize_t msg_size;
	ssize_t buf_size = 100;
	uint8* buffer = (uint8*) malloc(buf_size);

	while (true)
	{
		if (timeout == (bigtime_t) -1)
		{
			msg_size = port_buffer_size(port);
		}
		else  // have timeout
		{
			msg_size = port_buffer_size_etc(port, B_ABSOLUTE_TIMEOUT, timeout);
			if (msg_size == B_TIMED_OUT)
			{
				Timeout(timeoutData);
				timeout = (bigtime_t) -1;
				timeoutData = NULL;
				continue;
			}
		}

		if (msg_size < 0) { break; }  // error reading port

		if (msg_size > buf_size)
		{
			buffer = (uint8*) realloc(buffer, msg_size);
			buf_size = msg_size;
		}

		read_port(port, &msg_code, buffer, msg_size);

		if (msg_size > 20)  // minimum valid size
		{
			#ifdef DEBUG
			printf("*** received: ");
			for (int32 t = 0; t < msg_size; ++t)
			{
				printf("%02X, ", ((uint8*) buffer)[t]);
			}
			printf("\n");
			#endif

			currentProducer = *((uint32*)    (buffer +  0));
			int32 targetId  = *((uint32*)    (buffer +  4));
			bigtime_t time  = *((bigtime_t*) (buffer +  8));
			bool atomic     = *((bool*)      (buffer + 16));

			if (targetId == id)  // only if we are the destination
			{
				Data((uchar*) (buffer + 20), msg_size - 20, atomic, time);
			}
		}
	}

	free(buffer);
	return 0;
}

//------------------------------------------------------------------------------
