/*
 * Copyright © 2006-2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// NOTE: Based on my code in the BeOS interface for the VLC media player
// that I did during the VLC 0.4.3 - 0.4.6 times. Code not written by me
// removed. -Stephan Aßmus


#include "TransportControlGroup.h"

#include <stdio.h>
#include <string.h>

#include <String.h>

#include "ButtonBitmaps.h"
#include "PeakView.h"
#include "PlaybackState.h"
#include "PositionToolTip.h"
#include "SeekSlider.h"
#include "TransportButton.h"
#include "VolumeSlider.h"

enum {
	MSG_SEEK				= 'seek',
	MSG_PLAY				= 'play',
	MSG_STOP				= 'stop',
	MSG_REWIND				= 'rwnd',
	MSG_FORWARD				= 'frwd',
	MSG_SKIP_BACKWARDS		= 'skpb',
	MSG_SKIP_FORWARD		= 'skpf',
	MSG_SET_VOLUME			= 'stvl',
	MSG_SET_MUTE			= 'stmt',
};

#define BORDER_INSET 6.0
#define MIN_SPACE 4.0
#define SPEAKER_SLIDER_DIST 6.0
#define VOLUME_MIN_WIDTH 70.0
#define VOLUME_SLIDER_LAYOUT_WEIGHT 2.0

// the range of the volume sliders (in dB)
#define kVolumeDbMax	6.0
#define kVolumeDbMin	-60.0
// a power function for non linear sliders
#define kVolumeDbExpPositive 1.4	// for dB values > 0
#define kVolumeDbExpNegative 1.9	// for dB values < 0

#define kVolumeFactor	1000
#define kPositionFactor	3000


TransportControlGroup::TransportControlGroup(BRect frame, bool useSkipButtons,
		bool usePeakView, bool useWindButtons)
	:
	BView(frame, "transport control group", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_FRAME_EVENTS),
	fBottomControlHeight(0.0),
	fPeakViewMinWidth(0.0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	frame.Set(0.0, 0.0, 10.0, 10.0);

    // Seek Slider
	fSeekSlider = new SeekSlider(frame, "seek slider", new BMessage(MSG_SEEK),
		0, kPositionFactor);
	fSeekSlider->ResizeToPreferred();
	AddChild(fSeekSlider);

	fPositionToolTip = new PositionToolTip();
	fSeekSlider->SetToolTip(fPositionToolTip);

    // Buttons
	if (useSkipButtons) {
		// Skip Back
		frame.right = kRewindBitmapWidth - 1;
		frame.bottom = kRewindBitmapHeight - 1;
		fBottomControlHeight = kRewindBitmapHeight - 1.0;
		fSkipBack = new TransportButton(frame, B_EMPTY_STRING,
			kSkipBackBitmapBits, kPressedSkipBackBitmapBits,
			kDisabledSkipBackBitmapBits, new BMessage(MSG_SKIP_BACKWARDS));
		AddChild(fSkipBack);

		// Skip Foward
		fSkipForward = new TransportButton(frame, B_EMPTY_STRING,
			kSkipForwardBitmapBits, kPressedSkipForwardBitmapBits,
			kDisabledSkipForwardBitmapBits, new BMessage(MSG_SKIP_FORWARD));
		AddChild(fSkipForward);
	} else {
		fSkipBack = NULL;
		fSkipForward = NULL;
	}

	if (useWindButtons) {
		// Forward
		fForward = new TransportButton(frame, B_EMPTY_STRING,
			kForwardBitmapBits, kPressedForwardBitmapBits,
			kDisabledForwardBitmapBits, new BMessage(MSG_FORWARD));
		AddChild(fForward);

		// Rewind
		fRewind = new TransportButton(frame, B_EMPTY_STRING,
			kRewindBitmapBits, kPressedRewindBitmapBits,
			kDisabledRewindBitmapBits, new BMessage(MSG_REWIND));
		AddChild(fRewind);
	} else {
		fForward = NULL;
		fRewind = NULL;
	}

	// Play Pause
	frame.right = kPlayPauseBitmapWidth - 1;
	frame.bottom = kPlayPauseBitmapHeight - 1;
	if (fBottomControlHeight < kPlayPauseBitmapHeight - 1.0)
		fBottomControlHeight = kPlayPauseBitmapHeight - 1.0;
	fPlayPause = new PlayPauseButton(frame, B_EMPTY_STRING,
		kPlayButtonBitmapBits, kPressedPlayButtonBitmapBits,
		kDisabledPlayButtonBitmapBits, kPlayingPlayButtonBitmapBits,
		kPressedPlayingPlayButtonBitmapBits, kPausedPlayButtonBitmapBits,
		kPressedPausedPlayButtonBitmapBits, new BMessage(MSG_PLAY));

	AddChild(fPlayPause);

	// Stop
	frame.right = kStopBitmapWidth - 1;
	frame.bottom = kStopBitmapHeight - 1;
	if (fBottomControlHeight < kStopBitmapHeight - 1.0)
		fBottomControlHeight = kStopBitmapHeight - 1.0;
	fStop = new TransportButton(frame, B_EMPTY_STRING, kStopButtonBitmapBits,
		kPressedStopButtonBitmapBits, kDisabledStopButtonBitmapBits,
		new BMessage(MSG_STOP));
	AddChild(fStop);

	// Mute
	frame.right = kSpeakerIconBitmapWidth - 1;
	frame.bottom = kSpeakerIconBitmapHeight - 1;
	if (fBottomControlHeight < kSpeakerIconBitmapHeight - 1.0)
		fBottomControlHeight = kSpeakerIconBitmapHeight - 1.0;
	fMute = new TransportButton(frame, B_EMPTY_STRING, kSpeakerIconBits,
		kPressedSpeakerIconBits, kSpeakerIconBits, new BMessage(MSG_SET_MUTE));

	AddChild(fMute);

	// Volume Slider
	fVolumeSlider = new VolumeSlider(BRect(0.0, 0.0, VOLUME_MIN_WIDTH,
		kVolumeSliderBitmapHeight - 1.0), "volume slider",
		_DbToGain(_ExponentialToLinear(kVolumeDbMin)) * kVolumeFactor,
		_DbToGain(_ExponentialToLinear(kVolumeDbMax)) * kVolumeFactor,
		new BMessage(MSG_SET_VOLUME));
	fVolumeSlider->SetValue(_DbToGain(_ExponentialToLinear(0.0))
		* kVolumeFactor);
	AddChild(fVolumeSlider);

	// Peak view
	if (usePeakView) {
		fPeakView = new PeakView("peak view", false, false);
		AddChild(fPeakView);
		fPeakView->GetPreferredSize(&fPeakViewMinWidth, NULL);
	} else {
		fPeakView = NULL;
	}
}


TransportControlGroup::~TransportControlGroup()
{
	if (!fSeekSlider->IsEnabled())
		delete fPositionToolTip;
}


void
TransportControlGroup::AttachedToWindow()
{
	SetEnabled(EnabledButtons());

	// we are now a valid BHandler
	fSeekSlider->SetTarget(this);
	fVolumeSlider->SetTarget(this);
	if (fSkipBack)
		fSkipBack->SetTarget(this);
	if (fSkipForward)
		fSkipForward->SetTarget(this);
	if (fRewind)
		fRewind->SetTarget(this);
	if (fForward)
		fForward->SetTarget(this);
	fPlayPause->SetTarget(this);
	fStop->SetTarget(this);
	fMute->SetTarget(this);

	FrameResized(Bounds().Width(), Bounds().Height());
}


void
TransportControlGroup::FrameResized(float width, float height)
{
	// layout controls
	BRect r(Bounds());
	r.InsetBy(BORDER_INSET, BORDER_INSET);
	_LayoutControls(r);
}


void
TransportControlGroup::GetPreferredSize(float* width, float* height)
{
	BRect r(_MinFrame());

	if (width)
		*width = r.Width();
	if (height)
		*height = r.Height();
}


void
TransportControlGroup::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PLAY:
			_TogglePlaying();
			break;
		case MSG_STOP:
			_Stop();
			break;

		case MSG_REWIND:
			_Rewind();
			break;
		case MSG_FORWARD:
			_Forward();
			break;

		case MSG_SKIP_BACKWARDS:
			_SkipBackward();
			break;
		case MSG_SKIP_FORWARD:
			_SkipForward();
			break;

		case MSG_SET_VOLUME:
			_UpdateVolume();
			break;
		case MSG_SET_MUTE:
			_ToggleMute();
			break;

		case MSG_SEEK:
			_UpdatePosition();
			break;

		default:
		    BView::MessageReceived(message);
		    break;
	}
}


// #pragma mark - default implementation for the virtuals


uint32
TransportControlGroup::EnabledButtons()
{
	return 0;
}


void TransportControlGroup::TogglePlaying() {}
void TransportControlGroup::Stop() {}
void TransportControlGroup::Rewind() {}
void TransportControlGroup::Forward() {}
void TransportControlGroup::SkipBackward() {}
void TransportControlGroup::SkipForward() {}
void TransportControlGroup::VolumeChanged(float value) {}
void TransportControlGroup::ToggleMute() {}
void TransportControlGroup::PositionChanged(float value) {}


// #pragma mark -


float
TransportControlGroup::_LinearToExponential(float dbIn)
{
	float db = dbIn;
	if (db >= 0) {
		db = db * (pow(fabs(kVolumeDbMax), (1.0 / kVolumeDbExpPositive))
			/ fabs(kVolumeDbMax));
		db = pow(db, kVolumeDbExpPositive);
	} else {
		db = -db;
		db = db * (pow(fabs(kVolumeDbMin), (1.0 / kVolumeDbExpNegative))
			/ fabs(kVolumeDbMin));
		db = pow(db, kVolumeDbExpNegative);
		db = -db;
	}
	printf("_LinearToExponential %.4f => %.4f\n", dbIn, db);
	return db;
}


float
TransportControlGroup::_ExponentialToLinear(float dbIn)
{
	float db = dbIn;
	if (db >= 0) {
		db = pow(db, (1.0 / kVolumeDbExpPositive));
		db = db * (fabs(kVolumeDbMax) / pow(fabs(kVolumeDbMax),
			(1.0 / kVolumeDbExpPositive)));
	} else {
		db = -db;
		db = pow(db, (1.0 / kVolumeDbExpNegative));
		db = db * (fabs(kVolumeDbMin) / pow(fabs(kVolumeDbMin),
			(1.0 / kVolumeDbExpNegative)));
		db = -db;
	}
	//printf("_ExponentialToLinear %.4f => %.4f\n", dbIn, db);
	return db;
}


float
TransportControlGroup::_DbToGain(float db)
{
	return pow(10.0, db / 20.0);
}


float
TransportControlGroup::_GainToDb(float gain)
{
	return 20.0 * log10(gain);
}


// #pragma mark -


void
TransportControlGroup::SetEnabled(uint32 buttons)
{
	if (!LockLooper())
		return;

	fSeekSlider->SetEnabled(buttons & SEEK_ENABLED);
	fSeekSlider->SetToolTip((buttons & SEEK_ENABLED) != 0
		? fPositionToolTip : NULL);

	fVolumeSlider->SetEnabled(buttons & VOLUME_ENABLED);
	fMute->SetEnabled(buttons & VOLUME_ENABLED);

	if (fSkipBack)
		fSkipBack->SetEnabled(buttons & SKIP_BACK_ENABLED);
	if (fSkipForward)
		fSkipForward->SetEnabled(buttons & SKIP_FORWARD_ENABLED);
	if (fRewind)
		fRewind->SetEnabled(buttons & SEEK_BACK_ENABLED);
	if (fForward)
		fForward->SetEnabled(buttons & SEEK_FORWARD_ENABLED);

	fPlayPause->SetEnabled(buttons & PLAYBACK_ENABLED);
	fStop->SetEnabled(buttons & PLAYBACK_ENABLED);

	UnlockLooper();
}


// #pragma mark -


void
TransportControlGroup::SetPlaybackState(uint32 state)
{
	if (!LockLooper())
		return;

	switch (state) {
		case PLAYBACK_STATE_PLAYING:
			fPlayPause->SetPlaying();
			break;
		case PLAYBACK_STATE_PAUSED:
			fPlayPause->SetPaused();
			break;
		case PLAYBACK_STATE_STOPPED:
			fPlayPause->SetStopped();
			break;
	}

	UnlockLooper();
}


void
TransportControlGroup::SetSkippable(bool backward, bool forward)
{
	if (!LockLooper())
		return;

	if (fSkipBack)
		fSkipBack->SetEnabled(backward);
	if (fSkipForward)
		fSkipForward->SetEnabled(forward);

	UnlockLooper();
}


// #pragma mark -


void
TransportControlGroup::SetAudioEnabled(bool enabled)
{
	if (!LockLooper())
		return;

	fMute->SetEnabled(enabled);
	fVolumeSlider->SetEnabled(enabled);

	UnlockLooper();
}


void
TransportControlGroup::SetMuted(bool mute)
{
	if (!LockLooper())
		return;

	fVolumeSlider->SetMuted(mute);

	UnlockLooper();
}


void
TransportControlGroup::SetVolume(float value)
{
	float db = _GainToDb(value);
	float exponential = _LinearToExponential(db);
	float gain = _DbToGain(exponential);
	int32 pos = (int32)(floorf(gain * kVolumeFactor + 0.5));

	fVolumeSlider->SetValueNoInvoke(pos);
}


void
TransportControlGroup::SetPosition(float value, bigtime_t position,
	bigtime_t duration)
{
	if (fSeekSlider->IsTracking())
		return;

	fPositionToolTip->Update(position, duration);
	fSeekSlider->SetPosition(value);
}


float
TransportControlGroup::Position() const
{
	return fSeekSlider->Position();
}


void
TransportControlGroup::SetDisabledString(const char* string)
{
	fSeekSlider->SetDisabledString(string);
}


// #pragma mark -


void
TransportControlGroup::_LayoutControls(BRect frame) const
{
	BRect r(frame);
	// calculate absolutly minimal width
	float minWidth = 0.0;
	if (fSkipBack)
		minWidth += fSkipBack->Bounds().Width();
	if (fRewind)
		minWidth += fRewind->Bounds().Width();
	minWidth += fStop->Bounds().Width();
	minWidth += fPlayPause->Bounds().Width();
	if (fForward)
		minWidth += fForward->Bounds().Width();
	if (fSkipForward)
		minWidth += fSkipForward->Bounds().Width();
	minWidth += fMute->Bounds().Width();
	minWidth += VOLUME_MIN_WIDTH;
	if (fPeakView)
		minWidth += fPeakViewMinWidth;

	// layout seek slider
	r.bottom = r.top + fSeekSlider->Bounds().Height();
	_LayoutControl(fSeekSlider, r, true);

	// prevent spreading the controls too much
	if (frame.Width() > minWidth * 2.0)
		frame.right = frame.left + ceilf(minWidth * 2.0);

	float currentWidth = frame.Width();
	float space = (currentWidth - minWidth) / 6.0;
	// apply weighting
	space = min_c(MIN_SPACE + (space - MIN_SPACE) / VOLUME_SLIDER_LAYOUT_WEIGHT,
		MIN_SPACE * 2.0);
	// layout controls with "space" inbetween
	r.left = frame.left;
	r.top = r.bottom + MIN_SPACE + 1.0;
	r.bottom = frame.bottom;
	// skip back
	if (fSkipBack) {
		r.right = r.left + fSkipBack->Bounds().Width();
		_LayoutControl(fSkipBack, r);
		r.left = r.right + space;
	}
	// rewind
	if (fRewind) {
		r.right = r.left + fRewind->Bounds().Width();
		_LayoutControl(fRewind, r);
		r.left = r.right + space;
	}
	// stop
	r.right = r.left + fStop->Bounds().Width();
	_LayoutControl(fStop, r);
	r.left = r.right + space;
	// play/pause
	r.right = r.left + fPlayPause->Bounds().Width();
	_LayoutControl(fPlayPause, r);
	r.left = r.right + space;
	// forward
	if (fForward) {
		r.right = r.left + fForward->Bounds().Width();
		_LayoutControl(fForward, r);
		r.left = r.right + space;
	}
	// skip forward
	if (fSkipForward) {
		r.right = r.left + fSkipForward->Bounds().Width();
		_LayoutControl(fSkipForward, r);
		r.left = r.right + space;
	}
	// speaker icon
	r.left = r.right + space + space;
	r.right = r.left + fMute->Bounds().Width();
	_LayoutControl(fMute, r);

	// volume slider
	r.left = r.right + SPEAKER_SLIDER_DIST;
		// keep speaker icon and volume slider attached
	// layout volume slider
	float peakViewWidth = 0.0;
	if (fPeakView)
		peakViewWidth = (frame.right - r.left) / 2 + space;

	r.right = frame.right - peakViewWidth;
	_LayoutControl(fVolumeSlider, r, true);

	if (fPeakView) {
		peakViewWidth -= space;
		r.left = r.right + space;
		r.right = r.left + peakViewWidth;
		_LayoutControl(fPeakView, r, true, true);
	}
}


BRect
TransportControlGroup::_MinFrame() const
{
	// add up width of controls along bottom (seek slider will likely adopt)
	float minWidth = 2 * BORDER_INSET;
	if (fSkipBack)
		minWidth += fSkipBack->Bounds().Width() + MIN_SPACE;
	if (fRewind)
		minWidth += fRewind->Bounds().Width() + MIN_SPACE;
	minWidth += fStop->Bounds().Width() + MIN_SPACE;
	minWidth += fPlayPause->Bounds().Width() + MIN_SPACE;
	if (fForward)
		minWidth += fForward->Bounds().Width() + MIN_SPACE;
	if (fSkipForward)
		minWidth += fSkipForward->Bounds().Width() + MIN_SPACE + MIN_SPACE;
	minWidth += fMute->Bounds().Width() + SPEAKER_SLIDER_DIST;
	minWidth += VOLUME_MIN_WIDTH;
	if (fPeakView)
		minWidth += fPeakViewMinWidth;

	// add up height of seek slider and heighest control on bottom
	float minHeight = 2 * BORDER_INSET;
	minHeight += fSeekSlider->Bounds().Height() + MIN_SPACE + MIN_SPACE / 2.0;
	minHeight += fBottomControlHeight;
	return BRect(0.0, 0.0, minWidth - 1.0, minHeight - 1.0);
}


void
TransportControlGroup::_LayoutControl(BView* view, BRect frame,
	bool resizeWidth, bool resizeHeight) const
{
	if (!resizeHeight)
		// center vertically
		frame.top = (frame.top + frame.bottom - view->Bounds().Height()) / 2.0;
	if (!resizeWidth)
		// center horizontally
		frame.left = (frame.left + frame.right - view->Bounds().Width()) / 2.0;
	view->MoveTo(frame.LeftTop());
	float width = resizeWidth ? frame.Width() : view->Bounds().Width();
	float height = resizeHeight ? frame.Height() : view->Bounds().Height();
	if (resizeWidth || resizeHeight)
		view->ResizeTo(width, height);
}


void
TransportControlGroup::_TogglePlaying()
{
	TogglePlaying();
}


void
TransportControlGroup::_Stop()
{
	fPlayPause->SetStopped();
	Stop();
}


void
TransportControlGroup::_Rewind()
{
	Rewind();
}


void
TransportControlGroup::_Forward()
{
	Forward();
}


void
TransportControlGroup::_SkipBackward()
{
	SkipBackward();
}


void
TransportControlGroup::_SkipForward()
{
	SkipForward();
}


void
TransportControlGroup::_UpdateVolume()
{
	float pos = fVolumeSlider->Value() / (float)kVolumeFactor;
	float db = _ExponentialToLinear(_GainToDb(pos));
	float gain = _DbToGain(db);
	printf("_SetVolume: pos %.4f, db %.4f, gain %.4f\n", pos, db, gain);
	VolumeChanged(gain);
}


void
TransportControlGroup::_ToggleMute()
{
	fVolumeSlider->SetMuted(!fVolumeSlider->IsMuted());
	ToggleMute();
}


void
TransportControlGroup::_UpdatePosition()
{
	PositionChanged(fSeekSlider->Value() / (float)kPositionFactor);
}
