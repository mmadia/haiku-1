/*
 * Copyright (c) 2002-2003 Matthijs Hollemans
 */

#include "debug.h"
#include "MidiConsumer.h"
#include "MidiRoster.h"
#include "protocol.h"

//------------------------------------------------------------------------------

BMidiLocalConsumer::BMidiLocalConsumer(const char* name)
	: BMidiConsumer(name)
{
	TRACE(("BMidiLocalConsumer::BMidiLocalConsumer"))

	isLocal = true;
	refCount = 1;

	BMidiRoster::MidiRoster()->CreateLocal(this);
}

//------------------------------------------------------------------------------

BMidiLocalConsumer::~BMidiLocalConsumer()
{
	TRACE(("BMidiLocalConsumer::~BMidiLocalConsumer"))

	BMidiRoster::MidiRoster()->DeleteLocal(this);
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
	UNIMPLEMENTED
	return 0;
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::SetTimeout(bigtime_t when, void* data)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::Timeout(void* data)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::Data(
	uchar* data, size_t length, bool atomic, bigtime_t time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::NoteOff(
	uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::NoteOn(
	uchar channel, uchar note, uchar velocity, bigtime_t time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::KeyPressure(
	uchar channel, uchar note, uchar pressure, bigtime_t time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::ControlChange(
	uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::ProgramChange(
	uchar channel, uchar programNumber, bigtime_t time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::ChannelPressure(
	uchar channel, uchar pressure, bigtime_t time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::PitchBend(
	uchar channel, uchar lsb, uchar msb, bigtime_t time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::SystemExclusive(
	void* data, size_t dataLength, bigtime_t time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::SystemCommon(
	uchar statusByte, uchar data1, uchar data2, bigtime_t time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::SystemRealTime(uchar statusByte, bigtime_t time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::TempoChange(int32 bpm, bigtime_t time)
{
	UNIMPLEMENTED
}

//------------------------------------------------------------------------------

void BMidiLocalConsumer::AllNotesOff(bool justChannel, bigtime_t time)
{
	UNIMPLEMENTED
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
