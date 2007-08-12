/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#ifndef _PICTURE_TEST_WINDOW_H
#define _PICTURE_TEST_WINDOW_H

#include <Window.h>

class PictureTestWindow : public BWindow
{
	typedef BWindow Inherited;

public:
	PictureTestWindow();
	void MessageReceived(BMessage *msg);
	bool QuitRequested();

private:

	enum {
		kMsgRunTests = 'PTst',
		kMsgWriteImages,
	};

	void BuildGUI();
	void UpdateHeader();
	void RunTests();
	void RunTests(int32 testIndex);
	void RunTests(int32 testIndex, color_space colorSpace);
	
	BListView *fListView;
	BStringView *fHeader;
	
	int32 fFailedTests;
	int32 fNumberOfTests;
};

#endif
