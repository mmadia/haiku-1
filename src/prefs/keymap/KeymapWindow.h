// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        KeymapWindow.h
//  Author:      Sandor Vroemisse, Jérôme Duval
//  Description: Keymap Preferences
//  Created :    July 12, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef KEYMAP_WINDOW_H
#define KEYMAP_WINDOW_H

#include <Window.h>
#include <MenuBar.h>
#include "KeymapTextView.h"
#include "Keymap.h"

#define WINDOW_TITLE				"Keymap"
#define WINDOW_LEFT_TOP_POSITION	BPoint( 80, 25 )
#define WINDOW_DIMENSIONS			BRect( 0,0, 612,256 )

class KeymapListItem;
class KeymapApplication;

class MapView : public BView
{
public:
	MapView(BRect rect, const char *name, Keymap *keymap);
	void Draw(BRect rect);
	void DrawKey(int32 keyCode);
	void DrawBorder(BRect borderRect);
	void AttachedToWindow();
	void KeyDown(const char* bytes, int32 numBytes);
	void KeyUp(const char* bytes, int32 numBytes);
	void MessageReceived(BMessage *msg);
	void SetFontFamily(const font_family family);
	void MouseDown(BPoint point);
	void MouseUp(BPoint point);
	void MouseMoved(BPoint point, uint32 transit, const BMessage *msg);
private:	
	key_info fOldKeyInfo;
	BRect fKeysRect[128];
	bool fKeysVertical[128];
	uint8 fKeyState[16];
	BFont fCurrentFont;
	
	Keymap				*fCurrentMap;
	KeymapTextView		*fTextView;
};


class KeymapWindow : public BWindow {
public:
			KeymapWindow( BRect frame );
	bool	QuitRequested();
	void	MessageReceived( BMessage* message );

protected:
	BListView			*fSystemListView;
	BListView			*fUserListView;
	// the map that's currently highlighted
	BButton				*fUseButton;
	BButton				*fRevertButton;
	BMenu				*fFontMenu;
	
	MapView				*fMapView;

	BMenuBar			*AddMenuBar();
	void				AddMaps(BView *placeholderView);
	void				UseKeymap();
	
	void FillSystemMaps();
	void FillUserMaps();
	
	BEntry* 			CurrentMap();
		
	Keymap				fCurrentMap;
};

#endif // KEYMAP_WINDOW_H
