/*
 * Copyright 2003-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 *		Marcus Overhagen
 *		Jonas Sundström
 */

//! VolumeControl and link items in Deskbar

#include "VolumeSlider.h"
#include "DeskButton.h"
#include "iconfile.h"

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Deskbar.h>
#include <Dragger.h>
#include <File.h>
#include <FindDirectory.h>
#include <List.h>
#include <MenuItem.h>
#include <Message.h>
#include <MimeType.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <String.h>
#include <View.h>

#include <stdio.h>
#include <strings.h>

#define MEDIA_SETTINGS 'mese'
#define SOUND_SETTINGS 'sose'
#define OPEN_MEDIA_PLAYER 'omep'
#define TOGGLE_DONT_BEEP 'tdbp'
#define SET_VOLUME_WHICH 'svwh'

#define VOLUME_CTL_NAME "MediaReplicant"
	// R5 name needed, Media prefs manel removes by name

#define SETTINGS_FILE "x-vnd.Haiku-desklink"

const char *kAppSignature = "application/x-vnd.Haiku-desklink";
	// the application signature used by the replicant to find the
	// supporting code


class _EXPORT MediaReplicant;
	// the dragger part has to be exported

class MediaReplicant : public BView {
public:
	MediaReplicant(BRect frame, const char *name,
		uint32 resizeMask = B_FOLLOW_ALL, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
	MediaReplicant(BMessage *);
		// BMessage * based constructor needed to support archiving
	virtual ~MediaReplicant();

	// archiving overrides
	static MediaReplicant *Instantiate(BMessage *data);
	virtual	status_t Archive(BMessage *data, bool deep = true) const;

	// misc BView overrides
	virtual void AttachedToWindow();
	virtual void MouseDown(BPoint);
	virtual void MouseUp(BPoint);
	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage* message);

private:
	void _AlertFindDirectory(status_t status, const char *where);
	status_t _LaunchByPath(const char *path);
	status_t _LaunchBySig(const char *sig);
	void _LoadSettings();
	void _SaveSettings();

	BBitmap*		fSegments;
	VolumeSlider*	fVolumeSlider;
	bool 			fDontBeep;
		// don't beep on volume change
	int32 			fVolumeWhich;
		// which volume parameter to act on (Mixer/Phys.Output)
};

//
//	This is the exported function that will be used by Deskbar
//	to create and add the replicant
//
extern "C" _EXPORT BView* instantiate_deskbar_item();

BView *
instantiate_deskbar_item()
{
	return new MediaReplicant(BRect(0, 0, 16, 16), VOLUME_CTL_NAME);
}


MediaReplicant::MediaReplicant(BRect frame, const char *name,
		uint32 resizeMask, uint32 flags)
	: BView(frame, name, resizeMask, flags),
	fVolumeSlider(NULL)
{
	// Background Bitmap
	fSegments = new BBitmap(BRect(0, 0, kSpeakerWidth - 1, kSpeakerHeight - 1), B_CMAP8);
	fSegments->SetBits(kSpeakerBits, kSpeakerWidth*kSpeakerHeight, 0, B_CMAP8);
	_LoadSettings();
}


MediaReplicant::MediaReplicant(BMessage *message)
	: BView(message),
	fVolumeSlider(NULL)
{
	// Background Bitmap
	fSegments = new BBitmap(BRect(0, 0, 16 - 1, 16 - 1), B_CMAP8);
	fSegments->SetBits(kSpeakerBits, 16*16, 0, B_CMAP8);
	_LoadSettings();
}


MediaReplicant::~MediaReplicant()
{
	delete fSegments;
	_SaveSettings();
}


MediaReplicant *
MediaReplicant::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, VOLUME_CTL_NAME))
		return NULL;

	return new MediaReplicant(data);
}


status_t 
MediaReplicant::Archive(BMessage *data, bool deep) const
{
	status_t status = BView::Archive(data, deep);
	if (status < B_OK)
		return status;

	return data->AddString("add_on", kAppSignature);
}


void
MediaReplicant::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case B_ABOUT_REQUESTED:
		(new BAlert("About Volume Control", "Volume Control (Replicant)\n"
			    "  Brought to you by Jérôme DUVAL.\n\n"
			    "Copyright " B_UTF8_COPYRIGHT "2003-2008, Haiku","OK"))->Go(NULL);
		break;

	case OPEN_MEDIA_PLAYER:
	{
		BPath mediaPlayerPath;
		status_t status = find_directory(B_BEOS_APPS_DIRECTORY, &mediaPlayerPath);
		if (status != B_OK) {
			_AlertFindDirectory(status, __PRETTY_FUNCTION__);
			break;
		}
		mediaPlayerPath.Append("MediaPlayer");

		// launch the media player app
		if (_LaunchBySig("application/x-vnd.Haiku-MediaPlayer") == B_OK
			|| _LaunchBySig("application/x-vnd.Be.MediaPlayer") == B_OK
			|| _LaunchByPath(mediaPlayerPath.Path()) == B_OK)
			break;

		(new BAlert("desklink", "Couldn't launch MediaPlayer", "OK"))->Go();
		break;
	}

	case MEDIA_SETTINGS:
	{
		BPath mediaPrefsPath;
		status_t status = find_directory(B_BEOS_PREFERENCES_DIRECTORY,
			&mediaPrefsPath);
		if (status != B_OK) {
			_AlertFindDirectory(status, __PRETTY_FUNCTION__);
			break;
		}
		mediaPrefsPath.Append("Media");

		// launch the media prefs app
		if (_LaunchBySig("application/x-vnd.Haiku-Media") == B_OK
			|| _LaunchBySig("application/x-vnd.Be.MediaPrefs") == B_OK
			|| _LaunchByPath(mediaPrefsPath.Path()) == B_OK)
			break;

		(new BAlert("desklink", "Couldn't launch Media Preferences", "OK"))->Go();
		break;
	}

	case SOUND_SETTINGS:
	{
		BPath soundsPrefsPath;
		status_t status = find_directory(B_BEOS_PREFERENCES_DIRECTORY,
			&soundsPrefsPath);
		if (status != B_OK) {
			_AlertFindDirectory(status, __PRETTY_FUNCTION__);
			break;
		}
		soundsPrefsPath.Append("Sounds");
		
		// launch the sounds prefs app
		if (_LaunchBySig("application/x-vnd.Haiku-Sounds") == B_OK
			|| _LaunchBySig("application/x-vnd.Be.SoundsPrefs") == B_OK
			|| _LaunchByPath(soundsPrefsPath.Path()) == B_OK)
			break;

		(new BAlert("desklink", "Couldn't launch Sounds Preferences", "OK"))->Go();
		break;
	}

	case TOGGLE_DONT_BEEP:
		fDontBeep = !fDontBeep;
		break;

	case SET_VOLUME_WHICH:
		message->FindInt32("volwhich", &fVolumeWhich);
		break;
	
	case B_MOUSE_WHEEL_CHANGED:
	{
		float dy;
		if (message->FindFloat("be:wheel_delta_y", &dy) == B_OK
			&& dy != 0.0) {
			MixerControl mixerControl(fVolumeWhich);
			mixerControl.ChangeVolumeBy(dy < 0 ? 20 : -20);
		}
		break;
	}	
	default:
		BView::MessageReceived(message);
		break;		
	}
}


status_t
MediaReplicant::_LaunchByPath(const char *path)
{
	BEntry ent;
	entry_ref ref;
	app_info appInfo;
	status_t err;

	err = ent.SetTo(path);
	if (err)
		return err;
	err = ent.GetRef(&ref);
	if (err)
		return err;
	err = be_roster->Launch(&ref);
	if (err != B_ALREADY_RUNNING)
		return err; // should be B_OK or fatal error
	err = be_roster->GetAppInfo(&ref, &appInfo);
	if (err)
		return err;
	return be_roster->ActivateApp(appInfo.team);
}


status_t
MediaReplicant::_LaunchBySig(const char *sig)
{
	app_info appInfo;
	status_t err;

	err = be_roster->Launch(sig);
	if (err != B_ALREADY_RUNNING)
		return err; // should be B_OK or fatal error
	err = be_roster->GetAppInfo(sig, &appInfo);
	if (err)
		return err;
	return be_roster->ActivateApp(appInfo.team);
}


void
MediaReplicant::AttachedToWindow()
{
	BView *parent = Parent();
	if (parent)
		SetViewColor(parent->ViewColor());

	BView::AttachedToWindow();
}


void 
MediaReplicant::Draw(BRect rect)
{
	BView::Draw(rect);
	
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fSegments);
}


void
MediaReplicant::MouseDown(BPoint point)
{
	uint32 mouseButtons;
	BPoint where;
	GetMouse(&where, &mouseButtons, true);
	
	where = ConvertToScreen(point);
	
	if (mouseButtons & B_SECONDARY_MOUSE_BUTTON) {
		BPopUpMenu *menu = new BPopUpMenu("", false, false);
		menu->SetFont(be_plain_font);
		menu->AddItem(new BMenuItem("Media Preferences" B_UTF8_ELLIPSIS, new BMessage(MEDIA_SETTINGS)));
		menu->AddItem(new BMenuItem("Sound Preferences" B_UTF8_ELLIPSIS, new BMessage(SOUND_SETTINGS)));
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem("Open MediaPlayer", new BMessage(OPEN_MEDIA_PLAYER)));
		menu->AddSeparatorItem();
		BMenuItem *tmpItem = new BMenuItem("Don't beep", new BMessage(TOGGLE_DONT_BEEP));
		menu->AddItem(tmpItem);
		tmpItem->SetMarked(fDontBeep);
		BMenu *volMenu = new BMenu("Act On");
		volMenu->SetFont(be_plain_font);
		BMessage *msg;
		msg = new BMessage(SET_VOLUME_WHICH);
		msg->AddInt32("volwhich", VOLUME_USE_MIXER);
		tmpItem = new BMenuItem("System Mixer", msg);
		tmpItem->SetMarked(fVolumeWhich == VOLUME_USE_MIXER);
		volMenu->AddItem(tmpItem);
		msg = new BMessage(SET_VOLUME_WHICH);
		msg->AddInt32("volwhich", VOLUME_USE_PHYS_OUTPUT);
		tmpItem = new BMenuItem("Physical Output", msg);
		volMenu->AddItem(tmpItem);
		tmpItem->SetMarked(fVolumeWhich == VOLUME_USE_PHYS_OUTPUT);
		menu->AddItem(volMenu);

		menu->AddSeparatorItem();
		menu->SetFont(be_plain_font);
		menu->AddItem(new BMenuItem("About" B_UTF8_ELLIPSIS,
			new BMessage(B_ABOUT_REQUESTED)));

		menu->SetTargetForItems(this);
		volMenu->SetTargetForItems(this);
		menu->Go(where, true, true, BRect(where - BPoint(4, 4), 
			where + BPoint(4, 4)));
	} else if (mouseButtons & B_PRIMARY_MOUSE_BUTTON) {
		// Show VolumeSlider
		fVolumeSlider = new VolumeSlider(BRect(where.x, where.y, where.x + 207, where.y + 19),
			fDontBeep, fVolumeWhich);
		fVolumeSlider->Show();
	}
}


void
MediaReplicant::MouseUp(BPoint point)
{
	// don't Quit() ! thanks for FFM users
}


void
MediaReplicant::_LoadSettings()
{
	fDontBeep = false;
	fVolumeWhich = VOLUME_USE_MIXER;

	BPath p;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &p, false) < B_OK)
		return;
	p.SetTo(p.Path(), SETTINGS_FILE);
	BFile settings(p.Path(), B_READ_ONLY);
	if (settings.InitCheck() < B_OK)
		return;
	BMessage msg;
	if (msg.Unflatten(&settings) < B_OK)
		return;
	msg.FindInt32("volwhich", &fVolumeWhich);
	msg.FindBool("dontbeep", &fDontBeep);
}


void
MediaReplicant::_SaveSettings()
{
	BPath p;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &p, false) < B_OK)
		return;
	p.SetTo(p.Path(), SETTINGS_FILE);
	BFile settings(p.Path(), B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE);
	if (settings.InitCheck() < B_OK)
		return;
	BMessage msg('CNFG');
	msg.AddInt32("volwhich", fVolumeWhich);
	msg.AddBool("dontbeep", fDontBeep);
	ssize_t len=0;
	if (msg.Flatten(&settings, &len) < B_OK)
		return;
}


void 
MediaReplicant::_AlertFindDirectory(status_t status, const char *where)
{
	BString errorMessage;
	errorMessage << "At " << where << "\n";
	errorMessage << "find_directory() failed. \nReason: ";
	errorMessage << strerror(status);
	(new BAlert("AlertError", errorMessage.String(), "OK", NULL, NULL,
		B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
}


int
main(int, char **argv)
{	
	BApplication app(kAppSignature);
	bool atLeastOnePath = false;
	BList titleList;
	BList actionList;
	BDeskbar deskbar;
	status_t err = B_OK;

	for (int32 i = 1; argv[i]!=NULL; i++) {
		if (strcmp(argv[i], "--help") == 0)
			break;
		
		if (strcmp(argv[i], "--list") == 0) {
			int32 i, found = 0, count;
			count = deskbar.CountItems();
			printf("Deskbar items:\n");
			// the API is doomed, so don't try to enum for too long
			for (i = 0; (found < count) && (i >= 0) && (i < 5000); i++) {
				const char scratch[2] = ""; // BDeskbar is buggy
				const char *name=scratch;
				if (deskbar.GetItemInfo(i, &name) >= B_OK) {
					found++;
					printf("Item %ld: '%s'\n", i, name);
					free((void *)name); // INTENDED
				}
			}
			return 0;
		}

		if (strncmp(argv[i], "--remove", 8) == 0) {
			BString replicant = "DeskButton";
			if (strncmp(argv[i] + 8, "=", 1) == 0) {
				if (strlen(argv[i] + 9) > 0) {
					replicant = argv[i] + 9;
				} else {
					printf("desklink: Missing replicant name.\n");
					return 1;
				}
			}
			int32 found = 0;
			int32 found_id;
			while (deskbar.GetItemInfo(replicant.String(), &found_id) == B_OK) {
				err = deskbar.RemoveItem(found_id);
				if (err != B_OK) {
					printf("desklink: Error removing replicant id %ld: %s\n",
						found_id, strerror(err));
					break;
				}
				found++;
			}
			printf("Removed %ld items.\n", found);
			return err;
		}

		if (strncmp(argv[i], "cmd=", 4) == 0) {
			BString *title = new BString(argv[i] + 4);
			int32 index = title->FindFirst(':');
			if (index <= 0) {
				printf("desklink: usage: cmd=title:action\n");
			} else {
				title->Truncate(index);
				BString *action = new BString(argv[i] + 4);
				action->Remove(0, index+1);
				titleList.AddItem(title);
				actionList.AddItem(action);
			}
			continue;
		}

		atLeastOnePath = true;

		BEntry entry(argv[i], true);
		entry_ref ref;
		
		if (entry.Exists()) {
			entry.GetRef(&ref);
		} else if (BMimeType::IsValid(argv[i])) {
			if (be_roster->FindApp(argv[i], &ref) != B_OK) {
				printf("desklink: cannot find '%s'\n", argv[i]);
				return 1;
			}
		} else {
			printf("desklink: cannot find '%s'\n", argv[i]);
			return 1;
		}
		
		err = deskbar.AddItem(&ref);
		if (err != B_OK) {
			err = deskbar.AddItem(new DeskButton(BRect(0, 0, 15, 15),
				&ref, "DeskButton", titleList, actionList));
			if (err != B_OK) {
				printf("desklink: Deskbar refuses link to '%s': %s\n", argv[i], strerror(err));
				return 1;
			}
		}

		titleList.MakeEmpty();
		actionList.MakeEmpty();
	}

	if (!atLeastOnePath) {
		printf(	"usage: desklink { [ --list|--remove|[cmd=title:action ... ] [ path|signature ] } ...\n"
			"--list: list all Deskbar addons.\n"
			"--remove: remove all desklink addons.\n"
			"--remove=name: remove all 'name' addons.\n");
		return 1;
	}

	return 0;
}
