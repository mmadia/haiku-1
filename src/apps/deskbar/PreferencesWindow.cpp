/*
 * Copyright 2009 Haiku, Inc.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "PreferencesWindow.h"

#include <CheckBox.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <OpenWithTracker.h>
#include <RadioButton.h>
#include <SeparatorView.h>

#include <ctype.h>

#include "BarApp.h"
#include "StatusView.h"


PreferencesWindow::PreferencesWindow(BRect frame)
	:
	BWindow(frame, "Deskbar Preferences", B_TITLED_WINDOW, B_NOT_RESIZABLE
		| B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE)
{
	// Controls
	fMenuRecentDocuments = new BCheckBox("Recent Documents:",
		new BMessage(kUpdateRecentCounts));
	fMenuRecentApplications = new BCheckBox("Recent Applications:",
		new BMessage(kUpdateRecentCounts));
	fMenuRecentFolders = new BCheckBox("Recent Folders:",
		new BMessage(kUpdateRecentCounts));

	fMenuRecentDocumentCount = new BTextControl(NULL, NULL,
		new BMessage(kUpdateRecentCounts));
	fMenuRecentApplicationCount = new BTextControl(NULL, NULL,
		new BMessage(kUpdateRecentCounts));
	fMenuRecentFolderCount = new BTextControl(NULL, NULL,
		new BMessage(kUpdateRecentCounts));

	fAppsSort = new BCheckBox("Sort Running Applications",
		new BMessage(kSortRunningApps));
	fAppsSortTrackerFirst = new BCheckBox("Tracker Always First",
		new BMessage(kTrackerFirst));
	fAppsShowExpanders = new BCheckBox("Show Application Expander",
		new BMessage(kSuperExpando));
	fAppsExpandNew = new BCheckBox("Expand New Applications",
		new BMessage(kExpandNewTeams));

	fClock24Hours = new BCheckBox("24 Hour Clock", new BMessage(kMsgMilTime));
	fClockSeconds = new BCheckBox("Show Seconds",
		new BMessage(kMsgShowSeconds));
	fClockEuropeanDate = new BCheckBox("European Date",
		new BMessage(kMsgEuroDate));
	fClockFullDate = new BCheckBox("Full Date", new BMessage(kMsgFullDate));

	fWindowAlwaysOnTop = new BCheckBox("Always On Top",
		new BMessage(kAlwaysTop));
	fWindowAutoRaise = new BCheckBox("Auto-raise", new BMessage(kAutoRaise));

	BTextView* docTextView = fMenuRecentDocumentCount->TextView();
	BTextView* appTextView = fMenuRecentApplicationCount->TextView();
	BTextView* folderTextView = fMenuRecentFolderCount->TextView();

	for (int32 i = 0; i < 256; i++) {
		if (!isdigit(i)) {
			docTextView->DisallowChar(i);
			appTextView->DisallowChar(i);
			folderTextView->DisallowChar(i);
		}
	}

	docTextView->SetMaxBytes(4);
	appTextView->SetMaxBytes(4);
	folderTextView->SetMaxBytes(4);

	// Values
	TBarApp* barApp = static_cast<TBarApp*>(be_app);
	desk_settings* appSettings = barApp->Settings();;

	fAppsSort->SetValue(appSettings->sortRunningApps);
	fAppsSortTrackerFirst->SetValue(appSettings->trackerAlwaysFirst);
	fAppsShowExpanders->SetValue(appSettings->superExpando);
	fAppsExpandNew->SetValue(appSettings->expandNewTeams);

	fMenuRecentDocuments->SetValue(false);
	fMenuRecentApplications->SetValue(false);
	fMenuRecentFolders->SetValue(false);

	fMenuRecentDocumentCount->SetEnabled(false);
	fMenuRecentApplicationCount->SetEnabled(false);
	fMenuRecentFolderCount->SetEnabled(false);

	int32 docCount = appSettings->recentDocsCount;
	int32 appCount = appSettings->recentAppsCount;
	int32 folderCount = appSettings->recentFoldersCount;

	if (docCount > 0) {
		fMenuRecentDocuments->SetValue(true);
		fMenuRecentDocumentCount->SetEnabled(true);
	}

	if (appCount > 0) {
		fMenuRecentApplications->SetValue(true);
		fMenuRecentApplicationCount->SetEnabled(true);
	}

	if (folderCount > 0) {
		fMenuRecentFolders->SetValue(true);
		fMenuRecentFolderCount->SetEnabled(true);
	}

	BString docString;
	BString appString;
	BString folderString;

	docString << docCount;
	appString << appCount;
	folderString << folderCount;

	fMenuRecentDocumentCount->SetText(docString.String());
	fMenuRecentApplicationCount->SetText(appString.String());
	fMenuRecentFolderCount->SetText(folderString.String());

	TReplicantTray* replicantTray = barApp->BarView()->fReplicantTray;

	fClock24Hours->SetValue(replicantTray->ShowingMiltime());
	fClockSeconds->SetValue(replicantTray->ShowingSeconds());
	fClockEuropeanDate->SetValue(replicantTray->ShowingEuroDate());
	fClockFullDate->SetValue(replicantTray->ShowingFullDate());

	bool showingClock = barApp->BarView()->ShowingClock();
	fClock24Hours->SetEnabled(showingClock);
	fClockSeconds->SetEnabled(showingClock);
	fClockEuropeanDate->SetEnabled(showingClock);
	fClockFullDate->SetEnabled(replicantTray->CanShowFullDate());

	fWindowAlwaysOnTop->SetValue(appSettings->alwaysOnTop);
	fWindowAutoRaise->SetValue(appSettings->autoRaise);

	_EnableDisableDependentItems();

	// Targets
	fAppsSort->SetTarget(be_app);
	fAppsSortTrackerFirst->SetTarget(be_app);
	fAppsExpandNew->SetTarget(be_app);

	fClock24Hours->SetTarget(replicantTray);
	fClockSeconds->SetTarget(replicantTray);
	fClockEuropeanDate->SetTarget(replicantTray);
	fClockFullDate->SetTarget(replicantTray);

	fWindowAlwaysOnTop->SetTarget(be_app);
	fWindowAutoRaise->SetTarget(be_app);

	// Layout
	fMenuBox = new BBox("fMenuBox");
	fAppsBox = new BBox("fAppsBox");
	fClockBox = new BBox("fClockBox");
	fWindowBox = new BBox("fWindowBox");

	BStringView* menuString = new BStringView(NULL, "Menu");
	BStringView* appsString = new BStringView(NULL, "Applications");
	BStringView* clockString = new BStringView(NULL, "Clock");
	BStringView* windowString = new BStringView(NULL, "Window");

	BFont font;
	menuString->GetFont(&font);
	font.SetFace(B_BOLD_FACE);

	menuString->SetFont(&font, B_FONT_FACE);
	appsString->SetFont(&font, B_FONT_FACE);
	clockString->SetFont(&font, B_FONT_FACE);
	windowString->SetFont(&font, B_FONT_FACE);

	fMenuBox->SetLabel(menuString);
	fAppsBox->SetLabel(appsString);
	fClockBox->SetLabel(clockString);
	fWindowBox->SetLabel(windowString);

	BView* view;
	view = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 10)
			.AddGroup(B_HORIZONTAL, 10)
				.AddGroup(B_VERTICAL, 10)
					.Add(fMenuRecentDocuments)
					.Add(fMenuRecentFolders)
					.Add(fMenuRecentApplications)
					.End()
				.AddGroup(B_VERTICAL, 10)
					.Add(fMenuRecentDocumentCount)
					.Add(fMenuRecentApplicationCount)
					.Add(fMenuRecentFolderCount)
					.End()
				.End()
			.Add(new BButton("Edit Menu" B_UTF8_ELLIPSIS,
				new BMessage(kEditMenuInTracker)))
			.SetInsets(14, 14, 14, 14)
			.End()
		.View();
	fMenuBox->AddChild(view);

	view = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 10)
			.Add(fAppsSort)
			.Add(fAppsSortTrackerFirst)
			.Add(fAppsShowExpanders)
			.AddGroup(B_HORIZONTAL, 0)
				.SetInsets(20, 0, 0, 0)
				.Add(fAppsExpandNew)
				.End()
			.SetInsets(14, 14, 14, 14)
			.End()
		.View();
	fAppsBox->AddChild(view);

	view = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 10)
			.Add(fClock24Hours)
			.Add(fClockSeconds)
			.Add(fClockEuropeanDate)
			.Add(fClockFullDate)
			.SetInsets(14, 14, 14, 14)
			.End()
		.View();
	fClockBox->AddChild(view);

	view = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 10)
			.Add(fWindowAlwaysOnTop)
			.Add(fWindowAutoRaise)
			.SetInsets(14, 14, 14, 14)
			.End()
		.View();
	fWindowBox->AddChild(view);

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, 10)
			.Add(fMenuBox)
			.Add(fWindowBox)
			.Add(fAppsBox)
			.Add(fClockBox)
			.SetInsets(14, 14, 14, 14)
			.End()
		.End();

	CenterOnScreen();
}


PreferencesWindow::~PreferencesWindow()	
{
	_UpdateRecentCounts();
	be_app->PostMessage(kConfigClose);
}


void 
PreferencesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		
		case kEditMenuInTracker:
			OpenWithTracker(B_USER_DESKBAR_DIRECTORY);
			break;

		case kUpdateRecentCounts:
			_UpdateRecentCounts();
			break;

		case kSuperExpando:
			_EnableDisableDependentItems();
			be_app->PostMessage(message);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void 
PreferencesWindow::_UpdateRecentCounts()
{
	BMessage message(kUpdateRecentCounts);

	int32 docCount = atoi(fMenuRecentDocumentCount->Text());
	int32 appCount = atoi(fMenuRecentApplicationCount->Text());
	int32 folderCount = atoi(fMenuRecentFolderCount->Text());

	if (docCount <= 0 || fMenuRecentDocuments->Value() == false)
		message.AddInt32("documents", 0);
	else
		message.AddInt32("documents", docCount);

	if (appCount <= 0 || fMenuRecentApplications->Value() == false)
		message.AddInt32("applications", 0);
	else
		message.AddInt32("applications", appCount);

	if (folderCount <= 0 || fMenuRecentFolders->Value() == false)
		message.AddInt32("folders", 0);
	else
		message.AddInt32("folders", folderCount);

	be_app->PostMessage(&message);

	_EnableDisableDependentItems();
}


void 
PreferencesWindow::_EnableDisableDependentItems()
{
	if (fAppsShowExpanders->Value())
		fAppsExpandNew->SetEnabled(true);
	else
		fAppsExpandNew->SetEnabled(false);

	if (fMenuRecentDocuments->Value())
		fMenuRecentDocumentCount->SetEnabled(true);
	else
		fMenuRecentDocumentCount->SetEnabled(false);

	if (fMenuRecentApplications->Value())
		fMenuRecentApplicationCount->SetEnabled(true);
	else
		fMenuRecentApplicationCount->SetEnabled(false);

	if (fMenuRecentFolders->Value())
		fMenuRecentFolderCount->SetEnabled(true);
	else
		fMenuRecentFolderCount->SetEnabled(false);
}


void 
PreferencesWindow::WindowActivated(bool active)
{
	if (!active && IsMinimized())
		PostMessage(B_QUIT_REQUESTED);
}

