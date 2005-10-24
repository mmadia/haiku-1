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
#include "Desktop.h"
#include "Layer.h"
#include "Workspace.h"

class DisplayDriver;
class HWInterface;
class RGBColor;
class Screen;
class WinBorder;

namespace BPrivate {
	class PortLink;
};

#ifndef DISPLAY_HAIKU_LOGO
#	define DISPLAY_HAIKU_LOGO 1
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
	virtual	void				ScrollBy(float x, float y)
								{ // not allowed
								}

	// For the active workspaces
	virtual	Layer*				FirstChild() const;
	virtual	Layer*				NextChild() const;
	virtual	Layer*				PreviousChild() const;
	virtual	Layer*				LastChild() const;

			void				HideWinBorder(WinBorder* winBorder);
			void				ShowWinBorder(WinBorder* winBorder);
			void				SetWinBorderWorskpaces(WinBorder *winBorder,
										uint32 oldIndex,
										uint32 newIndex);

			void				RevealNewWMState(Workspace::State &oldWMState);
// TODO: we need to replace Winborder* with Layer*
	inline	WinBorder*			Focus() const { return fWMState.Focus; }
	inline	WinBorder*			Front() const { return fWMState.Front; }
	inline	WinBorder*			Active() const { return fWMState.Active; }
			bool				SetActive(WinBorder* newActive);

	inline	void				SetWorkspaceCount(int32 wksCount);
	inline	int32				WorkspaceCount() const { return fWsCount; }
	inline	Workspace*			WorkspaceAt(int32 index) const { return fWorkspace[index]; }
	inline	Workspace*			ActiveWorkspace() const { return fWorkspace[fActiveWksIndex]; }
	inline	int32				ActiveWorkspaceIndex() const { return fActiveWksIndex; }
			bool				SetActiveWorkspace(int32 index);

			void				ReadWorkspaceData(const char *path);
			void				SaveWorkspaceData(const char *path);

			void				SetWorkspacesLayer(Layer* layer) { fWorkspacesLayer = layer; }
			Layer*				WorkspacesLayer() const { return fWorkspacesLayer; }
	
			void				SetBGColor(const RGBColor &col);
			RGBColor			BGColor(void) const;
	
	inline	int32				Buttons(void) { return fButtons; }
	virtual bool				HasClient(void) { return false; }
	
			void				SetDragMessage(BMessage *msg);
			BMessage*			DragMessage(void) const;

			bool				AddToInputNotificationLists(Layer *lay, uint32 mask, uint32 options);
			bool				SetNotifyLayer(Layer *lay, uint32 mask, uint32 options);
			void				ClearNotifyLayer();

			void				LayerRemoved(Layer* layer);

	static	int32				WorkingThread(void *data);

			// Other methods
			bool				Lock() { return fAllRegionsLock.Lock(); }
			void				Unlock() { fAllRegionsLock.Unlock(); }
			bool				IsLocked() { return fAllRegionsLock.IsLocked(); }
			void				RunThread();
			status_t			EnqueueMessage(BPrivate::PortLink &message);
			void				GoInvalidate(Layer *layer, const BRegion &region);
			void				GoRedraw(Layer *layer, const BRegion &region);
			void				GoChangeWinBorderFeel(WinBorder *winBorder, int32 newFeel);

	virtual	void				Draw(const BRect &r);

			thread_id			LockingThread() { return fAllRegionsLock.LockingThread(); }
	
			BRegion				fRedrawReg;
			BList				fCopyRegList;
			BList				fCopyList;

private:
friend class Desktop;
friend class WinBorder; // temporarily, I need invalidate_layer()

			// these are meant for Desktop class only!
			void				AddWinBorder(WinBorder* winBorder);
			void				RemoveWinBorder(WinBorder* winBorder);
			void				AddSubsetWinBorder(WinBorder *winBorder, WinBorder *toWinBorder);
			void				RemoveSubsetWinBorder(WinBorder *winBorder, WinBorder *fromWinBorder);

			void				show_winBorder(WinBorder* winBorder);
			void				hide_winBorder(WinBorder* winBorder);

			void				change_winBorder_feel(WinBorder *winBorder, int32 newFeel);

#ifndef NEW_CLIPPING
			void				empty_visible_regions(Layer *layer);
#endif
			// Input related methods
			void				MouseEventHandler(int32 code, BPrivate::PortLink& link);
			void				KeyboardEventHandler(int32 code, BPrivate::PortLink& link);

			void				_ProcessMouseMovedEvent(PointerEvent &evt);

	inline	HWInterface*		GetHWInterface() const
									{ return fDesktop->GetHWInterface(); }

			Desktop*			fDesktop;
			BMessage*			fDragMessage;
			Layer*				fLastLayerUnderMouse;

			Layer*				fNotifyLayer;
			uint32				fSavedEventMask;
			uint32				fSavedEventOptions;
			BList				fMouseNotificationList;
			BList				fKeyboardNotificationList;

			BLocker				fAllRegionsLock;

			thread_id			fThreadID;
			port_id				fListenPort;

			int32				fButtons;
			BPoint				fLastMousePosition;
	
			int32				fActiveWksIndex;
			int32				fWsCount;
			Workspace**			fWorkspace;
			Layer*				fWorkspacesLayer;

// TODO: fWMState MUST be associated with a surface. This is the case now
//   with RootLayer, but after Axel's refractoring this should go in
//   WorkspaceLayer, I think.
			Workspace::State	fWMState;
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
