//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		Application.cpp
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	BApplication class is the center of the application
//					universe.  The global be_app and be_app_messenger 
//					variables are defined here as well.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <new>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// System Includes -------------------------------------------------------------
#include <AppFileInfo.h>
#include <Application.h>
#include <Cursor.h>
#include <Entry.h>
#include <File.h>
#include <Locker.h>
#include <Path.h>
#include <RegistrarDefs.h>
#include <Roster.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
BApplication*	be_app = NULL;
BMessenger		be_app_messenger;

BResources*	BApplication::_app_resources = NULL;
BLocker		BApplication::_app_resources_lock("_app_resources_lock");

// argc/argv
extern const int __libc_argc;
extern const char * const *__libc_argv;

//------------------------------------------------------------------------------

// debugging
//#define DBG(x) x
#define DBG(x)
#define OUT	printf

enum {
	NOT_IMPLEMENTED	= B_ERROR,
};

// prototypes of helper functions
static const char* looper_name_for(const char *signature);
static void assert_app_signature(const char *signature);
static status_t get_app_path(char *buffer);
static thread_id main_thread_for(team_id team);

//------------------------------------------------------------------------------
BApplication::BApplication(const char* signature)
			: BLooper(looper_name_for(signature)),
			  fAppName(NULL),
			  fServerFrom(-1),
			  fServerTo(-1),
			  fServerHeap(NULL),
			  fPulseRate(500000),
			  fInitialWorkspace(0),
			  fDraggedMessage(NULL),
			  fPulseRunner(NULL),
			  fInitError(B_NO_INIT),
			  fReadyToRunCalled(false)
{
	InitData(signature, NULL);
}
//------------------------------------------------------------------------------
BApplication::BApplication(const char* signature, status_t* error)
			: BLooper(looper_name_for(signature)),
			  fAppName(NULL),
			  fServerFrom(-1),
			  fServerTo(-1),
			  fServerHeap(NULL),
			  fPulseRate(500000),
			  fInitialWorkspace(0),
			  fDraggedMessage(NULL),
			  fPulseRunner(NULL),
			  fInitError(B_NO_INIT),
			  fReadyToRunCalled(false)
{
	InitData(signature, error);
}
//------------------------------------------------------------------------------
BApplication::~BApplication()
{
	// unregister from the roster
	be_roster->RemoveApp(Team());
}
//------------------------------------------------------------------------------
BApplication::BApplication(BMessage* data)
			: BLooper(looper_name_for(NULL)),
			  fAppName(NULL),
			  fServerFrom(-1),
			  fServerTo(-1),
			  fServerHeap(NULL),
			  fPulseRate(500000),
			  fInitialWorkspace(0),
			  fDraggedMessage(NULL),
			  fPulseRunner(NULL),
			  fInitError(B_NO_INIT),
			  fReadyToRunCalled(false)
{
}
//------------------------------------------------------------------------------
BArchivable* BApplication::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data, "BApplication"))
	{
		return NULL;
	}

	return new BApplication(data);
}
//------------------------------------------------------------------------------
status_t BApplication::Archive(BMessage* data, bool deep) const
{
	return NOT_IMPLEMENTED;
}
//------------------------------------------------------------------------------
status_t BApplication::InitCheck() const
{
	return fInitError;
}
//------------------------------------------------------------------------------
thread_id BApplication::Run()
{
	AssertLocked();

	if (fRunCalled)
	{
		// Not allowed to call Run() more than once
		// TODO: test
		// find out what message is actually here
		debugger("");
	}

	fTaskID = find_thread(NULL);

	if (fMsgPort == B_NO_MORE_PORTS || fMsgPort == B_BAD_VALUE)
	{
		return fMsgPort;
	}

	fRunCalled = true;

	run_task();

	return fTaskID;
}
//------------------------------------------------------------------------------
void BApplication::Quit()
{
	if (!IsLocked()) {
		const char* name = Name();
		if (!name)
			name = "no-name";
		printf("ERROR - you must Lock a looper before calling Quit(), "
			   "team=%ld, looper=%s", Team(), name);
	}
	// Set the termination flag. That's sufficient in some cases.
	fTerminating = true;
	// Delete the object, if not running only.
	if (!fRunCalled)
		delete this;
	// In case another thread called Quit(), things are a bit more complicated.
	// BLooper::Quit() handles that gracefully.
	else if (find_thread(NULL) != fTaskID)
		BLooper::Quit();
	// prevent the BLooper destructor from killing the main thread
	fTaskID = -1;
}
//------------------------------------------------------------------------------
bool BApplication::QuitRequested()
{
	// No windows -- nothing to do.
	return BLooper::QuitRequested();
}
//------------------------------------------------------------------------------
void BApplication::Pulse()
{
}
//------------------------------------------------------------------------------
void BApplication::ReadyToRun()
{
}
//------------------------------------------------------------------------------
void BApplication::MessageReceived(BMessage* msg)
{
	BLooper::MessageReceived(msg);
}
//------------------------------------------------------------------------------
void BApplication::ArgvReceived(int32 argc, char** argv)
{
}
//------------------------------------------------------------------------------
void BApplication::AppActivated(bool active)
{
}
//------------------------------------------------------------------------------
void BApplication::RefsReceived(BMessage* a_message)
{
}
//------------------------------------------------------------------------------
void BApplication::AboutRequested()
{
}
//------------------------------------------------------------------------------
BHandler* BApplication::ResolveSpecifier(BMessage* msg, int32 index,
										 BMessage* specifier, int32 form,
										 const char* property)
{
	return NULL;	// not implemented
}
//------------------------------------------------------------------------------
void BApplication::ShowCursor()
{
	// TODO: talk to app_server
}
//------------------------------------------------------------------------------
void BApplication::HideCursor()
{
	// TODO: talk to app_server
}
//------------------------------------------------------------------------------
void BApplication::ObscureCursor()
{
	// TODO: talk to app_server
}
//------------------------------------------------------------------------------
bool BApplication::IsCursorHidden() const
{
	// TODO: talk to app_server
	return false;	// not implemented
}
//------------------------------------------------------------------------------
void BApplication::SetCursor(const void* cursor)
{
	// BeBook sez: If you want to call SetCursor() without forcing an immediate
	//				sync of the Application Server, you have to use a BCursor.
	// By deductive reasoning, this function forces a sync. =)
	BCursor Cursor(cursor);
	SetCursor(&Cursor, true);
}
//------------------------------------------------------------------------------
void BApplication::SetCursor(const BCursor* cursor, bool sync)
{
	// TODO: talk to app_server
}
//------------------------------------------------------------------------------
int32 BApplication::CountWindows() const
{
	// BeBook sez: The windows list includes all windows explicitely created by
	//				the app ... but excludes private windows create by Be
	//				classes.
	// I'm taking this to include private menu windows, thus the incl_menus
	// param is false.
	return count_windows(false);
}
//------------------------------------------------------------------------------
BWindow* BApplication::WindowAt(int32 index) const
{
	// BeBook sez: The windows list includes all windows explicitely created by
	//				the app ... but excludes private windows create by Be
	//				classes.
	// I'm taking this to include private menu windows, thus the incl_menus
	// param is false.
	return window_at(index, false);
}
//------------------------------------------------------------------------------
int32 BApplication::CountLoopers() const
{
	// Tough nut to crack; not documented *anywhere*.  Dug down into BLooper and
	// found its private sLooperCount var
	return 0;	// not implemented
}
//------------------------------------------------------------------------------
BLooper* BApplication::LooperAt(int32 index) const
{
	return NULL;	// not implemented
}
//------------------------------------------------------------------------------
bool BApplication::IsLaunching() const
{
	return !fReadyToRunCalled;
}
//------------------------------------------------------------------------------
status_t BApplication::GetAppInfo(app_info* info) const
{
	return be_roster->GetRunningAppInfo(be_app->Team(), info);
}
//------------------------------------------------------------------------------
BResources* BApplication::AppResources()
{
	return NULL;	// not implemented
}
//------------------------------------------------------------------------------
void BApplication::DispatchMessage(BMessage* message, BHandler* handler)
{
	switch (message->what) {
		case B_ARGV_RECEIVED:
		{
			// build the argv vector
			status_t error = B_OK;
			int32 argc;
			char **argv = NULL;
			if (message->FindInt32("argc", &argc) == B_OK && argc > 0) {
				argv = new char*[argc];
				for (int32 i = 0; error == B_OK && i < argc; i++)
					argv[i] = NULL;
				// copy the arguments
				for (int32 i = 0; error == B_OK && i < argc; i++) {
					const char *arg = NULL;
					error = message->FindString("argv", i, &arg);
					if (error == B_OK && arg) {
						argv[i] = new(nothrow) char[strlen(arg) + 1];
						if (argv[i])
							strcpy(argv[i], arg);
						else
							error = B_NO_MEMORY;
					}
				}
			}
			// call the hook
			if (error == B_OK)
				ArgvReceived(argc, argv);
			// cleanup
			if (argv) {
				for (int32 i = 0; i < argc; i++)
					delete[] argv[i];
				delete[] argv;
			}
			break;
		}
		case B_REFS_RECEIVED:
			RefsReceived(message);
			break;
		case B_READY_TO_RUN:
			// TODO: Find out, whether to set fReadyToRunCalled before or
			// after calling the hook.
			ReadyToRun();
			fReadyToRunCalled = true;
			break;
		default:
			BLooper::DispatchMessage(message, handler);
			break;
	}
}
//------------------------------------------------------------------------------
void BApplication::SetPulseRate(bigtime_t rate)
{
	fPulseRate = rate;
}
//------------------------------------------------------------------------------
status_t BApplication::GetSupportedSuites(BMessage* data)
{
	return NOT_IMPLEMENTED;
}
//------------------------------------------------------------------------------
status_t BApplication::Perform(perform_code d, void* arg)
{
	return NOT_IMPLEMENTED;
}
//------------------------------------------------------------------------------
BApplication::BApplication(uint32 signature)
{
}
//------------------------------------------------------------------------------
BApplication::BApplication(const BApplication& rhs)
{
}
//------------------------------------------------------------------------------
BApplication& BApplication::operator=(const BApplication& rhs)
{
	return *this;
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication1()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication2()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication3()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication4()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication5()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication6()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication7()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication8()
{
}
//------------------------------------------------------------------------------
bool BApplication::ScriptReceived(BMessage* msg, int32 index, BMessage* specifier, int32 form, const char* property)
{
	return false;	// not implemented
}
//------------------------------------------------------------------------------
void BApplication::run_task()
{
	task_looper();
}
//------------------------------------------------------------------------------
void BApplication::InitData(const char* signature, status_t* error)
{
	// check signature
	assert_app_signature(signature);
	fAppName = signature;
	bool isRegistrar = (signature && !strcmp(signature, kRegistrarSignature));
	// get team and thread
	team_id team = Team();
	thread_id thread = main_thread_for(team);
	// get app executable ref
	entry_ref ref;
	char appFilePath[B_PATH_NAME_LENGTH + 1];
	fInitError = get_app_path(appFilePath);
	if (fInitError == B_OK) {
		BEntry entry(appFilePath, true);
		fInitError = entry.GetRef(&ref);
	}
	// get the BAppFileInfo and extract the information we need
	uint32 appFlags = B_REG_DEFAULT_APP_FLAGS;
	if (fInitError == B_OK) {
		BAppFileInfo fileInfo;
		BFile file(&ref, B_READ_ONLY);
		fInitError = fileInfo.SetTo(&file);
		if (fInitError == B_OK) {
			fileInfo.GetAppFlags(&appFlags);
			char appFileSignature[B_MIME_TYPE_LENGTH + 1];
			// compare the file signature and the supplied signature
			if (fileInfo.GetSignature(appFileSignature) == B_OK) {
				if (strcmp(appFileSignature, signature) != 0) {
					printf("Signature in rsrc doesn't match constructor arg. "
						   "(%s,%s)\n", signature, appFileSignature);
				}
			}
		}
	}
	// check whether or not we are pre-registered
	bool preRegistered = false;
	app_info appInfo;
	if (fInitError == B_OK && !isRegistrar)
		preRegistered = be_roster->IsAppPreRegistered(&ref, team, &appInfo);
	if (preRegistered) {
		// we are pre-registered => the app info has been filled in
		// Check whether we need to replace the looper port with a port
		// created by the roster.
		if (appInfo.port >= 0 && appInfo.port != fMsgPort) {
			delete_port(fMsgPort);
			fMsgPort = appInfo.port;
		} else
			appInfo.port = fMsgPort;
		// Create a B_ARGV_RECEIVED message and send it to ourselves.
		// TODO: When BLooper::AddMessage() is done, use that instead of
		// PostMessage().
		if (!(appFlags & B_ARGV_ONLY) && __libc_argc > 1) {
			BMessage argvMessage(B_ARGV_RECEIVED);
			do_argv(&argvMessage);
			PostMessage(&argvMessage, this);
		}
		// complete the registration
		fInitError = be_roster->CompleteRegistration(team, thread,
													 appInfo.port);
	} else if (fInitError == B_OK) {
		// not pre-registered -- try to register the application
		team_id otherTeam = -1;
		status_t regError = B_OK;
		// the registrar must not register
		if (!isRegistrar) {
			regError = be_roster->AddApplication(signature, &ref, appFlags,
				team, thread, fMsgPort, true, NULL, &otherTeam);
		}
		if (regError == B_ALREADY_RUNNING) {
			// An instance is already running and we asked for
			// single/exclusive launch. Send our argv to the running app and
			// exit.
			if (!(appFlags & B_ARGV_ONLY) && otherTeam >= 0
				&& __libc_argc > 1) {
				BMessage argvMessage(B_ARGV_RECEIVED);
				do_argv(&argvMessage);
				// We need to replace the first argv string with the path
				// of the respective application.
				app_info otherAppInfo;
				if (be_roster->GetRunningAppInfo(otherTeam, &otherAppInfo)
					== B_OK) {
					BPath path;
					if (path.SetTo(&otherAppInfo.ref) == B_OK)
						argvMessage.ReplaceString("argv", 0, path.Path());
				}
				BMessenger(NULL, otherTeam).SendMessage(&argvMessage);
			}
			exit(0);
		}
		if (regError == B_OK) {
			// the registrations was successful
			// Create a B_ARGV_RECEIVED message and send it to ourselves.
			// TODO: When BLooper::AddMessage() is done, use that instead of
			// PostMessage().
			if (!(appFlags & B_ARGV_ONLY) && __libc_argc > 1) {
				BMessage argvMessage(B_ARGV_RECEIVED);
				do_argv(&argvMessage);
				PostMessage(&argvMessage, this);
			}
			// send a B_READY_TO_RUN message as well
			PostMessage(B_READY_TO_RUN, this);
		} else
			fInitError = regError;
	}
	// TODO: SetName()
	// return the error
	if (error)
		*error = fInitError;
}
//------------------------------------------------------------------------------
void BApplication::BeginRectTracking(BRect r, bool trackWhole)
{
}
//------------------------------------------------------------------------------
void BApplication::EndRectTracking()
{
}
//------------------------------------------------------------------------------
void BApplication::get_scs()
{
}
//------------------------------------------------------------------------------
void BApplication::setup_server_heaps()
{
}
//------------------------------------------------------------------------------
void* BApplication::rw_offs_to_ptr(uint32 offset)
{
	return NULL;	// not implemented
}
//------------------------------------------------------------------------------
void* BApplication::ro_offs_to_ptr(uint32 offset)
{
	return NULL;	// not implemented
}
//------------------------------------------------------------------------------
void* BApplication::global_ro_offs_to_ptr(uint32 offset)
{
	return NULL;	// not implemented
}
//------------------------------------------------------------------------------
void BApplication::connect_to_app_server()
{
}
//------------------------------------------------------------------------------
void BApplication::send_drag(BMessage* msg, int32 vs_token, BPoint offset, BRect drag_rect, BHandler* reply_to)
{
}
//------------------------------------------------------------------------------
void BApplication::send_drag(BMessage* msg, int32 vs_token, BPoint offset, int32 bitmap_token, drawing_mode dragMode, BHandler* reply_to)
{
}
//------------------------------------------------------------------------------
void BApplication::write_drag(_BSession_* session, BMessage* a_message)
{
}
//------------------------------------------------------------------------------
bool BApplication::quit_all_windows(bool force)
{
	return false;	// not implemented
}
//------------------------------------------------------------------------------
bool BApplication::window_quit_loop(bool, bool)
{
	return false;	// not implemented
}
//------------------------------------------------------------------------------
void BApplication::do_argv(BMessage* message)
{
	if (message) {
		int32 argc = __libc_argc;
		const char * const *argv = __libc_argv;
		// add argc
		message->AddInt32("argc", argc);
		// add argv
		for (int32 i = 0; i < argc; i++)
			message->AddString("argv", argv[i]);
		// add current working directory
		char cwd[B_PATH_NAME_LENGTH + 1];
		if (getcwd(cwd, B_PATH_NAME_LENGTH + 1))
			message->AddString("cwd", cwd);
	}
}
//------------------------------------------------------------------------------
uint32 BApplication::InitialWorkspace()
{
	return 0;	// not implemented
}
//------------------------------------------------------------------------------
int32 BApplication::count_windows(bool incl_menus) const
{
	return 0;	// not implemented
}
//------------------------------------------------------------------------------
BWindow* BApplication::window_at(uint32 index, bool incl_menus) const
{
	return NULL;	// not implemented
}
//------------------------------------------------------------------------------
status_t BApplication::get_window_list(BList* list, bool incl_menus) const
{
	return NOT_IMPLEMENTED;
}
//------------------------------------------------------------------------------
int32 BApplication::async_quit_entry(void* data)
{
	return 0;	// not implemented
}
//------------------------------------------------------------------------------

// assert_app_signature
//
// Terminates with and error message, when the supplied signature is not a
// valid application signature.
static
void
assert_app_signature(const char *signature)
{
	bool isValid = false;
	BMimeType type(signature);
	if (type.IsValid() && !type.IsSupertypeOnly()
		&& BMimeType("application").Contains(&type)) {
		isValid = true;
	}
	if (!isValid) {
		printf("bad signature (%s), must begin with \"application/\" and "
			   "can't conflict with existing registered mime types inside "
			   "the \"application\" media type.\n", signature);
		exit(1);
	}
}

// looper_name_for
//
// Returns the looper name for a given signature: Normally "AppLooperPort",
// but in case of the registrar a special name.
static
const char*
looper_name_for(const char *signature)
{
	if (signature && !strcmp(signature, kRegistrarSignature))
		return kRosterPortName;
	return "AppLooperPort";
}

// get_app_path
//
// Returns the path of the application. buffer must be of length
// B_PATH_NAME_LENGTH + 1.
static
status_t
get_app_path(char *buffer)
{
	status_t error = (buffer ? B_OK : B_BAD_VALUE);
	image_info info;
	int32 cookie = 0;
	bool found = false;
	if (error == B_OK) {
		while (!found && get_next_image_info(0, &cookie, &info) == B_OK) {
			if (info.type == B_APP_IMAGE) {
				strncpy(buffer, info.name, B_PATH_NAME_LENGTH);
				buffer[B_PATH_NAME_LENGTH] = 0;
				found = true;
			}
		}
	}
	if (error == B_OK && !found)
		error = B_ENTRY_NOT_FOUND;
	return error;
}

// main_thread_for
//
// Returns the ID of the supplied team's main thread.
static
thread_id
main_thread_for(team_id team)
{
	// For I can't find any trace of how to explicitly get the main thread,
	// I assume the main thread is the one with the least thread ID.
	thread_id thread = -1;
	int32 cookie = 0;
	thread_info info;
	while (get_next_thread_info(team, &cookie, &info) == B_OK) {
		if (thread < 0 || info.thread < thread)
			thread = info.thread;
	}
	return thread;
}


/*
 * $Log $
 *
 * $Id  $
 *
 */

