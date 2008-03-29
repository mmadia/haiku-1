/*
 * Copyright 2002-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 *		Hartmut Reh
 */
 
// TODO Remove. Use shared/libprint/Preview.cpp instead.

#include <stdlib.h>

#include <Application.h>
#include <Debug.h>
#include <String.h>
#include "BeUtils.h"
#include "Utils.h"

#include "Preview.h"

// Implementation of PreviewPage
PreviewPage::PreviewPage(int32 page, PrintJobPage* pjp)
	: fPage(page)
	, fPictures(NULL)
	, fPoints(NULL)
	, fRects(NULL)
{
	fNumberOfPictures = pjp->NumberOfPictures();
	fPictures = new BPicture[fNumberOfPictures];
	fPoints = new BPoint[fNumberOfPictures];
	fRects = new BRect[fNumberOfPictures];
	status_t rc = B_ERROR;
	for (int32 i = 0; i < fNumberOfPictures && 
		(rc = pjp->NextPicture(fPictures[i], fPoints[i], fRects[i])) == B_OK; i ++);
	fStatus = rc;
}

PreviewPage::~PreviewPage() {
	delete []fPictures;
	delete []fPoints;
	delete []fRects;
}

status_t PreviewPage::InitCheck() const {
	return fStatus;
}

void PreviewPage::Draw(BView* view) 
{
	ASSERT(fStatus == B_OK);
	for (int32 i = 0; i < fNumberOfPictures; i ++) 
	{
		view->DrawPicture(&fPictures[i], fPoints[i]);
	}
}

// Implementation of PreviewView

const float kPreviewTopMargin = 10;
const float kPreviewBottomMargin = 30;
const float kPreviewLeftMargin = 10;
const float kPreviewRightMargin = 30;

const uint8 ZOOM_IN[] = { 16, 1, 6, 6, 0, 0, 15, 128, 48, 96, 32, 32, 66, 16, 66, 16,
	79, 144, 66, 16, 66, 16, 32, 32, 48, 112, 15, 184, 0, 28, 0, 14, 0, 6, 0, 0, 15,
	128, 63, 224, 127, 240, 127, 240, 255, 248, 255, 248, 255, 248, 255, 248, 255, 248,
	127, 248, 127, 248, 63, 252, 15, 254, 0, 31, 0, 15, 0, 7 };

const uint8 ZOOM_OUT[] = { 16, 1, 6, 6, 0, 0, 15, 128, 48, 96, 32, 32, 64, 16, 64, 16,
	79, 144, 64, 16, 64, 16, 32, 32, 48, 112, 15, 184, 0, 28, 0, 14, 0, 6, 0, 0, 15,
	128, 63, 224, 127, 240, 127, 240, 255, 248, 255, 248, 255, 248, 255, 248, 255, 248,
	127, 248, 127, 248, 63, 252, 15, 254, 0, 31, 0, 15, 0, 7 };

PreviewView::PreviewView(BFile* jobFile, BRect rect)
	: BView(rect, "PreviewView", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
	, fPage(0)
	, fZoom(0)
	, fTracking(false)
	, fInsideView(true)
	, fReader(jobFile)
	, fCachedPage(NULL)
{
}

PreviewView::~PreviewView() {
	delete fCachedPage;
}


void
PreviewView::Show()
{
	BView::Show();
	be_app->SetCursor(ZOOM_IN);
}


void
PreviewView::MouseDown(BPoint point)
{
	MakeFocus(true);
	BMessage *message = Window()->CurrentMessage();

	int32 button;
	if (message && message->FindInt32("buttons", &button) == B_OK) {
		if (button == B_PRIMARY_MOUSE_BUTTON) {
			int32 modifier;
			if (message->FindInt32("modifiers", &modifier) == B_OK) {
				if (modifier & B_SHIFT_KEY)
					ZoomOut();
				else
					ZoomIn();
			}
		}

		if (button == B_SECONDARY_MOUSE_BUTTON) {
			fTracking = true;
			be_app->SetCursor(B_HAND_CURSOR);
			SetMouseEventMask(B_POINTER_EVENTS,
				B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
			fScrollStart = Bounds().LeftTop() + ConvertToScreen(point);
		}
	}
}


void
PreviewView::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if (fTracking) {
		uint32 button;
		GetMouse(&point, &button, false);
		point = fScrollStart - ConvertToScreen(point);

		float hMin, hMax;
		BScrollBar *hBar = ScrollBar(B_HORIZONTAL);
		hBar->GetRange(&hMin, &hMax);

		float vMin, vMax;
		BScrollBar *vBar = ScrollBar(B_VERTICAL);
		vBar->GetRange(&vMin, &vMax);

		if (point.x < 0.0) point.x = 0.0;
		if (point.y < 0.0) point.y = 0.0;
		if (point.x > hMax) point.x = hMax;
		if (point.y > vMax) point.y = vMax;

		ScrollTo(point);
	} else {
		switch (transit) {
			case B_ENTERED_VIEW: {
				fInsideView = true;
				be_app->SetCursor(ZOOM_IN);
			}	break;

			case B_EXITED_VIEW: {
				fInsideView = false;
				be_app->SetCursor(B_HAND_CURSOR);
			}	break;

			default: {
				if (modifiers() & B_SHIFT_KEY)
					be_app->SetCursor(ZOOM_OUT);
				else
					be_app->SetCursor(ZOOM_IN);
			}	break;
		}
	}
}


void
PreviewView::MouseUp(BPoint point)
{
	(void)point;
	fTracking = false;
	fScrollStart.Set(0.0, 0.0);
	if (fInsideView && ((modifiers() & B_SHIFT_KEY) == 0))
		be_app->SetCursor(ZOOM_IN);
}


void
PreviewView::KeyDown(const char* bytes, int32 numBytes)
{
	MakeFocus(true);	
	switch (bytes[0]) {
		case '-': {
			if (modifiers() & B_CONTROL_KEY)
				ZoomOut();
		}	break;

		case '+' : {
			if (modifiers() & B_CONTROL_KEY)
				ZoomIn();
		}	break;

		default: {
			BView::KeyDown(bytes, numBytes);
		}	break;
	}
}


// returns 2 ^ fZoom
float PreviewView::ZoomFactor() const {
	const int32 b = 4;
	int32 zoom;
	if (fZoom > 0) {
		zoom = (1 << b) << fZoom;
	} else {
		zoom = (1 << b) >> -fZoom;
	}
	float factor = zoom / (float)(1 << b);
	return factor * fReader.GetScale() / 100.0;
}

BRect PreviewView::PageRect() const {
	float f = ZoomFactor();
	BRect r = fReader.PaperRect();
	return ScaleRect(r, f);
}

BRect PreviewView::PrintableRect() const {
	float f = ZoomFactor();
	BRect r = fReader.PrintableRect();
	return ScaleRect(r, f);
}

BRect PreviewView::ViewRect() const {
	BRect r(PageRect());
	r.right += kPreviewLeftMargin + kPreviewRightMargin;
	r.bottom += kPreviewTopMargin + kPreviewBottomMargin;
	return r;
}

status_t PreviewView::InitCheck() const {
	return fReader.InitCheck();
}

bool PreviewView::IsPageLoaded(int32 page) const {
	return fCachedPage != NULL && fCachedPage->Page() == page;
}

bool PreviewView::IsPageValid() const {
	return fCachedPage && fCachedPage->InitCheck() == B_OK;
}

void PreviewView::LoadPage(int32 page) {
	delete fCachedPage; fCachedPage = NULL;
	PrintJobPage pjp;
	if (fReader.GetPage(page, pjp) == B_OK) {
		fCachedPage = new PreviewPage(page, &pjp);
	}
}

void PreviewView::DrawPageFrame(BRect rect) {
	const float kShadowIndent = 3;
	const float kShadowWidth = 3;
	float x, y;
	float right, bottom;
	rgb_color frameColor = {0, 0, 0, 0};
	rgb_color shadowColor = {90, 90, 90, 0};
	BRect r(PageRect());
	
	PushState();
	// draw page border around page
	r.InsetBy(-1, -1);
	r.OffsetTo(kPreviewLeftMargin-1, kPreviewTopMargin-1);
	SetHighColor(frameColor);
	StrokeRect(r);
	
	// draw page shadow
	SetHighColor(shadowColor);

	x = r.right + 1;
	right = x + kShadowWidth;
	bottom = r.bottom + 1 + kShadowWidth;
	y = r.top + kShadowIndent;
	FillRect(BRect(x, y, right, bottom));

	x = r.left + kShadowIndent;
	y = r.bottom  + 1;
	FillRect(BRect(x, y, r.right, bottom));
	PopState();
}



void PreviewView::DrawPage(BRect rect) 
{		
	// constrain clipping region to printable rectangle
	BRect r(PrintableRect());
	r.OffsetBy(kPreviewLeftMargin, kPreviewTopMargin);
	BRegion clip(r);
	// Text drawing does not work if clipping region
	// is constraint.
	// TODO enable after app_server bug has been fixed
	// ConstrainClippingRegion(&clip);	

	// draw page contents
	PushState();
	
	// print job coordinates are relative to the printable rect
	BRect printRect = fReader.PrintableRect();
	SetOrigin(kPreviewLeftMargin + printRect.left * ZoomFactor(), 
		kPreviewTopMargin + printRect.top * ZoomFactor());
	
	SetScale(ZoomFactor());
	
		PushState();
		fCachedPage->Draw(this);
		PopState();
		
	PopState();
}

void PreviewView::Draw(BRect rect) {
	if (fReader.InitCheck() == B_OK) {
		if (!IsPageLoaded(fPage)) {
			LoadPage(fPage);
		}
		if (IsPageValid()) {
			DrawPageFrame(rect);
			DrawPage(rect);
		}
	}
}

void PreviewView::FrameResized(float width, float height) {
	FixScrollbars();
}

bool PreviewView::ShowsFirstPage() const {
	return fPage == 0;
}

bool PreviewView::ShowsLastPage() const {
	return fPage == NumberOfPages() - 1;
}

int PreviewView::NumberOfPages() const {
	return fReader.NumberOfPages();
}

void PreviewView::ShowNextPage() {
	if (!ShowsLastPage()) {
		fPage ++; 
		Invalidate();
	}
}

void PreviewView::ShowPrevPage() {
	if (!ShowsFirstPage()) {
		fPage --; 
		Invalidate();
	}
}
	
void PreviewView::ShowFirstPage() {
	if (!ShowsFirstPage()) {
		fPage = 0; 
		Invalidate();
	}
}

void PreviewView::ShowLastPage() {
	if (!ShowsLastPage()) {
		fPage = NumberOfPages()-1; 
		Invalidate();
	}
}

void PreviewView::ShowFindPage(int page) {
	page --;
	
	if (page < 0) {
		page = 0;
	} else if (page > (NumberOfPages()-1)) {
		page = NumberOfPages()-1;
	}
	
	fPage = (int32)page;

	Invalidate();
}

bool PreviewView::CanZoomIn() const {
	return fZoom < 4;
}

bool PreviewView::CanZoomOut() const {
	return fZoom > -2;
}

void PreviewView::ZoomIn() {
	if (CanZoomIn()) {
		fZoom ++; FixScrollbars(); 
		Invalidate();
	}
}

void PreviewView::ZoomOut() {
	if (CanZoomOut()) {
		fZoom --; FixScrollbars(); 
		Invalidate();
	}
}

void PreviewView::FixScrollbars() {
	BRect frame = Bounds();
	BScrollBar * scroll;
	float x, y;
	float bigStep, smallStep;
	float width = PageRect().Width() + kPreviewLeftMargin + kPreviewRightMargin;
	float height = PageRect().Height() + kPreviewTopMargin + kPreviewBottomMargin;
	x = width - frame.Width();
	if (x < 0.0) {
		x = 0.0;
	}
	y = height - frame.Height();
	if (y < 0.0) {
		y = 0.0;
	}
	
	scroll = ScrollBar (B_HORIZONTAL);
	scroll->SetRange (0.0, x);
	scroll->SetProportion ((width - x) / width);
	bigStep = frame.Width() - 2;
	smallStep = bigStep / 10.;
	scroll->SetSteps (smallStep, bigStep);

	scroll = ScrollBar (B_VERTICAL);
	scroll->SetRange (0.0, y);
	scroll->SetProportion ((height - y) / height);
	bigStep = frame.Height() - 2;
	smallStep = bigStep / 10.;
	scroll->SetSteps (smallStep, bigStep);
}

// Implementation of PreviewWindow

PreviewWindow::PreviewWindow(BFile* jobFile) 
	: BlockingWindow(BRect(20, 24, 400, 600), "Preview", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS)
{
	const float kButtonDistance = 15;
	const float kPageTextDistance = 10;
	
	float top = 7;
	float left = 20;
	float width, height;
		
	BRect r = Frame();
	r.right = r.IntegerWidth() - B_V_SCROLL_BAR_WIDTH; r.left = 0;
	r.bottom = r.IntegerHeight() - B_H_SCROLL_BAR_HEIGHT; r.top = 0;

		// add navigation and zoom buttons
	// largest button first to get its size
	fPrev = new BButton(BRect(left, top, left+10, top+10), "Prev", "Previous Page", new BMessage(MSG_PREV_PAGE));
	AddChild(fPrev);
	fPrev->ResizeToPreferred();
	width = fPrev->Bounds().Width()+1;
	height = fPrev->Bounds().Height()+1;

	fFirst = new BButton(BRect(left, top, left+10, top+10), "First", "First Page", new BMessage(MSG_FIRST_PAGE));
	AddChild(fFirst);
	fFirst->ResizeTo(width, height);
	left = fFirst->Frame().right + kButtonDistance;

	// move Previous Page button after First Page button
	fPrev->MoveTo(left, top);	
	left = fPrev->Frame().right + kButtonDistance;
	
	fNext = new BButton(BRect(left, top, left+10, top+10), "Next", "Next Page", new BMessage(MSG_NEXT_PAGE));
	AddChild(fNext);
	fNext->ResizeTo(width, height);
	left = fNext->Frame().right + kButtonDistance;
	
	fLast = new BButton(BRect(left, top, left+10, top+10), "Last", "Last Page", new BMessage(MSG_LAST_PAGE));
	AddChild(fLast);
	fLast->ResizeTo(width, height);
	left = fLast->Frame().right + kPageTextDistance;
	
	// page number text
	fPageNumber = new BTextControl(BRect(left, top+3, left+10, top+10), "FindPage", "", "", new BMessage(MSG_FIND_PAGE));
	fPageNumber->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_RIGHT);
	int num;
	for (num = 0; num <= 255; num++) {
		fPageNumber->TextView()->DisallowChar(num);
	}
	for (num = 0; num <= 9; num++) {
		fPageNumber->TextView()->AllowChar('0' + num);
	}
	fPageNumber->TextView()-> SetMaxBytes(5);	
	AddChild(fPageNumber);
	fPageNumber->ResizeTo(be_plain_font->StringWidth("999999") + 10, height);
	left = fPageNumber->Frame().right + 5;
	
	// number of pages text
	fPageText = new BStringView(BRect(left, top, left + 100, top + 15), "pageText", "");
	AddChild(fPageText);
		// estimate max. width
	float pageTextWidth = fPageText->StringWidth("of 99999 Pages");
		// estimate height
	BFont font;
	fPageText->GetFont(&font);
	float pageTextHeight = font.Size() + 2;
		// resize to estimated size
	fPageText->ResizeTo(pageTextWidth, pageTextHeight);
	left += pageTextWidth + kPageTextDistance;
	fPageText->MoveBy(0, (height - font.Size()) / 2);
	
	
	fZoomIn = new BButton(BRect(left, top, left+10, top+10), "ZoomIn", "Zoom In", new BMessage(MSG_ZOOM_IN));
	AddChild(fZoomIn);
	fZoomIn->ResizeTo(width, height);
	left = fZoomIn->Frame().right + kButtonDistance;
	
	fZoomOut = new BButton(BRect(left, top, left+10, top+10), "ZoomOut", "Zoom Out", new BMessage(MSG_ZOOM_OUT));
	AddChild(fZoomOut);
	fZoomOut->ResizeTo(width, height);
	
	fButtonBarHeight = fZoomOut->Frame().bottom + 7;
	
		// add preview view		
	r.top = fButtonBarHeight;
	fPreview = new PreviewView(jobFile, r);
	
	
	fPreviewScroller = new BScrollView("PreviewScroller", fPreview, B_FOLLOW_ALL, 0, true, true, B_FANCY_BORDER);
	fPreviewScroller->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fPreviewScroller);
	
	if (fPreview->InitCheck() == B_OK) {
		ResizeToPage();
		fPreview->FixScrollbars();
		UpdateControls();
		fPreview->MakeFocus(true);
	}
}

void PreviewWindow::ResizeToPage() {
	BScreen screen;
	BRect r(fPreview->ViewRect());
	float width, height;
	float maxWidth, maxHeight, minWidth;
	const float windowBorderWidth = 5;
	const float windowBorderHeight = 5;
	
	if (screen.Frame().right == 0.0) {	
		return; // invalid screen object
	}
	
	width = r.Width() + 1 + B_V_SCROLL_BAR_WIDTH;
	height = r.Height() + 1 + fButtonBarHeight + B_H_SCROLL_BAR_HEIGHT;

	// dimensions so that window does not reach outside of screen	
	maxWidth = screen.Frame().Width() + 1 - windowBorderWidth - Frame().left;
	maxHeight = screen.Frame().Height() + 1 - windowBorderHeight - Frame().top;

	// width so that all buttons are visible
	minWidth = 	fZoomOut->Frame().right + 10;
	
	if (width < minWidth) width = minWidth;

	if (width > maxWidth) width = maxWidth;
	if (height > maxHeight) height = maxHeight;
	
	ResizeTo(width, height);
}

void PreviewWindow::UpdateControls() {
	fFirst->SetEnabled(!fPreview->ShowsFirstPage());
	fPrev->SetEnabled(!fPreview->ShowsFirstPage());
	fNext->SetEnabled(!fPreview->ShowsLastPage());
	fLast->SetEnabled(!fPreview->ShowsLastPage());
	fZoomIn->SetEnabled(fPreview->CanZoomIn());
	fZoomOut->SetEnabled(fPreview->CanZoomOut());
	
	BString page;
	page << fPreview->CurrentPage();
	fPageNumber->SetText(page.String());	

	BString text;  
	text << "of " 
		<< fPreview->NumberOfPages();
	if (fPreview->NumberOfPages() == 1) {
		text << " Page";
	} else {
		text << " Pages";
	}
	fPageText->SetText(text.String());}

void PreviewWindow::MessageReceived(BMessage* m) {
	switch (m->what) {
		case MSG_FIRST_PAGE: 
			fPreview->ShowFirstPage();
			break;
		case MSG_NEXT_PAGE: 
			fPreview->ShowNextPage();
			break;
		case MSG_PREV_PAGE: 
			fPreview->ShowPrevPage();
			break;	
		case MSG_LAST_PAGE: 
			fPreview->ShowLastPage();
			break;
		case MSG_FIND_PAGE: 
			fPreview->ShowFindPage(atoi(fPageNumber->Text())) ;
			break;
		case MSG_ZOOM_IN: 
			fPreview->ZoomIn(); 
			ResizeToPage();
			break;
		case MSG_ZOOM_OUT: 
			fPreview->ZoomOut(); 
			ResizeToPage();
			break;
		case B_MODIFIERS_CHANGED:
			fPreview->MouseMoved(BPoint(), B_INSIDE_VIEW, m);
			break;
		default:
			inherited::MessageReceived(m); return;
	}
	UpdateControls();
}

status_t PreviewWindow::Go() {
	status_t st = InitCheck();
	if (st == B_OK) {
		return inherited::Go();
	} else {
		Quit();
	}
	return st;
}
