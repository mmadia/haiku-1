//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		RootLayer.h
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//					DarkWyrm <bpmagic@columbus.rr.com>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Class used for the top layer of each workspace's Layer tree
//  
//------------------------------------------------------------------------------
#ifndef _ROOTLAYER_H_
#define _ROOTLAYER_H_

#include <List.h>
#include <Locker.h>

#include "DebugInfoManager.h"
#include "Layer.h"
#include "FMWList.h"
#include "CursorManager.h"
#include "Workspace.h"

class RGBColor;
class Screen;
class WinBorder;
class Desktop;
class DisplayDriver;
class BPortLink;

#ifndef DISPLAY_HAIKU_LOGO
#define DISPLAY_HAIKU_LOGO 1
#endif

#if DISPLAY_HAIKU_LOGO
class UtilityBitmap;
#endif

/*!
	\class RootLayer RootLayer.h
	\brief Class used for the top layer of each workspace's Layer tree
	
	RootLayers are used to head up the top of each Layer tree and reimplement certain 
	Layer functions to act accordingly. There is only one for each workspace class.
	
*/
class RootLayer : public Layer {
public:
								RootLayer(const char *name,	int32 workspaceCount,
										Desktop *desktop, DisplayDriver *driver);
	virtual						~RootLayer(void);
	
	virtual	void				MoveBy(float x, float y);
	virtual	void				ResizeBy(float x, float y);
	
	// For the active workspaces
	virtual	Layer*				VirtualTopChild(void) const;
	virtual	Layer*				VirtualLowerSibling(void) const;
	virtual	Layer*				VirtualUpperSibling(void) const;
	virtual	Layer*				VirtualBottomChild(void) const;

			void				HideWinBorder(WinBorder* winBorder);
			void				ShowWinBorder(WinBorder* winBorder);
			void				SetWinBorderWorskpaces(WinBorder *winBorder,
										uint32 oldWksIndex,
										uint32 newWksIndex);
			WinBorder*			WinBorderAt(const BPoint& pt) const;
	inline	WinBorder*			FocusWinBorder() const { return fWorkspace[fActiveWksIndex]->Focus(); }
	inline	WinBorder*			FrontWinBorder() const { return fWorkspace[fActiveWksIndex]->Front(); }
	inline	WinBorder*			ActiveWinBorder() const {
									return (fWorkspace[fActiveWksIndex]->Focus() == 
												fWorkspace[fActiveWksIndex]->Front()
											&& fWorkspace[fActiveWksIndex]->Front() != NULL)?
									fWorkspace[fActiveWksIndex]->Front(): NULL;
								}

	inline	void				SetWorkspaceCount(int32 wksCount);
	inline	int32				WorkspaceCount() const { return fWsCount; }
	inline	Workspace*			WorkspaceAt(int32 index) const { return fWorkspace[index]; }
	inline	Workspace*			ActiveWorkspace() const { return fWorkspace[fActiveWksIndex]; }
	inline	int32				ActiveWorkspaceIndex() const { return fActiveWksIndex; }
			bool				SetActiveWorkspace(int32 index);

			void				ReadWorkspaceData(const char *path);
			void				SaveWorkspaceData(const char *path);
	
			void				SetScreens(Screen *screen[], int32 rows, int32 columns);
			Screen**			Screens(void);
			bool				SetScreenMode(int32 width, int32 height, uint32 colorspace, float frequency);
			int32				ScreenRows(void) const { return fRows; }
			int32				ScreenColumns(void) const { return fColumns; }

			void				SetBGColor(const RGBColor &col);
			RGBColor			BGColor(void) const;
	
	inline	int32				Buttons(void) { return fButtons; }
	virtual bool				HasClient(void) { return false; }
	
			void				SetDragMessage(BMessage *msg);
			BMessage*			DragMessage(void) const;

			bool				SetEventMaskLayer(Layer *lay, uint32 mask, uint32 options);

	static	int32				WorkingThread(void *data);

			CursorManager&		GetCursorManager() { return fCursorManager; }

			// Other methods
			bool				Lock() { return fAllRegionsLock.Lock(); }
			void				Unlock() { fAllRegionsLock.Unlock(); }
			bool				IsLocked() { return fAllRegionsLock.IsLocked(); }
			void				RunThread();
			status_t			EnqueueMessage(BPortLink &message);
			void				GoInvalidate(const Layer *layer, const BRegion &region);
			void				GoRedraw(const Layer *layer, const BRegion &region);
			void				GoChangeWinBorderFeel(const WinBorder *winBorder, int32 newFeel);

	virtual	void				Draw(const BRect &r);

			// Debug methods
			void				PrintToStream(void);
			thread_id			LockingThread() { return fAllRegionsLock.LockingThread(); }
	
			BRegion				fRedrawReg;
			BList				fCopyRegList;
			BList				fCopyList;

private:
friend class Desktop;

			// these are meant for Desktop class only!
			void				AddWinBorder(WinBorder* winBorder);
			void				RemoveWinBorder(WinBorder* winBorder);
			void				AddSubsetWinBorder(WinBorder *winBorder, WinBorder *toWinBorder);
			void				RemoveSubsetWinBorder(WinBorder *winBorder, WinBorder *fromWinBorder);

			void				show_winBorder(WinBorder* winBorder);
			void				hide_winBorder(WinBorder* winBorder);

			void				change_winBorder_feel(WinBorder *winBorder, int32 newFeel);

			bool				get_workspace_windows();
			void 				draw_window_tab(WinBorder *exFocus);
			void				empty_visible_regions(Layer *layer);

			void				invalidate_layer(Layer *layer, const BRegion &region);
			void				redraw_layer(Layer *layer, const BRegion &region);

			void				winborder_activation(WinBorder* exActive);

			void				show_final_scene(WinBorder *exFocus, WinBorder *exActive);

			// Input related methods
			void				MouseEventHandler(int32 code, BPortLink& link);
			void				KeyboardEventHandler(int32 code, BPortLink& link);

			Desktop*			fDesktop;
			BMessage*			fDragMessage;
			Layer*				fLastMouseMoved;
			WinBorder*			fMouseTargetWinBorder;
			int32				fViewAction;
			Layer*				fEventMaskLayer;

			CursorManager		fCursorManager;

			BLocker				fAllRegionsLock;

			thread_id			fThreadID;
			port_id				fListenPort;

			BList				fScreenPtrList;
			int32				fRows;
			int32				fColumns;

			int32				fScreenWidth;
			int32				fScreenHeight;
			uint32				fColorSpace;
			float				fFrequency;

			int32				fButtons;
			BPoint				fLastMousePosition;
			bool				fMovingWindow;
			bool				fResizingWindow;
			bool				fHaveWinBorderList;
	
			int32				fActiveWksIndex;
			int32				fWsCount;
			Workspace**			fWorkspace;

			int32				fWinBorderListLength;
			WinBorder**			fWinBorderList2;
			WinBorder**			fWinBorderList;
			int32				fWinBorderCount;
	mutable int32				fWinBorderIndex;

			int32				fScreenShotIndex;
			bool				fQuiting;

#if ON_SCREEN_DEBUGGING_INFO
	friend	class DebugInfoManager;
			void				AddDebugInfo(const char* string);
			BString				fDebugInfo;
#endif
#if DISPLAY_HAIKU_LOGO
			UtilityBitmap*		fLogoBitmap;
#endif
};

#endif
