// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:			MouseWindow.cpp
//  Authors:		Jérôme Duval,
//					Andrew McCall (mccall@digitalparadise.co.uk)
//					Axel Dörfler (axeld@pinc-software.de)
//  Description:	Mouse Preferences
//  Created:		December 10, 2003
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <Alert.h>
#include <Application.h>
#include <Message.h>
#include <Screen.h>
#include <Slider.h>
#include <Button.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Debug.h>
#include <string.h>

#include "MouseWindow.h"
#include "MouseMessages.h"
#include "SettingsView.h"


MouseWindow::MouseWindow(BRect rect)
	: BWindow(rect, "Mouse", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BView *view = new BView(Bounds(), "view", B_FOLLOW_ALL, 0);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	// Add the main settings view
	fSettingsView = new SettingsView(Bounds().InsetBySelf(kBorderSpace + 1, kBorderSpace + 1),
		fSettings);
	view->AddChild(fSettingsView);

	// Add the "Default" button
	BRect rect(kBorderSpace, fSettingsView->Frame().bottom + kItemSpace + 2,
		kBorderSpace + 75, fSettingsView->Frame().bottom + 20);
	BButton *button = new BButton(rect, "defaults", "Defaults", new BMessage(BUTTON_DEFAULTS));
	button->ResizeToPreferred();
	view->AddChild(button);

	// Add the "Revert" button
	rect.OffsetBy(button->Bounds().Width() + kItemSpace, 0);
	fRevertButton = new BButton(rect, "revert", "Revert", new BMessage(BUTTON_REVERT));
	fRevertButton->SetEnabled(false);
	view->AddChild(fRevertButton);

	SetPulseRate(100000);
		// we are using the pulse rate to scan pressed mouse
		// buttons and draw the selected imagery

	ResizeTo(fSettingsView->Frame().right + kBorderSpace, button->Frame().bottom + kBorderSpace - 1);
	MoveTo(fSettings.WindowPosition());
}


bool
MouseWindow::QuitRequested()
{
	fSettings.SetWindowPosition(Frame().LeftTop());
	be_app->PostMessage(B_QUIT_REQUESTED);	

	return true;
}


void
MouseWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case BUTTON_DEFAULTS: {
			// reverts to default settings
			fSettingsView->dcSpeedSlider->SetValue(500);
			fSettingsView->dcSpeedSlider->Invoke();
			fSettingsView->mouseSpeedSlider->SetValue(500);
			fSettingsView->mouseSpeedSlider->Invoke();
			fSettingsView->mouseAccSlider->SetValue(500);
			fSettingsView->mouseAccSlider->Invoke();
			fSettingsView->focusMenu->ItemAt(0)->SetMarked(true);
			set_mouse_type(3);
			fSettingsView->mouseTypeMenu->ItemAt(2)->SetMarked(true);
			set_mouse_mode(B_NORMAL_MOUSE);
			fSettingsView->fCurrentMouseMap.button[0] = B_PRIMARY_MOUSE_BUTTON;
			fSettingsView->fCurrentMouseMap.button[1] = B_SECONDARY_MOUSE_BUTTON;
			fSettingsView->fCurrentMouseMap.button[2] = B_TERTIARY_MOUSE_BUTTON;
			set_mouse_map(&fSettingsView->fCurrentMouseMap);

			SetRevertable(true);
			break;
		}

		case BUTTON_REVERT: {
			// revert to last settings
			fSettingsView->Init();
			fSettings.Revert();
			//fSettingsView->Update();

			SetRevertable(false);
			break;
		}

		case POPUP_MOUSE_TYPE: {
			status_t err = set_mouse_type(fSettingsView->mouseTypeMenu->IndexOf(fSettingsView->mouseTypeMenu->FindMarked())+1);
			if (err < B_OK)
				printf("error while setting mouse type : %s\n", strerror(err));

			SetRevertable(true);
			break;
		}

		case POPUP_MOUSE_FOCUS: {
			mode_mouse mouse_mode = B_NORMAL_MOUSE;
			switch (fSettingsView->focusMenu->IndexOf(fSettingsView->focusMenu->FindMarked())) {
				case 0: mouse_mode = B_NORMAL_MOUSE; break;
				case 1: mouse_mode = B_FOCUS_FOLLOWS_MOUSE; break;
				case 2: mouse_mode = B_WARP_MOUSE; break;
				case 3: mouse_mode = B_INSTANT_WARP_MOUSE; break;
			}
			set_mouse_mode(mouse_mode);

			SetRevertable(true);
			break;
		}

		case SLIDER_DOUBLE_CLICK_SPEED: {
			int32 value = fSettingsView->dcSpeedSlider->Value();
			int32 click_speed;	// slow = 1000000, fast = 0
			click_speed = (int32) (1000000 - value * 1000); 
			status_t err = set_click_speed(click_speed);
			if (err < B_OK)
				printf("error while setting click speed : %s\n", strerror(err));

			SetRevertable(true);
			break;
		}

		case SLIDER_MOUSE_SPEED: {
			int32 value = fSettingsView->mouseSpeedSlider->Value();
			int32 mouse_speed;	// slow = 8192, fast = 524287
			mouse_speed = (int32) pow(2, value * 6 / 1000) * 8192; 
			status_t err = set_mouse_speed(mouse_speed);
			if (err < B_OK)
				printf("error while setting mouse speed : %s\n", strerror(err));

			SetRevertable(true);
			break;
		}

		case SLIDER_MOUSE_ACC: {
			int32 value = fSettingsView->mouseAccSlider->Value();
			int32 mouse_acc;	// slow = 0, fast = 262144
			mouse_acc = (int32) pow(value * 4 / 1000, 2) * 16384;
			status_t err = set_mouse_acceleration(mouse_acc);
			if (err < B_OK)
				printf("error while setting mouse acceleration : %s\n", strerror(err));

			SetRevertable(true);
			break;
		}

		case POPUP_MOUSE_MAP: {
			int32 index = fSettingsView->mouseMapMenu->IndexOf(fSettingsView->mouseMapMenu->FindMarked());
			int32 number = B_PRIMARY_MOUSE_BUTTON;
			switch (index) {
				case 0: number = B_PRIMARY_MOUSE_BUTTON; break;
				case 1: number = B_SECONDARY_MOUSE_BUTTON; break;
				case 2: number = B_TERTIARY_MOUSE_BUTTON; break;
			}
			fSettingsView->fCurrentMouseMap.button[fSettingsView->fCurrentButton] = number;
			status_t err = set_mouse_map(&fSettingsView->fCurrentMouseMap);
			fSettingsView->fCurrentButton = -1;
			if (err < B_OK)
				printf("error while setting mouse map : %s\n", strerror(err));

			SetRevertable(true);
			break;
		}

		case ERROR_DETECTED: {
			(new BAlert("Error", "Something has gone wrong!", "OK", NULL, NULL,
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT))->Go();
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void 
MouseWindow::SetRevertable(bool revertable)
{
	fRevertButton->SetEnabled(revertable);
}

