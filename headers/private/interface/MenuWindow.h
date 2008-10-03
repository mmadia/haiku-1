/*
 * Copyright 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */
#ifndef __MENUWINDOW_H
#define __MENUWINDOW_H


#include <Window.h>

class BMenu;


namespace BPrivate {

class BMenuFrame;
class BMenuScroller;


class BMenuWindow : public BWindow {
	public:
		BMenuWindow(const char *name);
		virtual ~BMenuWindow();

		virtual void DispatchMessage(BMessage *message, BHandler *handler);
	
		void AttachMenu(BMenu *menu);
		void DetachMenu();
	
		void AttachScrollers();
		void DetachScrollers();

		bool CheckForScrolling(const BPoint &cursor);
		bool TryScrollBy(const float &step);
	
	private:
		BMenu *fMenu;
		BMenuFrame *fMenuFrame;
		BMenuScroller *fUpperScroller;
		BMenuScroller *fLowerScroller;
		
		float fValue;
		float fLimit;
		
		bool _Scroll(const BPoint &cursor);
		void _ScrollBy(const float &step);
};

}	// namespace BPrivate

#endif	// __MENUWINDOW_H
