//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _DIAL_UP_VIEW__H
#define _DIAL_UP_VIEW__H

#include <View.h>
#include <Message.h>

#include <PPPInterfaceListener.h>


class DialUpView : public BView {
	public:
		DialUpView(BRect frame);
		virtual ~DialUpView();
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		
		void UpDownThread();

	private:
		void GetPPPDirectories(BDirectory *settingsDirectory,
			BDirectory *profileDirectory) const;
		
		void HandleReportMessage(BMessage *message);
		void CreateTabs();
		
		void UpdateStatus(int32 code);
		void WatchInterface(ppp_interface_id ID);
		
		bool LoadSettings(bool isNew);
		void IsModified(bool& settings, bool& profile);
		bool SaveSettings(BMessage& settings, BMessage& profile, bool saveTemporary);
		bool SaveSettingsToFile();
		
		void LoadInterfaces();
		void LoadAddons();
		
		void AddInterface(const char *name, bool isNew = false);
		void SelectInterface(int32 index, bool isNew = false);
		int32 CountInterfaces() const;
		int32 FindNextMenuInsertionIndex(BMenu *menu, const BString& name,
			int32 index = 0);

	private:
		PPPInterfaceListener fListener;
		
		thread_id fUpDownThread;
		
		BMessage fAddons, fSettings, fProfile;
		driver_settings *fDriverSettings;
		BMenuItem *fCurrentItem;
		ppp_interface_id fWatching;
		
		bool fKeepLabel;
		BStringView *fStatusView;
		BButton *fConnectButton;
		BPopUpMenu *fInterfaceMenu;
		BMenuField *fMenuField;
		BTabView *fTabView;
};


#endif
