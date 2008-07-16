/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Slider.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <Errors.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <Region.h>
#include <Window.h>


BSlider::BSlider(BRect frame, const char* name, const char* label,
			BMessage* message, int32 minValue, int32 maxValue,
			thumb_style thumbType, uint32 resizingMode, uint32 flags)
	: BControl(frame, name, label, message, resizingMode, flags),
	fModificationMessage(NULL),
	fSnoozeAmount(20000),

	fMinLimitLabel(NULL),
	fMaxLimitLabel(NULL),

	fMinValue(minValue),
	fMaxValue(maxValue),
	fKeyIncrementValue(1),

	fHashMarkCount(0),
	fHashMarks(B_HASH_MARKS_NONE),

	fStyle(thumbType),

	fOrientation(B_HORIZONTAL),
	fBarThickness(6.0)
{
	SetBarColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_4_TINT));

	UseFillColor(false, NULL);

	_InitObject();
	SetValue(0);
}


BSlider::BSlider(BRect frame, const char *name, const char *label,
			BMessage *message, int32 minValue, int32 maxValue,
			orientation posture, thumb_style thumbType, uint32 resizingMode,
			uint32 flags)
	: BControl(frame, name, label, message, resizingMode, flags),
	fModificationMessage(NULL),
	fSnoozeAmount(20000),

	fMinLimitLabel(NULL),
	fMaxLimitLabel(NULL),

	fMinValue(minValue),
	fMaxValue(maxValue),
	fKeyIncrementValue(1),

	fHashMarkCount(0),
	fHashMarks(B_HASH_MARKS_NONE),

	fStyle(thumbType),

	fOrientation(posture),
	fBarThickness(6.0)
{
	SetBarColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_4_TINT));

	UseFillColor(false, NULL);

	_InitObject();
	SetValue(0);
}


BSlider::BSlider(const char *name, const char *label, BMessage *message, 
			int32 minValue, int32 maxValue, orientation posture,
			thumb_style thumbType, uint32 flags)
	: BControl(name, label, message, flags),
	fModificationMessage(NULL),
	fSnoozeAmount(20000),

	fMinLimitLabel(NULL),
	fMaxLimitLabel(NULL),

	fMinValue(minValue),
	fMaxValue(maxValue),
	fKeyIncrementValue(1),

	fHashMarkCount(0),
	fHashMarks(B_HASH_MARKS_NONE),

	fStyle(thumbType),

	fOrientation(posture),
	fBarThickness(6.0)
{
	SetBarColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_4_TINT));

	UseFillColor(false, NULL);

	_InitObject();
	SetValue(0);
}


BSlider::BSlider(BMessage *archive)
	: BControl(archive)
{
	fModificationMessage = NULL;

	if (archive->HasMessage("_mod_msg")) {
		BMessage* message = new BMessage;

		archive->FindMessage("_mod_msg", message);
		
		SetModificationMessage(message);
	}

	if (archive->FindInt32("_sdelay", &fSnoozeAmount) != B_OK)
		SetSnoozeAmount(20000);

	rgb_color color;
	if (archive->FindInt32("_fcolor", (int32 *)&color) == B_OK)
		UseFillColor(true, &color);
	else
		UseFillColor(false);

	int32 orient;
	if (archive->FindInt32("_orient", &orient) == B_OK)
		fOrientation = (orientation)orient;
	else
		fOrientation = B_HORIZONTAL;

	fMinLimitLabel = NULL;
	fMaxLimitLabel = NULL;

	const char* minlbl = NULL;
	const char* maxlbl = NULL;

	archive->FindString("_minlbl", &minlbl);
	archive->FindString("_maxlbl", &maxlbl);

	SetLimitLabels(minlbl, maxlbl);

	if (archive->FindInt32("_min", &fMinValue) != B_OK)
		fMinValue = 0;

	if (archive->FindInt32("_max", &fMaxValue) != B_OK)
		fMaxValue = 100;

	if (archive->FindInt32("_incrementvalue", &fKeyIncrementValue) != B_OK)
		fKeyIncrementValue = 1;

	if (archive->FindInt32("_hashcount", &fHashMarkCount) != B_OK)
		fHashMarkCount = 11;

	int16 hashloc;
	if (archive->FindInt16("_hashloc", &hashloc) == B_OK)
		fHashMarks = (hash_mark_location)hashloc;
	else
		fHashMarks = B_HASH_MARKS_NONE;

	int16 sstyle;
	if (archive->FindInt16("_sstyle", &sstyle) == B_OK)
		fStyle = (thumb_style)sstyle;
	else
		fStyle = B_BLOCK_THUMB;

	if (archive->FindInt32("_bcolor", (int32 *)&color) != B_OK)
		color = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_4_TINT);
	SetBarColor(color);

	float bthickness;
	if (archive->FindFloat("_bthickness", &bthickness) == B_OK)
		fBarThickness = bthickness;
	else
		fBarThickness = 6.0f;

	_InitObject();
}


BSlider::~BSlider()
{
#if USE_OFF_SCREEN_VIEW
	delete fOffScreenBits;
#endif

	delete fModificationMessage;
	free(fMinLimitLabel);
	free(fMaxLimitLabel);
}


void
BSlider::_InitObject()
{
	fLocation.x = 0;
	fLocation.y = 0;
	fInitialLocation.x = 0;
	fInitialLocation.y = 0;

#if USE_OFF_SCREEN_VIEW
	fOffScreenBits = NULL;
	fOffScreenView = NULL;
#endif

	fUpdateText = NULL;
	fMinSize.Set(-1, -1);
}


BArchivable*
BSlider::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BSlider"))
		return new BSlider(archive);

	return NULL;
}


status_t
BSlider::Archive(BMessage *archive, bool deep) const
{
	status_t ret = BControl::Archive(archive, deep);

	if (ModificationMessage() && ret == B_OK)
		ret = archive->AddMessage("_mod_msg", ModificationMessage());

	if (ret == B_OK)
		ret = archive->AddInt32("_sdelay", fSnoozeAmount);
	if (ret == B_OK)
		ret = archive->AddInt32("_bcolor", (const uint32 &)fBarColor);

	if (FillColor(NULL) && ret == B_OK)
		ret = archive->AddInt32("_fcolor", (const uint32 &)fFillColor);

	if (ret == B_OK && fMinLimitLabel)
		ret = archive->AddString("_minlbl", fMinLimitLabel);

	if (ret == B_OK && fMaxLimitLabel)
		ret = archive->AddString("_maxlbl", fMaxLimitLabel);

	if (ret == B_OK)
		ret = archive->AddInt32("_min", fMinValue);
	if (ret == B_OK)
		ret = archive->AddInt32("_max", fMaxValue);

	if (ret == B_OK)
		ret = archive->AddInt32("_incrementvalue", fKeyIncrementValue);
	if (ret == B_OK)
		ret = archive->AddInt32("_hashcount", fHashMarkCount);
	if (ret == B_OK)
		ret = archive->AddInt16("_hashloc", fHashMarks);
	if (ret == B_OK)	
		ret = archive->AddInt16("_sstyle", fStyle);
	if (ret == B_OK)
		ret = archive->AddInt32("_orient", fOrientation);
	if (ret == B_OK)
		ret = archive->AddFloat("_bthickness", fBarThickness);

	return ret;
}


status_t
BSlider::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}


void
BSlider::WindowActivated(bool state)
{
	BControl::WindowActivated(state);
}


void
BSlider::AttachedToWindow()
{
	ResizeToPreferred();

	fLocation.Set(9.0f, 0.0f);
	fUpdateText = UpdateText();

#if USE_OFF_SCREEN_VIEW
	BRect bounds(Bounds());

	if (!fOffScreenView) {
		fOffScreenView = new BView(bounds, "", B_FOLLOW_ALL, B_WILL_DRAW);

		BFont font;
		GetFont(&font);
		fOffScreenView->SetFont(&font);
	}

	if (!fOffScreenBits) {
		fOffScreenBits = new BBitmap(bounds, B_CMAP8, true, false);

		if (fOffScreenBits && fOffScreenView)
			fOffScreenBits->AddChild(fOffScreenView);

	} else if (fOffScreenView)
		fOffScreenBits->AddChild(fOffScreenView);
#endif // USE_OFF_SCREEN_VIEW

	BControl::AttachedToWindow();

	BView* view = OffscreenView();
	if (view) {
		rgb_color color = ViewColor();
		if (Parent() != NULL)
			color = Parent()->ViewColor();

/*		fOffScreenBits->Lock();
		fOffScreenView->SetViewColor(color);
		fOffScreenView->SetLowColor(color);
		fOffScreenBits->Unlock();*/

		view->LockLooper();
		view->SetViewColor(B_TRANSPARENT_COLOR);
		view->SetLowColor(color);
		view->UnlockLooper();
	}

	SetValue(Value());
		// makes sure the value is within valid bounds
}


void
BSlider::AllAttached()
{
	BControl::AllAttached();
}


void
BSlider::AllDetached()
{
	BControl::AllDetached();
}


void
BSlider::DetachedFromWindow()
{
	BControl::DetachedFromWindow();

#if USE_OFF_SCREEN_VIEW
	if (fOffScreenBits) {
		delete fOffScreenBits;
		fOffScreenBits = NULL;
		fOffScreenView = NULL;
	}
#endif
}


void
BSlider::MessageReceived(BMessage *msg)
{
	BControl::MessageReceived(msg);
}


void
BSlider::FrameMoved(BPoint new_position)
{
	BControl::FrameMoved(new_position);
}


void
BSlider::FrameResized(float w,float h)
{
	BControl::FrameResized(w, h);

	BRect bounds(Bounds());

	if (bounds.right <= 0.0f || bounds.bottom <= 0.0f)
		return;

#if USE_OFF_SCREEN_VIEW
	if (fOffScreenBits) {
		fOffScreenBits->RemoveChild(fOffScreenView);
		delete fOffScreenBits;

		fOffScreenView->ResizeTo(bounds.Width(), bounds.Height());
	
		fOffScreenBits = new BBitmap(Bounds(), B_CMAP8, true, false);
		fOffScreenBits->AddChild(fOffScreenView);
	}
#endif

	Invalidate();
}


void
BSlider::KeyDown(const char *bytes, int32 numBytes)
{
	if (!IsEnabled() || IsHidden())
		return;

	int32 newValue = Value();

	switch (bytes[0]) {
		case B_LEFT_ARROW:
		case B_DOWN_ARROW: {
			newValue -= KeyIncrementValue();
			break;
		}
		case B_RIGHT_ARROW:
		case B_UP_ARROW: {
			newValue += KeyIncrementValue();
			break;
		}
		case B_HOME:
			newValue = fMinValue;
			break;
		case B_END:
			newValue = fMaxValue;
			break;
		default:
			BControl::KeyDown(bytes, numBytes);
			return;
	}

	if (newValue != Value()) {
		SetValue(newValue);
		InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
	}
}


/*!
	Makes sure the \a point is within valid bounds.
	Returns \c true if the relevant coordinate (depending on the orientation
	of the slider) differs from \a comparePoint.
*/
bool
BSlider::_ConstrainPoint(BPoint& point, BPoint comparePoint) const
{
	if (fOrientation == B_HORIZONTAL) {
		if (point.x != comparePoint.x) {
			if (point.x < _MinPosition())
				point.x = _MinPosition();
			else if (point.x > _MaxPosition())
				point.x = _MaxPosition();

			return true;
		}
	} else {
		if (point.y != comparePoint.y) {
			if (point.y > _MinPosition())
				point.y = _MinPosition();
			else if (point.y < _MaxPosition())
				point.y = _MaxPosition();

			return true;
		}
	}

	return false;
}


void
BSlider::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;

	if (BarFrame().Contains(point) || ThumbFrame().Contains(point))
		fInitialLocation = _Location();

	uint32 buttons;
	GetMouse(&point, &buttons, true);

	_ConstrainPoint(point, fInitialLocation);
	SetValue(ValueForPoint(point));

	if (_Location() != fInitialLocation)
		InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);

	if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) {
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
	} else {
		// synchronous mouse tracking
		BPoint prevPoint;

		while (buttons) {
			prevPoint = point;

			snooze(SnoozeAmount());
			GetMouse(&point, &buttons, true);

			if (_ConstrainPoint(point, prevPoint)) {
				int32 value = ValueForPoint(point);
				if (value != Value()) {
					SetValue(value);
					InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
				}
			}
		}
		if (_Location() != fInitialLocation)
			Invoke();
	}
}


void
BSlider::MouseUp(BPoint point)
{
	if (IsTracking()) {
		if (_Location() != fInitialLocation)
			Invoke();

		SetTracking(false);
	} else
		BControl::MouseUp(point);
}


void
BSlider::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (IsTracking()) {
		if (_ConstrainPoint(point, _Location())) {
			int32 value = ValueForPoint(point);
			if (value != Value()) {
				SetValue(value);
				InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
			}
		}
	} else
		BControl::MouseMoved(point, transit, message);
}


void
BSlider::Pulse()
{
	BControl::Pulse();
}


void
BSlider::SetLabel(const char *label)
{
	BControl::SetLabel(label);
}


void
BSlider::SetLimitLabels(const char *minLabel, const char *maxLabel)
{
	free(fMinLimitLabel);
	fMinLimitLabel = minLabel ? strdup(minLabel) : NULL;

	free(fMaxLimitLabel);
	fMaxLimitLabel = maxLabel ? strdup(maxLabel) : NULL;

	InvalidateLayout();
	Invalidate();
}


const char*
BSlider::MinLimitLabel() const
{
	return fMinLimitLabel;
}


const char*
BSlider::MaxLimitLabel() const
{
	return fMaxLimitLabel;
}


void
BSlider::SetValue(int32 value)
{
	if (value < fMinValue)
		value = fMinValue;
	if (value > fMaxValue)
		value = fMaxValue;

	if (value == Value())
		return;

	BPoint loc;
	float range = (float)(fMaxValue - fMinValue);
	if (range == 0)
		range = 1;

	float pos = (float)(value - fMinValue) / range *
		_MaxPosition() - _MinPosition();

	if (fOrientation == B_HORIZONTAL) {
		loc.x = ceil(_MinPosition() + pos);
		loc.y = 0;
	} else {
		loc.x = 0;
		loc.y = floor(_MaxPosition() - pos);
	}

	BRect oldThumbFrame = ThumbFrame();

	// While it would be enough to do this dependent on fUseFillColor,
	// that doesn't work out if DrawBar() has been overridden by a sub class
	if (fOrientation == B_HORIZONTAL)
		oldThumbFrame.top = BarFrame().top;
	else
		oldThumbFrame.right = BarFrame().right;

	_SetLocation(loc);

	BControl::SetValueNoUpdate(value);
	BRect invalid = oldThumbFrame | ThumbFrame();

	if (Style() == B_TRIANGLE_THUMB) {
		// 1) we need to take care of pixels touched because of
		//    anti-aliasing
		// 2) we need to update the region with the focus mark as well
		//    (a method BSlider::FocusMarkFrame() would be nice as well)
		if (fOrientation == B_HORIZONTAL) {
			if (IsFocus())
				invalid.bottom += 2;
			invalid.InsetBy(-1, 0);
		} else {
			if (IsFocus())
				invalid.left -= 2;
			invalid.InsetBy(0, -1);
		}
	}

	Invalidate(invalid);

	// update text label

	float oldWidth = 0.0f, width = 0.0f;
	if (fUpdateText != NULL)
		oldWidth = StringWidth(fUpdateText);

	fUpdateText = UpdateText();
	if (fUpdateText != NULL)
		width = StringWidth(fUpdateText);

	width = ceilf(max_c(width, oldWidth)) + 2.0f;
	if (width != 0) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		BRect rect(-width, 0, 0, ceilf(fontHeight.ascent + fontHeight.descent));
		rect.OffsetBy(Bounds().Width(), 0);
		Invalidate(rect);
	}
}


int32
BSlider::ValueForPoint(BPoint location) const
{
	float position = fOrientation == B_HORIZONTAL ? location.x : location.y;
	float min = _MinPosition();
	float max = _MaxPosition();

	if (position < min)
		position = min;
	if (position > max)
		position = max;

	return (int32)roundf(((position - min) * (fMaxValue - fMinValue) / (max - min)) + fMinValue);
}


void
BSlider::SetPosition(float position)
{
	if (position <= 0.0f)
		BControl::SetValue(fMinValue);
	else if (position >= 1.0f)
		BControl::SetValue(fMaxValue);
	else
		BControl::SetValue((int32)(position * (fMaxValue - fMinValue) + fMinValue));
}


float
BSlider::Position() const
{
	float range = (float)(fMaxValue - fMinValue);
	if (range == 0.0f)
		range = 1.0f;

	return (float)(Value() - fMinValue) / range;
}


void
BSlider::SetEnabled(bool on)
{
	BControl::SetEnabled(on);
}


void
BSlider::GetLimits(int32 *minimum, int32 *maximum)
{
	*minimum = fMinValue;
	*maximum = fMaxValue;
}


// #pragma mark - drawing


void
BSlider::Draw(BRect updateRect)
{
	// clear out background
	BRegion background(updateRect);
	background.Exclude(BarFrame());

	// ToDo: the triangle thumb doesn't delete its background, so we still have
	// to do it Note, this also creates a different behaviour for subclasses,
	// depending on the thumb style - if possible this should be avoided.
	if (Style() == B_BLOCK_THUMB)
		background.Exclude(ThumbFrame());

#if USE_OFF_SCREEN_VIEW
	if (!fOffScreenBits)
		return;

	if (fOffScreenBits->Lock()) {
		fOffScreenView->SetViewColor(ViewColor());
		fOffScreenView->SetLowColor(ViewColor());
#endif

		if (background.Frame().IsValid())
			OffscreenView()->FillRegion(&background, B_SOLID_LOW);

#if USE_OFF_SCREEN_VIEW
		fOffScreenView->Sync();
		fOffScreenBits->Unlock();
	}
#endif

	DrawSlider();
}


void
BSlider::DrawSlider()
{
	if (LockLooper()) {
#if USE_OFF_SCREEN_VIEW
		if (!fOffScreenBits)
			return;
		if (fOffScreenBits->Lock()) {
#endif
			DrawBar();
			DrawHashMarks();
			DrawThumb();
			DrawFocusMark();
			DrawText();

#if USE_OFF_SCREEN_VIEW
			fOffScreenView->Sync();
			fOffScreenBits->Unlock();
	
			DrawBitmap(fOffScreenBits, B_ORIGIN);
		}
#endif
		UnlockLooper();
	}
}


void
BSlider::DrawBar()
{
	BRect frame = BarFrame();
	BView *view = OffscreenView();

	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightenmax;
	rgb_color darken1;
	rgb_color darken2;
	rgb_color darkenmax;

	rgb_color barColor;
	rgb_color fillColor;

	if (IsEnabled()) {
		lightenmax	= tint_color(no_tint, B_LIGHTEN_MAX_TINT);
		darken1		= tint_color(no_tint, B_DARKEN_1_TINT);
		darken2		= tint_color(no_tint, B_DARKEN_2_TINT);
		darkenmax	= tint_color(no_tint, B_DARKEN_MAX_TINT);
		barColor	= fBarColor;
		fillColor	= fFillColor;
	} else {
		lightenmax	= tint_color(no_tint, B_LIGHTEN_MAX_TINT);
		darken1		= no_tint;
		darken2		= tint_color(no_tint, B_DARKEN_1_TINT);
		darkenmax	= tint_color(no_tint, B_DARKEN_3_TINT);

		barColor.red	= (fBarColor.red + no_tint.red) / 2;
		barColor.green	= (fBarColor.green + no_tint.green) / 2;
		barColor.blue	= (fBarColor.blue + no_tint.blue) / 2;

		fillColor.red	= (fFillColor.red + no_tint.red) / 2;
		fillColor.green	= (fFillColor.green + no_tint.green) / 2;
		fillColor.blue	= (fFillColor.blue + no_tint.blue) / 2;
	}

	// exclude the block thumb from the bar filling

	BRect lowerFrame = frame.InsetByCopy(1, 1);
	lowerFrame.top++;
	lowerFrame.left++;
	BRect upperFrame = lowerFrame;
	BRect thumbFrame;

	if (Style() == B_BLOCK_THUMB) {
		thumbFrame = ThumbFrame();

		if (fOrientation == B_HORIZONTAL) {
			lowerFrame.right = thumbFrame.left;
			upperFrame.left = thumbFrame.right;
		} else {
			lowerFrame.top = thumbFrame.bottom;
			upperFrame.bottom = thumbFrame.top;
		}
	} else if (fUseFillColor) {
		if (fOrientation == B_HORIZONTAL) {
			lowerFrame.right = floor(lowerFrame.left - 1 + Position()
				* (lowerFrame.Width() + 1));
			upperFrame.left = lowerFrame.right;
		} else {
			lowerFrame.top = floor(lowerFrame.bottom + 1 - Position()
				* (lowerFrame.Height() + 1));
			upperFrame.bottom = lowerFrame.top;
		}
	}

	view->SetHighColor(barColor);
	view->FillRect(upperFrame);

	if (Style() == B_BLOCK_THUMB || fUseFillColor) {
		if (fUseFillColor)
			view->SetHighColor(fillColor);
		view->FillRect(lowerFrame);
	}

	if (Style() == B_BLOCK_THUMB) {
		// We don't want to stroke the lines over the thumb

		PushState();

		BRegion region;
		GetClippingRegion(&region);
		region.Exclude(thumbFrame);
		ConstrainClippingRegion(&region);
	}

	view->SetHighColor(darken1);
	view->StrokeLine(BPoint(frame.left, frame.top),
					 BPoint(frame.left + 1.0f, frame.top));
	view->StrokeLine(BPoint(frame.left, frame.bottom),
					 BPoint(frame.left + 1.0f, frame.bottom));
	view->StrokeLine(BPoint(frame.right - 1.0f, frame.top),
					 BPoint(frame.right, frame.top));

	view->SetHighColor(darken2);
	view->StrokeLine(BPoint(frame.left + 1.0f, frame.top),
					 BPoint(frame.right - 1.0f, frame.top));
	view->StrokeLine(BPoint(frame.left, frame.bottom - 1.0f),
					 BPoint(frame.left, frame.top + 1.0f));

	view->SetHighColor(lightenmax);
	view->StrokeLine(BPoint(frame.left + 1.0f, frame.bottom),
					 BPoint(frame.right, frame.bottom));
	view->StrokeLine(BPoint(frame.right, frame.bottom - 1.0f),
					 BPoint(frame.right, frame.top + 1.0f));

	frame.InsetBy(1.0f, 1.0f);

	view->SetHighColor(darkenmax);
	view->StrokeLine(BPoint(frame.left, frame.bottom),
					 BPoint(frame.left, frame.top));
	view->StrokeLine(BPoint(frame.left + 1.0f, frame.top),
					 BPoint(frame.right, frame.top));

	if (Style() == B_BLOCK_THUMB)
		PopState();
}


void
BSlider::DrawHashMarks()
{
	BRect frame = HashMarksFrame();
	BView *view = OffscreenView();

	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightenmax;
	rgb_color darken2;

	if (IsEnabled()) {
		lightenmax	= tint_color(no_tint, B_LIGHTEN_MAX_TINT);
		darken2		= tint_color(no_tint, B_DARKEN_2_TINT);
	} else {
		lightenmax	= tint_color(no_tint, B_LIGHTEN_2_TINT);
		darken2		= tint_color(no_tint, B_DARKEN_1_TINT);
	}

	float pos = _MinPosition();
	float factor = 0.0f;
	if (fHashMarkCount > 1)
		factor = (_MaxPosition() - pos) / (fHashMarkCount - 1);

	if (fHashMarks & B_HASH_MARKS_TOP && fHashMarkCount > 0) {

		view->BeginLineArray(fHashMarkCount * 2);

		if (fOrientation == B_HORIZONTAL) {
			for (int32 i = 0; i < fHashMarkCount; i++) {
				view->AddLine(BPoint(pos, frame.top),
							  BPoint(pos, frame.top + 5), darken2);
				view->AddLine(BPoint(pos + 1, frame.top),
							  BPoint(pos + 1, frame.top + 5), lightenmax);

				pos += factor;
			}
		} else {
			for (int32 i = 0; i < fHashMarkCount; i++) {
				view->AddLine(BPoint(frame.left, pos),
							  BPoint(frame.left + 5, pos), darken2);
				view->AddLine(BPoint(frame.left, pos + 1),
							  BPoint(frame.left + 5, pos + 1), lightenmax);

				pos += factor;
			}
		}

		view->EndLineArray();
	}

	pos = _MinPosition();

	if (fHashMarks & B_HASH_MARKS_BOTTOM && fHashMarkCount > 0) {
		view->BeginLineArray(fHashMarkCount * 2);

		if (fOrientation == B_HORIZONTAL) {
			for (int32 i = 0; i < fHashMarkCount; i++) {
				view->AddLine(BPoint(pos, frame.bottom - 5),
							  BPoint(pos, frame.bottom), darken2);
				view->AddLine(BPoint(pos + 1, frame.bottom - 5),
							  BPoint(pos + 1, frame.bottom), lightenmax);

				pos += factor;
			}
		} else {
			for (int32 i = 0; i < fHashMarkCount; i++) {
				view->AddLine(BPoint(frame.right - 5, pos),
							  BPoint(frame.right, pos), darken2);
				view->AddLine(BPoint(frame.right - 5, pos + 1),
							  BPoint(frame.right, pos + 1), lightenmax);

				pos += factor;
			}
		}

		view->EndLineArray();
	}
}


void
BSlider::DrawThumb()
{
	if (Style() == B_BLOCK_THUMB)
		_DrawBlockThumb();
	else
		_DrawTriangleThumb();
}


void
BSlider::DrawFocusMark()
{
	if (!IsFocus())
		return;

	OffscreenView()->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));

	BRect frame = ThumbFrame();

	if (fStyle == B_BLOCK_THUMB) {
		frame.left += 2.0f;
		frame.top += 2.0f;
		frame.right -= 3.0f;
		frame.bottom -= 3.0f;
		OffscreenView()->StrokeRect(frame);
	} else {
		if (fOrientation == B_HORIZONTAL) {
			OffscreenView()->StrokeLine(BPoint(frame.left, frame.bottom + 2.0f),
				BPoint(frame.right, frame.bottom + 2.0f));
		} else {
			OffscreenView()->StrokeLine(BPoint(frame.left - 2.0f, frame.top),
				BPoint(frame.left - 2.0f, frame.bottom));
		}
	}
}


void
BSlider::DrawText()
{
	BRect bounds(Bounds());
	BView *view = OffscreenView();

	if (IsEnabled()) {
		view->SetHighColor(0, 0, 0);
	} else {
		view->SetHighColor(tint_color(LowColor(), B_DISABLED_LABEL_TINT));
	}

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	if (Orientation() == B_HORIZONTAL) {
		if (Label())
			view->DrawString(Label(), BPoint(2.0f, ceilf(fontHeight.ascent)));

		// the update text is updated in SetValue() only
		if (fUpdateText != NULL) {
			view->DrawString(fUpdateText, BPoint(bounds.right - StringWidth(fUpdateText)
				- 2.0f, ceilf(fontHeight.ascent)));
		}

		if (fMinLimitLabel)
			view->DrawString(fMinLimitLabel, BPoint(2.0f, bounds.bottom - 4.0f));

		if (fMaxLimitLabel) {
			view->DrawString(fMaxLimitLabel, BPoint(bounds.right
				- StringWidth(fMaxLimitLabel) - 2.0f, bounds.bottom - 4.0f));
		}
	} else {
		if (Label()) {
			view->DrawString(Label(), BPoint(bounds.Width() / 2.0f -
											 StringWidth(Label()) / 2.0f,
											 fontHeight.ascent));
		}

		if (fUpdateText != NULL) {
			view->DrawString(fUpdateText, BPoint(bounds.Width() / 2.0f -
				StringWidth(fUpdateText) / 2.0f, bounds.bottom - fontHeight.descent - 4.0f));
		}

		if (fMaxLimitLabel) {
			view->DrawString(fMaxLimitLabel, BPoint(bounds.Width() / 2.0f -
				StringWidth(fMaxLimitLabel) / 2.0f, fontHeight.ascent + (Label()
					? ceilf(fontHeight.ascent + fontHeight.descent + fontHeight.leading + 2.0f)
					: 0.0f)));
		}

		if (fMinLimitLabel) {
			view->DrawString(fMinLimitLabel, BPoint(bounds.Width() / 2.0f
				- StringWidth(fMinLimitLabel) / 2.0f, bounds.bottom - 2.0f));
		}
	}
}


// #pragma mark -


char*
BSlider::UpdateText() const
{
	return NULL;
}


BRect
BSlider::BarFrame() const
{
	BRect frame(Bounds());

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	float textHeight = (float)ceil(fontHeight.ascent + fontHeight.descent);
	
	if (fStyle == B_BLOCK_THUMB) {
		if (Orientation() == B_HORIZONTAL) {
			frame.left = 8.0f;
			frame.top = 6.0f + (Label() ? textHeight + 4.0f : 0.0f);
			frame.right -= 8.0f;
			frame.bottom = frame.top + fBarThickness;
		} else {
			frame.left = floor((frame.Width() - fBarThickness) / 2.0f);
			frame.top = 12.0f + (Label() ? textHeight : 0.0f) +
				(fMaxLimitLabel ? textHeight : 0.0f);
			frame.right = frame.left + fBarThickness;
			frame.bottom = frame.bottom - 8.0f -
				(fMinLimitLabel ? textHeight + 4 : 0.0f);
		}
	} else {
		if (Orientation() == B_HORIZONTAL) {
			frame.left = 7.0f;
			frame.top = 6.0f + (Label() ? textHeight + 4.0f : 0.0f);
			frame.right -= 7.0f;
			frame.bottom = frame.top + fBarThickness;
		} else {
			frame.left = floor((frame.Width() - fBarThickness) / 2.0f);
			frame.top = 11.0f + (Label() ? textHeight : 0.0f) +
				(fMaxLimitLabel ? textHeight : 0.0f);
			frame.right = frame.left + fBarThickness;
			frame.bottom = frame.bottom - 7.0f -
				(fMinLimitLabel ? textHeight + 4 : 0.0f);
		}
	}

	return frame;
}


BRect
BSlider::HashMarksFrame() const
{
	BRect frame(BarFrame());

	if (fOrientation == B_HORIZONTAL) {
		frame.top -= 6.0f;
		frame.bottom += 6.0f;
	} else {
		frame.left -= 6.0f;
		frame.right += 6.0f;
	}

	return frame;
}


BRect
BSlider::ThumbFrame() const
{
	// TODO: The slider looks really ugly and broken when it is too little.
	// I would suggest using BarFrame() here to get the top and bottom coords
	// and spread them further apart for the thumb

	BRect frame = Bounds();
	font_height fheight;

	GetFontHeight(&fheight);

	float textHeight = (float)ceil(fheight.ascent + fheight.descent);

	if (fStyle == B_BLOCK_THUMB) {
		if (Orientation() == B_HORIZONTAL) {
			frame.left = (float)floor(Position() * (_MaxPosition() - _MinPosition()) +
				_MinPosition()) - 8.0f;
			frame.top = 2.0f + (Label() ? textHeight + 4.0f : 0.0f);
			frame.right = frame.left + 17.0f;
			frame.bottom = frame.top + fBarThickness + 7.0f;
		} else {
			frame.left = floor((frame.Width() - fBarThickness) / 2.0f) - 4;
			frame.top = (float)floor(Position() * (_MaxPosition() - _MinPosition()) +
				_MinPosition()) - 8.0f;
			frame.right = frame.left + fBarThickness + 7.0f;
			frame.bottom = frame.top + 17;
		}
	} else {
		if (Orientation() == B_HORIZONTAL) {
			frame.left = (float)floor(Position() * (_MaxPosition() - _MinPosition()) +
				_MinPosition()) - 6;
			frame.right = frame.left + 12.0f;
			frame.bottom = frame.bottom - 2.0f -
				(MinLimitLabel() || MaxLimitLabel() ? textHeight + 4.0f : 0.0f);
			frame.top = frame.bottom - 8.0f;
		} else {
			frame.left = floor((frame.Width() - fBarThickness) / 2.0f) - 3;
			frame.top = (float)floor(Position() * (_MaxPosition() - _MinPosition())) +
				_MinPosition() - 6.0f;
			frame.right = frame.left + 7;
			frame.bottom = frame.top + 12;
		}
	}

	return frame;
}


void
BSlider::SetFlags(uint32 flags)
{
	BControl::SetFlags(flags);
}


void
BSlider::SetResizingMode(uint32 mode)
{
	BControl::SetResizingMode(mode);
}


void
BSlider::GetPreferredSize(float* _width, float* _height)
{
	BSize preferredSize = PreferredSize();

	if (_width)
		*_width = preferredSize.width;

	if (_height)
		*_height = preferredSize.height;
}


void
BSlider::ResizeToPreferred()
{
	BControl::ResizeToPreferred();
}


status_t
BSlider::Invoke(BMessage *msg)
{
	return BControl::Invoke(msg);
}


BHandler*
BSlider::ResolveSpecifier(BMessage *message, int32 index,
						  BMessage *specifier, int32 command,
						  const char *property)
{
	return BControl::ResolveSpecifier(message, index, specifier,
									  command, property);
}


status_t
BSlider::GetSupportedSuites(BMessage *message)
{
	return BControl::GetSupportedSuites(message);
}


void
BSlider::SetModificationMessage(BMessage *message)
{
	if (fModificationMessage)
		delete fModificationMessage;

	fModificationMessage = message;
}


BMessage*
BSlider::ModificationMessage() const
{
	return fModificationMessage;
}


void
BSlider::SetSnoozeAmount(int32 snoozeTime)
{
	if (snoozeTime < 10000)
		snoozeTime = 10000;
	else if (snoozeTime > 1000000)
		snoozeTime = 1000000;

	fSnoozeAmount = snoozeTime;
}


int32
BSlider::SnoozeAmount() const
{
	return fSnoozeAmount;
}


void
BSlider::SetKeyIncrementValue(int32 increment_value)
{
	fKeyIncrementValue = increment_value;
}


int32
BSlider::KeyIncrementValue() const
{
	return fKeyIncrementValue;
}


void
BSlider::SetHashMarkCount(int32 hash_mark_count)
{
	fHashMarkCount = hash_mark_count;
	Invalidate();
}


int32
BSlider::HashMarkCount() const
{
	return fHashMarkCount;
}


void
BSlider::SetHashMarks(hash_mark_location where)
{
	fHashMarks = where;
	InvalidateLayout();
	Invalidate();
}


hash_mark_location
BSlider::HashMarks() const
{
	return fHashMarks;
}


void
BSlider::SetStyle(thumb_style style)
{
	fStyle = style;
	InvalidateLayout();
	Invalidate();
}


thumb_style
BSlider::Style() const
{
	return fStyle;
}


void
BSlider::SetBarColor(rgb_color bar_color)
{
	fBarColor = bar_color;
	Invalidate();
}


rgb_color
BSlider::BarColor() const
{
	return fBarColor;
}


void
BSlider::UseFillColor(bool use_fill, const rgb_color *bar_color)
{
	fUseFillColor = use_fill;

	if (use_fill && bar_color)
		fFillColor = *bar_color;

	Invalidate();
}


bool
BSlider::FillColor(rgb_color *bar_color) const
{
	if (bar_color && fUseFillColor)
		*bar_color = fFillColor;

	return fUseFillColor;
}


BView*
BSlider::OffscreenView() const
{
#if USE_OFF_SCREEN_VIEW
	return fOffScreenView;
#else
	return (BView*)this;
#endif
}


orientation
BSlider::Orientation() const
{
	return fOrientation;
}


void
BSlider::SetOrientation(orientation posture)
{
	fOrientation = posture;
	InvalidateLayout();
	Invalidate();
}


float
BSlider::BarThickness() const
{
	return fBarThickness;
}


void
BSlider::SetBarThickness(float thickness)
{
	if (thickness < 1.0)
		thickness = 1.0;
	if (thickness != fBarThickness) {
		fBarThickness = thickness;
		InvalidateLayout();
	}
}


void
BSlider::SetFont(const BFont *font, uint32 properties)
{
	BControl::SetFont(font, properties);

#if USE_OFF_SCREEN_VIEW
	if (fOffScreenView && fOffScreenBits) {
		if (fOffScreenBits->Lock()) {
			fOffScreenView->SetFont(font, properties);
			fOffScreenBits->Unlock();
		}
	}
#endif

	InvalidateLayout();
}


void
BSlider::SetLimits(int32 minimum, int32 maximum)
{
	if (minimum <= maximum) {
		fMinValue = minimum;
		fMaxValue = maximum;
	
		int32 value = Value();
		value = max_c(minimum, value);
		value = min_c(maximum, value);

		if (value != Value()) {
			SetValue(value);
		}
	}
}


// #pragma mark - layout related


void
BSlider::InvalidateLayout(bool descendants)
{
	// invalidate cached preferred size
	fMinSize.Set(-1, -1);

	BControl::InvalidateLayout(descendants);
}


BSize
BSlider::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(),
		_ValidateMinSize());
}


BSize
BSlider::MaxSize()
{
	BSize maxSize = _ValidateMinSize();
	if (fOrientation == B_HORIZONTAL)	
		maxSize.width = B_SIZE_UNLIMITED;
	else
		maxSize.height = B_SIZE_UNLIMITED;
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), maxSize);
}


BSize
BSlider::PreferredSize()
{
	BSize preferredSize = _ValidateMinSize();
	if (fOrientation == B_HORIZONTAL)	
		preferredSize.width = max_c(100.0, preferredSize.width);
	else
		preferredSize.height = max_c(100.0, preferredSize.height);
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), preferredSize);
}


// #pragma mark - private

void
BSlider::_DrawBlockThumb()
{
	BRect frame = ThumbFrame();
	BView *view = OffscreenView();

	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lighten2;
	rgb_color lighten1;
	rgb_color darken2;
	rgb_color darken3;
	rgb_color darkenmax;

	if (IsEnabled()) {
		lighten2	= tint_color(no_tint, B_LIGHTEN_2_TINT);
		lighten1	= no_tint;
		darken2		= tint_color(no_tint, B_DARKEN_2_TINT);
		darken3		= tint_color(no_tint, B_DARKEN_3_TINT);
		darkenmax	= tint_color(no_tint, B_DARKEN_MAX_TINT);
	} else {
		lighten2	= tint_color(no_tint, B_LIGHTEN_2_TINT);
		lighten1	= tint_color(no_tint, B_LIGHTEN_1_TINT);
		darken2		= tint_color(no_tint, (B_NO_TINT + B_DARKEN_1_TINT) / 2.0);
		darken3		= tint_color(no_tint, B_DARKEN_1_TINT);
		darkenmax	= tint_color(no_tint, B_DARKEN_3_TINT);
	}

	// blank background for shadow
	// ToDo: this also draws over the hash marks (though it's not *that* noticeable)
	view->SetHighColor(no_tint);
	view->StrokeLine(BPoint(frame.left, frame.top),
					 BPoint(frame.left, frame.top));

	BRect barFrame = BarFrame();
	if (barFrame.right >= frame.right) {
		// leave out barFrame from shadow background clearing
		view->StrokeLine(BPoint(frame.right, frame.top),
						 BPoint(frame.right, barFrame.top - 1.0f));
		view->StrokeLine(BPoint(frame.right, barFrame.bottom + 1.0f),
						 BPoint(frame.right, frame.bottom));
	} else {
		view->StrokeLine(BPoint(frame.right, frame.top),
						 BPoint(frame.right, frame.bottom));
	}

	view->StrokeLine(BPoint(frame.left, frame.bottom),
					 BPoint(frame.right - 1.0f, frame.bottom));
	view->StrokeLine(BPoint(frame.left, frame.bottom - 1.0f),
					 BPoint(frame.left, frame.bottom - 1.0f));
	view->StrokeLine(BPoint(frame.right - 1.0f, frame.top),
					 BPoint(frame.right - 1.0f, frame.top));

	// Outline (top, left)
	view->SetHighColor(darken3);
	view->StrokeLine(BPoint(frame.left, frame.bottom - 2.0f),
					 BPoint(frame.left, frame.top + 1.0f));
	view->StrokeLine(BPoint(frame.left + 1.0f, frame.top),
					 BPoint(frame.right - 2.0f, frame.top));

	// Shadow
	view->SetHighColor(0, 0, 0, IsEnabled() ? 100 : 50);
	view->SetDrawingMode(B_OP_ALPHA);
	view->StrokeLine(BPoint(frame.right, frame.top + 2.0f),
					 BPoint(frame.right, frame.bottom - 1.0f));
	view->StrokeLine(BPoint(frame.left + 2.0f, frame.bottom),
					 BPoint(frame.right - 1.0f, frame.bottom));

	view->SetDrawingMode(B_OP_COPY);
	view->SetHighColor(darken3);
	view->StrokeLine(BPoint(frame.right - 1.0f, frame.bottom - 1.0f),
					 BPoint(frame.right - 1.0f, frame.bottom - 1.0f));


	// First bevel
	frame.InsetBy(1.0f, 1.0f);

	view->SetHighColor(darkenmax);
	view->StrokeLine(BPoint(frame.left, frame.bottom),
					 BPoint(frame.right - 1.0f, frame.bottom));
	view->StrokeLine(BPoint(frame.right, frame.bottom - 1.0f),
					 BPoint(frame.right, frame.top));

	view->SetHighColor(lighten2);
	view->StrokeLine(BPoint(frame.left, frame.top),
					 BPoint(frame.left, frame.bottom - 1.0f));
	view->StrokeLine(BPoint(frame.left + 1.0f, frame.top),
					 BPoint(frame.right - 1.0f, frame.top));

	frame.InsetBy(1.0f, 1.0f);

	view->FillRect(BRect(frame.left, frame.top, frame.right - 1.0f, frame.bottom - 1.0f));

	// Second bevel and center dots
	view->SetHighColor(darken2);
	view->StrokeLine(BPoint(frame.left, frame.bottom),
					 BPoint(frame.right, frame.bottom));
	view->StrokeLine(BPoint(frame.right, frame.bottom - 1.0f),
					 BPoint(frame.right, frame.top));

	if (Orientation() == B_HORIZONTAL) {
		view->StrokeLine(BPoint(frame.left + 6.0f, frame.top + 2.0f),
						 BPoint(frame.left + 6.0f, frame.top + 2.0f));
		view->StrokeLine(BPoint(frame.left + 6.0f, frame.top + 4.0f),
						 BPoint(frame.left + 6.0f, frame.top + 4.0f));
		view->StrokeLine(BPoint(frame.left + 6.0f, frame.top + 6.0f),
						 BPoint(frame.left + 6.0f, frame.top + 6.0f));
	} else {
		view->StrokeLine(BPoint(frame.left + 2.0f, frame.top + 6.0f),
						 BPoint(frame.left + 2.0f, frame.top + 6.0f));
		view->StrokeLine(BPoint(frame.left + 4.0f, frame.top + 6.0f),
						 BPoint(frame.left + 4.0f, frame.top + 6.0f));
		view->StrokeLine(BPoint(frame.left + 6.0f, frame.top + 6.0f),
						 BPoint(frame.left + 6.0f, frame.top + 6.0f));
	}

	frame.InsetBy(1.0f, 1.0f);

	// Third bevel
	view->SetHighColor(lighten1);
	view->StrokeLine(BPoint(frame.left, frame.bottom),
					 BPoint(frame.right, frame.bottom));
	view->StrokeLine(BPoint(frame.right, frame.bottom - 1.0f),
					 BPoint(frame.right, frame.top));
}


void
BSlider::_DrawTriangleThumb()
{
	BRect frame = ThumbFrame();
	BView *view = OffscreenView();

	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightenmax;
	rgb_color lighten1;
	rgb_color darken2;
	rgb_color darken3;
	rgb_color darkenmax;

	if (IsEnabled()) {
		lightenmax	= tint_color(no_tint, B_LIGHTEN_MAX_TINT);
		lighten1	= no_tint;
		darken2		= tint_color(no_tint, B_DARKEN_2_TINT);
		darken3		= tint_color(no_tint, B_DARKEN_3_TINT);
		darkenmax	= tint_color(no_tint, B_DARKEN_MAX_TINT);
	} else {
		lightenmax	= tint_color(no_tint, B_LIGHTEN_2_TINT);
		lighten1	= tint_color(no_tint, B_LIGHTEN_1_TINT);
		darken2		= tint_color(no_tint, (B_NO_TINT + B_DARKEN_1_TINT) / 2.0);
		darken3		= tint_color(no_tint, B_DARKEN_1_TINT);
		darkenmax	= tint_color(no_tint, B_DARKEN_3_TINT);
	}

	view->SetDrawingMode(B_OP_OVER);

	if (Orientation() == B_HORIZONTAL) {
		view->SetHighColor(lighten1);
		view->FillTriangle(BPoint(frame.left + 1.0, frame.bottom - 2.0),
						   BPoint(frame.left + 6.0, frame.top + 1.0),
						   BPoint(frame.right - 1.0, frame.bottom - 2.0));

		view->SetHighColor(no_tint);
		view->StrokeLine(BPoint(frame.right - 2.0, frame.bottom - 2.0),
						 BPoint(frame.left + 3.0, frame.bottom - 2.0));

		view->SetHighColor(darkenmax);
		view->StrokeLine(BPoint(frame.left, frame.bottom),
						 BPoint(frame.right, frame.bottom));
		view->StrokeLine(BPoint(frame.right, frame.bottom - 1.0),
						 BPoint(frame.left + 6.0, frame.top + 1.0));

		view->SetHighColor(darken2);
		view->StrokeLine(BPoint(frame.right - 1.0, frame.bottom - 1.0),
						 BPoint(frame.left + 1.0, frame.bottom - 1.0));
		view->SetHighColor(darken3);
		view->StrokeLine(BPoint(frame.left, frame.bottom - 1.0),
						 BPoint(frame.left + 5.0, frame.top + 2.0));

		view->SetHighColor(lightenmax);
		view->StrokeLine(BPoint(frame.left + 2.0, frame.bottom - 2.0),
						 BPoint(frame.left + 6.0, frame.top + 2.0));
	} else {
		view->SetHighColor(lighten1);
		view->FillTriangle(BPoint(frame.left + 1.0f, frame.top),
			BPoint(frame.left + 7.0f, frame.top + 6.0f),
			BPoint(frame.left + 1.0f, frame.bottom));

		view->SetHighColor(darkenmax);
		view->StrokeLine(BPoint(frame.left, frame.top + 1),
			BPoint(frame.left, frame.bottom));
		view->StrokeLine(BPoint(frame.left + 1.0f, frame.bottom),
			BPoint(frame.left + 7.0f, frame.top + 6.0f));

		view->SetHighColor(darken2);
		view->StrokeLine(BPoint(frame.left, frame.top),
			BPoint(frame.left, frame.bottom - 1));
		view->StrokeLine(BPoint(frame.left + 1.0f, frame.top),
			BPoint(frame.left + 6.0f, frame.top + 5.0f));

		view->SetHighColor(no_tint);
		view->StrokeLine(BPoint(frame.left + 1.0f, frame.top + 2.0f),
			BPoint(frame.left + 1.0f, frame.bottom - 1.0f));
		view->StrokeLine(BPoint(frame.left + 2.0f, frame.bottom - 2.0f),
			BPoint(frame.left + 6.0f, frame.top + 6.0f));
	}

	view->SetDrawingMode(B_OP_COPY);
}


BPoint
BSlider::_Location() const
{
	return fLocation;
}


void
BSlider::_SetLocation(BPoint p)
{
	fLocation = p;
}


float
BSlider::_MinPosition() const
{
	if (fOrientation == B_HORIZONTAL)
		return BarFrame().left + 1.0f;

	return BarFrame().bottom - 1.0f;
}


float
BSlider::_MaxPosition() const
{
	if (fOrientation == B_HORIZONTAL)
		return BarFrame().right - 1.0f;

	return BarFrame().top + 1.0f;
}


BSize
BSlider::_ValidateMinSize()
{
	if (fMinSize.width >= 0) {
		// the preferred size is up to date
		return fMinSize;
	}

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	float width = 0.0;
	float height = 0.0;
	int32 rows = 0;

	if (Orientation() == B_HORIZONTAL) {
		height = 12.0 + fBarThickness;

		float labelWidth = 0;
		if (Label()) {
			labelWidth = StringWidth(Label());
			rows++;
		}

		if (MinLimitLabel())
			width = StringWidth(MinLimitLabel());
		if (MaxLimitLabel()) {
			// some space between the labels
			if (MinLimitLabel())
				width += 8.0;

			width += StringWidth(MaxLimitLabel());
		}

		if (labelWidth > width)
			width = labelWidth;
		if (width < 32.0)
			width = 32.0;

		if (MinLimitLabel() || MaxLimitLabel())
			rows++;

		height += rows * (ceilf(fontHeight.ascent)
			+ ceilf(fontHeight.descent) + 4.0);
	} else { 
		// B_VERTICAL
		width = 12.0 + fBarThickness;

		// find largest label
		float labelWidth = 0;
		if (Label()) {
			labelWidth = StringWidth(Label());
			rows++;
		}
		if (MinLimitLabel()) {
			labelWidth = max_c(labelWidth, StringWidth(MinLimitLabel()));
			rows++;
		}
		if (MaxLimitLabel()) {
			labelWidth = max_c(labelWidth, StringWidth(MaxLimitLabel()));
			rows++;
		}

		width = max_c(labelWidth, width);

		height = 32.0 + rows * (ceilf(fontHeight.ascent)
			+ ceilf(fontHeight.descent) + 4.0);

		if (Label() && MaxLimitLabel())
			height -= 4.0f;
	}

	fMinSize.width = width;
	fMinSize.height = height;

	return fMinSize;
}


// #pragma mark - FBC padding

void BSlider::_ReservedSlider5() {}
void BSlider::_ReservedSlider6() {}
void BSlider::_ReservedSlider7() {}
void BSlider::_ReservedSlider8() {}
void BSlider::_ReservedSlider9() {}
void BSlider::_ReservedSlider10() {}
void BSlider::_ReservedSlider11() {}
void BSlider::_ReservedSlider12() {}


BSlider &
BSlider::operator=(const BSlider &)
{
	return *this;
}


//	#pragma mark - R4.5 compatibility


#if __GNUC__ < 3

extern "C"
void _ReservedSlider4__7BSlider(BSlider *slider, int32 minimum, int32 maximum)
{
	slider->BSlider::SetLimits(minimum, maximum);
}

extern "C" void _ReservedSlider1__7BSlider() {}
extern "C" void _ReservedSlider2__7BSlider() {}
extern "C" void _ReservedSlider3__7BSlider() {}

#endif	// __GNUC__ < 3
