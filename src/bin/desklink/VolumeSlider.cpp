/*
 * Copyright (c) 2003-2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 */

#include <string.h>

#include <MediaRoster.h>
#include <MediaTheme.h>
#include <MultiChannelControl.h>
#include <Screen.h>
#include <Beep.h>
#include <Box.h>
#include <stdio.h>
#include <Debug.h>

#include "VolumeSlider.h"
#include "iconfile.h"

#define VOLUME_CHANGED 'vlcg'
#define VOLUME_UPDATED 'vlud'

#define REDZONESTART 151


VolumeSlider::VolumeSlider(BRect frame, bool dontBeep, int32 volumeWhich)
	: BWindow(frame, "VolumeSlider", B_BORDERED_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_WILL_ACCEPT_FIRST_CLICK, 0),
	fAudioMixerNode(NULL),
	fParamWeb(NULL),
	fMixerParam(NULL)
{
	// Make sure it's not outside the screen.
	const int32 kMargin = 3;
	BRect windowRect = ConvertToScreen(Bounds());
	BRect screenFrame(BScreen(B_MAIN_SCREEN_ID).Frame());
	if (screenFrame.right < windowRect.right + kMargin)
		MoveBy(- kMargin - windowRect.right + screenFrame.right, 0);
	if (screenFrame.bottom < windowRect.bottom + kMargin)
		MoveBy(0, - kMargin - windowRect.bottom + screenFrame.bottom);
	if (screenFrame.left > windowRect.left - kMargin)
		MoveBy(kMargin + screenFrame.left - windowRect.left, 0);
	if (screenFrame.top > windowRect.top - kMargin)
		MoveBy(0, kMargin + screenFrame.top - windowRect.top);

	float value = 0.0;
	bool retrying = false;
	fDontBeep = dontBeep;

	fAudioMixerNode = new media_node();

	status_t err = B_OK; /* BMediaRoster::Roster() doesn't set it if all is ok */
	const char *errString = NULL;
	BMediaRoster* roster = BMediaRoster::Roster(&err);

 retry:
	/* here we release the BMediaRoster once if we can't access the system mixer,
	 * to make sure it really isn't there, and it's not BMediaRoster that is messed up.
	 */
	if (retrying) {
		errString = NULL;
		PRINT(("retrying to get a Media Roster\n"));
		/* BMediaRoster looks doomed */
		roster = BMediaRoster::CurrentRoster();
		if (roster) {
			roster->Lock();
			roster->Quit();
		}
		snooze(10000);
		roster = BMediaRoster::Roster(&err);
	}
	
	if (roster && err == B_OK) {
		switch (volumeWhich) {
			case VOLUME_USE_MIXER:
				err = roster->GetAudioMixer(fAudioMixerNode);
				break;
			case VOLUME_USE_PHYS_OUTPUT:
				err = roster->GetAudioOutput(fAudioMixerNode);
				break;
		}
		if (err == B_OK) {
			if ((err = roster->GetParameterWebFor(*fAudioMixerNode, &fParamWeb)) == B_OK) {
				// Finding the Mixer slider in the audio output ParameterWeb 
				int32 numParams = fParamWeb->CountParameters();
				BParameter* p = NULL;
				bool foundMixerLabel = false;
				for (int i = 0; i < numParams; i++) {
					p = fParamWeb->ParameterAt(i);
					PRINT(("BParameter[%i]: %s\n", i, p->Name()));
					if (volumeWhich == VOLUME_USE_MIXER) {
						if (!strcmp(p->Kind(), B_MASTER_GAIN))
							break;
					} else if (volumeWhich == VOLUME_USE_PHYS_OUTPUT) {
						/* not all cards use the same name, and
						 * they don't seem to use Kind() == B_MASTER_GAIN
						 */
						if (!strcmp(p->Kind(), B_MASTER_GAIN))
							break;
						PRINT(("not MASTER_GAIN \n"));

						/* some audio card
						 */
						if (!strcmp(p->Name(), "Master"))
							break;
						PRINT(("not 'Master' \n"));

						/* some Ensonic card have all controls names 'Volume', so
						 * need to fint the one that has the 'Mixer' text label
						 */
						if (foundMixerLabel && !strcmp(p->Name(), "Volume"))
							break;
						if (!strcmp(p->Name(), "Mixer"))
							foundMixerLabel = true;
						PRINT(("not 'Mixer' \n"));
					}
#if 0
					//if (!strcmp(p->Name(), "Master")) {
					if (!strcmp(p->Kind(), B_MASTER_GAIN)) {
						for (; i < numParams; i++) {
							p = fParamWeb->ParameterAt(i);
							if (strcmp(p->Kind(), B_MASTER_GAIN)) p=NULL;
							else break;
						}
						break;
					} else p = NULL;
#endif
					p = NULL;
				}
				if (p == NULL) {
					errString = volumeWhich
						? "Could not find the soundcard":"Could not find the mixer";
				} else if (p->Type() != BParameter::B_CONTINUOUS_PARAMETER) {
					errString = volumeWhich
						? "Soundcard control unknown":"Mixer control unknown";
				} else {
					fMixerParam = dynamic_cast<BContinuousParameter*>(p);
					fMin = fMixerParam->MinValue();
					fMax = fMixerParam->MaxValue();
					fStep = fMixerParam->ValueStep();

					float chanData[2];
					bigtime_t lastChange;
					size_t size = sizeof(chanData);

					fMixerParam->GetValue(&chanData, &size, &lastChange);

					value = (chanData[0] - fMin) * 100 / ((fMax - fMin) ? (fMax - fMin) : 1);
				}
			} else {
				errString = "No parameter web";
			}
		} else {
			if (!retrying) {
				retrying = true;
				goto retry;
			}
			errString = volumeWhich ? "No Audio output" : "No Mixer";
		}
	} else {
		if (!retrying) {
			retrying = true;
			goto retry;
		}
		errString = "No Media Roster";
	}

	if (err != B_OK) {
		delete fAudioMixerNode;
		fAudioMixerNode = NULL;
	}
	if (errString)
		fprintf(stderr, "VolumeSlider: %s.\n", errString);

	BBox *box = new BBox(Bounds(), "sliderbox", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	AddChild(box);

	fHasChanged = false; /* make sure we don't beep if we don't change anything */
	if (fMixerParam == NULL)
		value = -1;
	fSlider = new SliderView(box->Bounds().InsetByCopy(1, 1), new BMessage(VOLUME_CHANGED), 
		errString == NULL ? "Volume" : errString, B_FOLLOW_LEFT | B_FOLLOW_TOP, (int32)value);
	box->AddChild(fSlider);

	fSlider->SetTarget(this);

	SetPulseRate(100);	
}


VolumeSlider::~VolumeSlider()
{
	delete fParamWeb;
	BMediaRoster* roster = BMediaRoster::CurrentRoster();
	if (roster && fAudioMixerNode)
		roster->ReleaseNode(*fAudioMixerNode);
}


void
VolumeSlider::WindowActivated(bool active)
{
	/* don't Quit() ! thanks for FFM users */
}


void
VolumeSlider::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case VOLUME_UPDATED:
			PRINT(("VOLUME_UPDATED\n"));
			UpdateVolume(fMixerParam);
			fHasChanged = true;
			break;

		case VOLUME_CHANGED:
			if (fHasChanged) {
				PRINT(("VOLUME_CHANGED\n"));
				UpdateVolume(fMixerParam);
				if (!fDontBeep)
					beep();
			}
			Quit();
			break;

		default:
			BWindow::MessageReceived(msg);		// not a slider message, not our problem
	}
}


void
VolumeSlider::UpdateVolume(BContinuousParameter* param)
{
	if (!param)
		return;

	float chanData[2];
	bigtime_t lastChange;
	size_t size = sizeof(chanData);

	fMixerParam->GetValue( &chanData, &size, &lastChange );

	for (int i = 0; i < 2; i++) {
		chanData[i] = (fSlider->Value() * (fMax - fMin) / 100) / fStep * fStep + fMin;
	}

	PRINT(("Min value: %f      Max Value: %f\nData: %f     %f\n",
		fMixerParam->MinValue(), fMixerParam->MaxValue(), chanData[0], chanData[1]));
	fMixerParam->SetValue(&chanData, sizeof(chanData), system_time()+1000);
}


//	#pragma mark -


SliderView::SliderView(BRect rect, BMessage *msg, const char *title,
		uint32 resizeFlags, int32 value)
	: BControl(rect, "slider", NULL, msg, resizeFlags, B_WILL_DRAW | B_PULSE_NEEDED),
	fLeftBitmap(BRect(0, 0, kLeftWidth - 1, kLeftHeight - 1), B_CMAP8),
	fRightBitmap(BRect(0, 0, kRightWidth - 1, kRightHeight - 1), B_CMAP8),
	fButtonBitmap(BRect(0, 0, kButtonWidth - 1, kButtonHeight - 1), B_CMAP8),
	fTitle(title)
{
	fLeftBitmap.SetBits(kLeftBits, kLeftWidth * kLeftHeight, 0, B_CMAP8);
	fRightBitmap.SetBits(kRightBits, kRightWidth * kRightHeight, 0, B_CMAP8);
	fButtonBitmap.SetBits(kButtonBits, kButtonWidth * kButtonHeight, 0, B_CMAP8);

	SetTracking(true);
	SetValue(value);
}


SliderView::~SliderView()
{
}


void
SliderView::Pulse()
{
	uint32 mouseButtons;
	BPoint where;
	GetMouse(&where, &mouseButtons, true);
	// button not pressed, exit
	if (!(mouseButtons & B_PRIMARY_MOUSE_BUTTON)) {
		SetTracking(false);
		Invoke();
	}
}


void
SliderView::Draw(BRect updateRect)
{
	SetHighColor(189,186,189);
	StrokeLine(BPoint(11,1), BPoint(192,1));
	SetHighColor(0,0,0);
	StrokeLine(BPoint(11,2), BPoint(192,2));
	SetHighColor(255,255,255);
	StrokeLine(BPoint(11,14), BPoint(192,14));
	SetHighColor(231,227,231);
	StrokeLine(BPoint(11,15), BPoint(192,15));

	SetLowColor(ViewColor());

	SetDrawingMode(B_OP_OVER);

	DrawBitmapAsync(&fLeftBitmap, BPoint(5,1));
	DrawBitmapAsync(&fRightBitmap, BPoint(193,1));

	float position = 11 + (192-11) * ((Value() == -1) ? 0 : Value()) / 100;
	float right = (position < REDZONESTART) ? position : REDZONESTART;
	SetHighColor(99,151,99);
	FillRect(BRect(11,3,right,4));
	SetHighColor(156,203,156);
	FillRect(BRect(11,5,right,13));
	if (right == REDZONESTART) {
		SetHighColor(156,101,99);
		FillRect(BRect(REDZONESTART,3,position,4));
		SetHighColor(255,154,156);
		FillRect(BRect(REDZONESTART,5,position,13));
	}		
	SetHighColor(156,154,156);
	FillRect(BRect(position,3,192,13));

	BFont font;
	float width = font.StringWidth(fTitle);

	SetHighColor(49,154,49);
	DrawString(fTitle, BPoint(11 + (192-11-width)/2, 12));

	DrawBitmapAsync(&fButtonBitmap, BPoint(position-5,3));

	Sync();

	SetDrawingMode(B_OP_COPY);
}


void 
SliderView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (!IsTracking())
		return;

	uint32 mouseButtons;
	BPoint where;
	GetMouse(&where, &mouseButtons, true);

	// button not pressed, exit
	if (! (mouseButtons & B_PRIMARY_MOUSE_BUTTON)) {
		Invoke();
		SetTracking(false);
	}

	if (Value() == -1 || !Bounds().InsetBySelf(2, 2).Contains(point))
		return;

	float v = MIN(MAX(point.x, 11), 192);
	v = (v - 11) / (192-11) * 100;
	v = MAX(MIN(v,100), 0);
	SetValue((int32)v);
	Draw(Bounds());
	Flush();

	if (Window())
		Window()->PostMessage(VOLUME_UPDATED);
}


void
SliderView::MouseUp(BPoint point)
{
	if (!IsTracking())
		return;

	if (Value() != -1 && Bounds().InsetBySelf(2, 2).Contains(point)) {
		float v = MIN(MAX(point.x, 11), 192);
		v = (v - 11) / (192-11) * 100;
		v = MAX(MIN(v,100), 0);
		SetValue((int32)v);
	}

	Invoke();
	SetTracking(false);
	Draw(Bounds());
	Flush();
}
