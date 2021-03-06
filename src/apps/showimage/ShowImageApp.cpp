/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 *		Ryan Leavengood
 */

#include "ShowImageApp.h"

#include <stdio.h>

#include <AboutWindow.h>
#include <Alert.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <FilePanel.h>
#include <Locale.h>
#include <Path.h>
#include <String.h>

#include "ShowImageConstants.h"
#include "ShowImageWindow.h"


#define WINDOWS_TO_IGNORE 1

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "AboutWindow"

extern const char* kApplicationSignature = "application/x-vnd.Haiku-ShowImage";


ShowImageApp::ShowImageApp()
	:
	BApplication(kApplicationSignature)
{
	fPulseStarted = false;
	fOpenPanel = new BFilePanel(B_OPEN_PANEL);
}


ShowImageApp::~ShowImageApp()
{
}


void
ShowImageApp::AboutRequested()
{
	const char* authors[] = {
		"Fernando F. Oliveira",
		"Michael Wilber",
		"Michael Pfeiffer",
		"Ryan Leavengood",
		NULL
	};
	BAboutWindow about(B_TRANSLATE("ShowImage"), 2003, authors);
	about.Show();
}


void
ShowImageApp::ReadyToRun()
{
	if (CountWindows() == WINDOWS_TO_IGNORE)
		fOpenPanel->Show();
	else {
		// If image windows are already open
		// (paths supplied on the command line)
		// start checking the number of open windows
		StartPulse();
	}

	be_clipboard->StartWatching(be_app_messenger);
		// tell the clipboard to notify this app when its contents change
}


void
ShowImageApp::StartPulse()
{
	if (!fPulseStarted) {
		// Tell the app to begin checking
		// for the number of open windows
		fPulseStarted = true;
		SetPulseRate(250000);
			// Set pulse to every 1/4 second
	}
}


void
ShowImageApp::Pulse()
{
	// Bug: The BFilePanel is automatically closed if the volume that
	// is displayed is unmounted.
	if (!IsLaunching() && CountWindows() <= WINDOWS_TO_IGNORE) {
		// If the application is not launching and
		// all windows are closed except for the file open panel,
		// quit the application
		PostMessage(B_QUIT_REQUESTED);
	}
}


void
ShowImageApp::ArgvReceived(int32 argc, char **argv)
{
	BMessage message;
	bool hasRefs = false;

	// get current working directory
	const char* cwd;
	if (CurrentMessage() == NULL
		|| CurrentMessage()->FindString("cwd", &cwd) != B_OK)
		cwd = "";

	for (int32 i = 1; i < argc; i++) {
		BPath path;
		if (argv[i][0] == '/') {
			// absolute path
			path.SetTo(argv[i]);
		} else {
			// relative path
			path.SetTo(cwd);
			path.Append(argv[i]);
		}

		entry_ref ref;
		status_t err = get_ref_for_path(path.Path(), &ref);
		if (err == B_OK) {
			message.AddRef("refs", &ref);
			hasRefs = true;
		}
	}

	if (hasRefs)
		RefsReceived(&message);
}


void
ShowImageApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_FILE_OPEN:
			fOpenPanel->Show();
			break;

		case MSG_WINDOW_QUIT:
			break;

		case B_CANCEL:
			// File open panel was closed,
			// start checking count of open windows
			StartPulse();
			break;

		case B_CLIPBOARD_CHANGED:
			CheckClipboard();
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
ShowImageApp::RefsReceived(BMessage* message)
{
	// If a tracker window opened me, get a messenger from it.
	if (message->HasMessenger("TrackerViewToken"))
		message->FindMessenger("TrackerViewToken", &fTrackerMessenger);

	uint32 type;
	int32 count;
	status_t ret = message->GetInfo("refs", &type, &count);
	if (ret != B_OK || type != B_REF_TYPE)
		return;

	entry_ref ref;
	for (int32 i = 0; i < count; i++) {
		if (message->FindRef("refs", i, &ref) == B_OK)
			Open(&ref);
	}
}


void
ShowImageApp::Open(const entry_ref* ref)
{
	new ShowImageWindow(ref, fTrackerMessenger);
}


void
ShowImageApp::BroadcastToWindows(BMessage* message)
{
	const int32 count = CountWindows();
	for (int32 i = 0; i < count; i ++) {
		// BMessenger checks for us if BWindow is still a valid object
		BMessenger msgr(WindowAt(i));
		msgr.SendMessage(message);
	}
}


void
ShowImageApp::CheckClipboard()
{
	// Determines if the contents of the clipboard contain
	// data that is useful to this application.
	// After checking the clipboard, a message is sent to
	// all windows indicating that the clipboard has changed
	// and whether or not the clipboard contains useful data.
	bool dataAvailable = false;

	if (be_clipboard->Lock()) {
		BMessage* clip = be_clipboard->Data();
		if (clip != NULL) {
			dataAvailable = clip->HasMessage("image/bitmap")
				|| clip->HasMessage("image/x-be-bitmap");
		}

		be_clipboard->Unlock();
	}

	BMessage msg(MSG_CLIPBOARD_CHANGED);
	msg.AddBool("data_available", dataAvailable);
	BroadcastToWindows(&msg);
}


bool
ShowImageApp::QuitRequested()
{
	// Give the windows a chance to prompt the user if there are changes
	bool result = BApplication::QuitRequested();
	if (result)
		be_clipboard->StopWatching(be_app_messenger);
			// tell clipboard we don't want anymore notification

	return result;
}


//	#pragma mark -


int
main(int, char **)
{
	ShowImageApp theApp;
	theApp.Run();
	return 0;
}

