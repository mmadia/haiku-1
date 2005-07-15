/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "VirtualScreen.h"

#include <DisplayDriver.h>
#include <HWInterface.h>
#include <Desktop.h>

#include <new>


VirtualScreen::VirtualScreen()
	:
	fScreenList(4, true),
	fDisplayDriver(NULL),
	fHWInterface(NULL)
{
}


VirtualScreen::~VirtualScreen()
{
}


status_t
VirtualScreen::RestoreConfiguration(Desktop& desktop)
{
	// Copy current Desktop settings
	//fSettings = desktop.Settings();

	ScreenList list;
	status_t status = gScreenManager->AcquireScreens(&desktop, NULL, 0, false, list);
	if (status < B_OK) {
		// TODO: we would try again here with force == true
		return status;
	}

	for (int32 i = 0; i < list.CountItems(); i++) {
		Screen* screen = list.ItemAt(i);

		AddScreen(screen);
	}

	return B_OK;
}


status_t
VirtualScreen::StoreConfiguration(BMessage& settings)
{
	// TODO: implement me
	return B_OK;
}


status_t
VirtualScreen::AddScreen(Screen* screen)
{
	screen_item* item = new(nothrow) screen_item;
	if (item == NULL)
		return B_NO_MEMORY;

	item->screen = screen;

	BMessage settings;
	if (_FindConfiguration(screen, settings) == B_OK) {
		// TODO: read from settings!
	} else {
		// TODO: more intelligent standard mode (monitor preference, desktop default, ...)
		screen->SetMode(800, 600, B_RGB32, 60.f);
	}

	// TODO: this works only for single screen configurations
	fDisplayDriver = screen->GetDisplayDriver();
	fHWInterface = screen->GetHWInterface();
	fFrame = screen->Frame();

	fScreenList.AddItem(item);

	return B_OK;
}


status_t
VirtualScreen::RemoveScreen(Screen* screen)
{
	// not implemented yet (config changes when running)
	return B_ERROR;
}


/*!
	Returns the smallest frame that spans over all screens
*/
BRect
VirtualScreen::Frame() const
{
	return fFrame;
}


Screen*
VirtualScreen::ScreenAt(int32 index) const
{
	screen_item* item = fScreenList.ItemAt(index);
	if (item != NULL)
		return item->screen;

	return NULL;
}


BRect
VirtualScreen::ScreenFrameAt(int32 index) const
{
	screen_item* item = fScreenList.ItemAt(index);
	if (item != NULL)
		return item->frame;

	return BRect(0, 0, 0, 0);
}


int32
VirtualScreen::CountScreens() const
{
	return fScreenList.CountItems();
}


status_t
VirtualScreen::_FindConfiguration(Screen* screen, BMessage& settings)
{
	// TODO: we probably want to identify the resolution by connected monitor,
	//		and not the display driver used...
	return B_ERROR;
}

