/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHARACTER_WINDOW_H
#define CHARACTER_WINDOW_H


#include <Messenger.h>
#include <Window.h>

class BFile;
class BListView;
class BMenu;
class BMenuItem;
class BSlider;
class BStringView;
class CharacterView;


class CharacterWindow : public BWindow {
public:
							CharacterWindow();
	virtual					~CharacterWindow();

	virtual void			MessageReceived(BMessage* message);
	virtual bool			QuitRequested();

private:
			status_t		_OpenSettings(BFile& file, uint32 mode);
			status_t		_LoadSettings(BMessage& settings);
			status_t		_SaveSettings();

			void			_SetFont(const char* family, const char* style);
			BMenu*			_CreateFontMenu();
			void			_UpdateUnicodeBlocks();
			void			_CreateUnicodeBlocks();

private:
			BListView*		fUnicodeBlockView;
			CharacterView*	fCharacterView;
			BMenuItem*		fSelectedFontItem;
			BSlider*		fFontSizeSlider;
			BStringView*	fCodeView;
};

#endif	// CHARACTER_WINDOW_H