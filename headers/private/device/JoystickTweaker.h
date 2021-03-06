/*
 * Copyright 2008, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fredrik Modeen
 */

#include "joystick_driver.h"

#include <List.h>
#include <Entry.h>

#if DEBUG
#include <stdio.h>
#endif

#define DEVICEPATH "/dev/joystick/"
#define JOYSTICKPATH "/boot/home/config/settings/joysticks/"

class BJoystick;

typedef struct _joystick_info {
        char            module_name[64];
        char            controller_name[64];
        int16           num_axes;
        int16           num_buttons;
        int16           num_hats;
        uint32          num_sticks;
        bool            calibration_enable;
        bigtime_t       max_latency;
        BList           name_axis;
        BList           name_hat;
        BList           name_button;
//      BList           name_
} joystick_info;

class _BJoystickTweaker {

public:
					_BJoystickTweaker();
					_BJoystickTweaker(BJoystick &stick);
		virtual		~_BJoystickTweaker();
		status_t	SendIOCT(uint32 op);
		status_t	GetInfo(_joystick_info* info, const char * ref);
		
		// BeOS R5's joystick pref need these
		status_t	save_config(const entry_ref * ref = NULL);
		void		scan_including_disabled();
		status_t	get_info();
		
private:
		void 		_BuildFromJoystickDesc(char *string, _joystick_info* info);
		status_t	_ScanIncludingDisabled(const char* rootPath, BList *list, 
						BEntry *rootEntry = NULL);
						
		void		_EmpyList(BList *list);		
		BJoystick* 	fJoystick;
#if DEBUG
public:
	static FILE *sLogFile;
#endif
};
