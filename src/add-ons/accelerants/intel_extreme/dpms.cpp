/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "accelerant.h"


void
enable_display_plane(bool enable)
{
	uint32 oldValue = read32(INTEL_DISPLAY_CONTROL);

	if (enable) {
		// when enabling the display, the register values are updated automatically
		write32(INTEL_DISPLAY_CONTROL, oldValue | DISPLAY_CONTROL_ENABLED);
	} else {
		// when disabling it, we have to trigger the update using a write to
		// the display base address
		write32(INTEL_DISPLAY_CONTROL, oldValue & ~DISPLAY_CONTROL_ENABLED);
		write32(INTEL_DISPLAY_BASE, 0);
	}
}


static void
enable_display_pipe(bool enable)
{
	uint32 oldValue = read32(INTEL_DISPLAY_PIPE_CONTROL);
	if (enable)
		write32(INTEL_DISPLAY_PIPE_CONTROL, oldValue | DISPLAY_PIPE_ENABLED);
	else
		write32(INTEL_DISPLAY_PIPE_CONTROL, oldValue & ~DISPLAY_PIPE_ENABLED);
}


void
set_display_power_mode(uint32 mode)
{
	uint32 monitorMode = 0;

	if (mode == B_DPMS_ON) {
		enable_display_pipe(true);
		enable_display_plane(true);
	}

	// TODO: wait for vblank!
	snooze(10000);

	switch (mode) {
		case B_DPMS_ON:
			monitorMode = DISPLAY_MONITOR_ON;
			break;
		case B_DPMS_SUSPEND:
			monitorMode = DISPLAY_MONITOR_SUSPEND;
			break;
		case B_DPMS_STAND_BY:
			monitorMode = DISPLAY_MONITOR_STAND_BY;
			break;
		case B_DPMS_OFF:
			monitorMode = DISPLAY_MONITOR_OFF;
			break;
	}

	write32(INTEL_DISPLAY_ANALOG_PORT, (read32(INTEL_DISPLAY_ANALOG_PORT)
		& ~(DISPLAY_MONITOR_MODE_MASK | DISPLAY_MONITOR_PORT_ENABLED))
		| monitorMode | (mode != B_DPMS_OFF ? DISPLAY_MONITOR_PORT_ENABLED : 0));

	if (mode != B_DPMS_ON) {
		enable_display_plane(false);
		enable_display_pipe(false);
	}
}


//	#pragma mark -


uint32
intel_dpms_capabilities(void)
{
	return B_DPMS_ON | B_DPMS_SUSPEND | B_DPMS_STAND_BY | B_DPMS_OFF;
}


uint32
intel_dpms_mode(void)
{
	return gInfo->shared_info->dpms_mode;
}


status_t
intel_set_dpms_mode(uint32 mode)
{
	gInfo->shared_info->dpms_mode = mode;
	set_display_power_mode(mode);

	return B_OK;
}

