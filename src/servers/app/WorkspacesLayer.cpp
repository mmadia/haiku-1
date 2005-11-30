/*
 * Copyright 2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include <ColorSet.h>
#include <WindowPrivate.h>

#include "AppServer.h"
#include "DrawingEngine.h"
#include "RootLayer.h"
#include "WindowLayer.h"
#include "Workspace.h"

#include "WorkspacesLayer.h"

WorkspacesLayer::WorkspacesLayer(BRect frame, const char* name,
	int32 token, uint32 resizeMode, uint32 flags, DrawingEngine* driver)
	: Layer(frame, name, token, resizeMode, flags, driver)
{
}


WorkspacesLayer::~WorkspacesLayer()
{
}


void
WorkspacesLayer::_GetGrid(int32& columns, int32& rows)
{
	DesktopSettings settings(Window()->Desktop());
	int32 count = settings.WorkspacesCount();

	rows = 1;
	for (int32 i = 2; i < count; i++) {
		if (count % i == 0)
			rows = i;
	}

	columns = count / rows;
}


BRect
WorkspacesLayer::_WorkspaceAt(int32 i)
{
	int32 columns, rows;
	_GetGrid(columns, rows);

	int32 width = Frame().IntegerWidth() / columns;
	int32 height = Frame().IntegerHeight() / rows;

	int32 column = i % columns;
	int32 row = i / columns;

	BRect rect(column * width, row * height, (column + 1) * width, (row + 1) * height);

	// make sure there is no gap anywhere
	if (column == columns - 1)
		rect.right = Frame().right;
	if (row == rows - 1)
		rect.bottom = Frame().bottom;

	BPoint pt(0,0);
	ConvertToScreen(&pt);
	rect.OffsetBy(pt);
	return rect;
}


BRect
WorkspacesLayer::_WindowFrame(const BRect& workspaceFrame,
	const BRect& screenFrame, const BRect& windowFrame,
	BPoint windowPosition)
{
	BRect frame = windowFrame;
	frame.OffsetTo(windowPosition);

	float factor = workspaceFrame.Width() / screenFrame.Width();
	frame.left = rintf(frame.left * factor);
	frame.right = rintf(frame.right * factor);

	factor = workspaceFrame.Height() / screenFrame.Height();
	frame.top = rintf(frame.top * factor);
	frame.bottom = rintf(frame.bottom * factor);

	frame.OffsetBy(workspaceFrame.LeftTop());
	return frame;
}


void
WorkspacesLayer::_DrawWindow(const BRect& workspaceFrame,
	const BRect& screenFrame, WindowLayer* window, BPoint windowPosition,
	BRegion& backgroundRegion, bool active)
{
	if (window->Feel() == kDesktopWindowFeel)
		return;

	BPoint offset = window->Frame().LeftTop() - windowPosition;
	BRect frame = _WindowFrame(workspaceFrame, screenFrame, window->Frame(),
		windowPosition);
	BRect tabFrame = window->GetDecorator()->GetTabRect();
	tabFrame = _WindowFrame(workspaceFrame, screenFrame,
		tabFrame, tabFrame.LeftTop() - offset);

	// ToDo: let decorator do this!
	RGBColor yellow = window->GetDecorator()->GetColors().window_tab;
	RGBColor gray(180, 180, 180);
	RGBColor white(255, 255, 255);

	if (!active) {
		_DarkenColor(yellow);
		_DarkenColor(gray);
		_DarkenColor(white);
	}

	if (tabFrame.left < frame.left)
		tabFrame.left = frame.left;
	if (tabFrame.right >= frame.right)
		tabFrame.right = frame.right - 1;

	tabFrame.top = frame.top - 1;
	tabFrame.bottom = frame.top - 1;

	backgroundRegion.Exclude(tabFrame);
	backgroundRegion.Exclude(frame);

	GetDrawingEngine()->StrokeLine(tabFrame.LeftTop(), tabFrame.RightBottom(), yellow);

	GetDrawingEngine()->StrokeRect(frame, gray);

	frame.InsetBy(1, 1);
	GetDrawingEngine()->FillRect(frame, white);
}


void
WorkspacesLayer::_DrawWorkspace(int32 index)
{
	BRect rect = _WorkspaceAt(index);

	Workspace workspace(*Window()->Desktop(), index);
	bool active = workspace.IsCurrent();
	if (active) {
		// draw active frame
		RGBColor black(0, 0, 0);
		GetDrawingEngine()->StrokeRect(rect, black);
	}

	rect.InsetBy(1, 1);

	RGBColor color = workspace.Color();
	if (!active)
		_DarkenColor(color);

	// draw windows

	BRegion backgroundRegion = VisibleRegion();

	// ToDo: would be nice to get the real update region here

	uint16 width, height;
	uint32 colorSpace;
	float frequency;
	Window()->Desktop()->ScreenAt(0)->GetMode(width, height,
		colorSpace, frequency);
	BRect screenFrame(0, 0, width - 1, height - 1);

	BRegion workspaceRegion(rect);
	backgroundRegion.IntersectWith(&workspaceRegion);
	GetDrawingEngine()->ConstrainClippingRegion(&backgroundRegion);

	WindowLayer* window;
	BPoint leftTop;
	while (workspace.GetNextWindow(window, leftTop) == B_OK) {
		_DrawWindow(rect, screenFrame, window, leftTop,
			backgroundRegion, active);
	}

	// draw background

	GetDrawingEngine()->ConstrainClippingRegion(&backgroundRegion);
	GetDrawingEngine()->FillRect(rect, color);

	// TODO: ConstrainClippingRegion() should accept a const parameter !!
	BRegion cRegion(VisibleRegion());
	GetDrawingEngine()->ConstrainClippingRegion(&cRegion);
}


void
WorkspacesLayer::_DarkenColor(RGBColor& color) const
{
	color = tint_color(color.GetColor32(), B_DARKEN_2_TINT);
}


void
WorkspacesLayer::Draw(const BRect& updateRect)
{
	// ToDo: either draw into an off-screen bitmap, or turn off flickering...

	int32 columns, rows;
	_GetGrid(columns, rows);

	// draw grid
	// horizontal lines

	BRect frame = Frame();
	BPoint pt(0,0);
	ConvertToScreen(&pt);
	frame.OffsetBy(pt);

	GetDrawingEngine()->StrokeLine(BPoint(frame.left, frame.top),
		BPoint(frame.right, frame.top), ViewColor());

	for (int32 row = 0; row < rows; row++) {
		BRect rect = _WorkspaceAt(row * columns);
		GetDrawingEngine()->StrokeLine(BPoint(frame.left, rect.bottom),
			BPoint(frame.right, rect.bottom), ViewColor());
	}

	// vertical lines

	GetDrawingEngine()->StrokeLine(BPoint(frame.left, frame.top),
		BPoint(frame.left, frame.bottom), ViewColor());

	for (int32 column = 0; column < columns; column++) {
		BRect rect = _WorkspaceAt(column);
		GetDrawingEngine()->StrokeLine(BPoint(rect.right, frame.top),
			BPoint(rect.right, frame.bottom), ViewColor());
	}

	// draw workspaces

	for (int32 i = rows * columns; i-- > 0;) {
		_DrawWorkspace(i);
	}
}


void
WorkspacesLayer::MouseDown(BMessage* message, BPoint where, int32* _viewToken)
{
	int32 columns, rows;
	_GetGrid(columns, rows);

	for (int32 i = columns * rows; i-- > 0;) {
		BRect rect = _WorkspaceAt(i);

		if (rect.Contains(where)) {
			Window()->Desktop()->SetWorkspace(i);
			break;
		}
	}

	Layer::MouseDown(message, where, _viewToken);
}


