/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */

#include <sys/bus.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>

#include <dev/ath/if_athvar.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(atheros, ath, pci)
NO_HAIKU_FBSD_MII_DRIVER();
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES|FBSD_WLAN);


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct ath_softc* ath = (struct ath_softc*)device_get_softc(dev);
	struct ath_hal* ah = ath->sc_ah;
	HAIKU_INTR_REGISTER_STATE;

	if (ath->sc_invalid) {
		/*
		 * The hardware is not ready/present, don't touch anything.
		 * Note this can happen early on if the IRQ is shared.
		 */
		return 0;
	}

	HAIKU_INTR_REGISTER_ENTER();
	if (!ath_hal_intrpend(ah)) {		/* shared irq, not for us */
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	/*
	 * We have to save the isr status right now.
	 * Some devices don't like having the interrupt disabled
	 * before accessing the isr status.
	 *
	 * Those devices return status 0, when status access
	 * occurs after disabling the interrupts with ath_hal_intrset.
	 *
	 * Note: Haiku's pcnet driver uses the same technique of
	 *       appending a sc_lastisr field.
	 */
	ath_hal_getisr(ah, &ath->sc_lastisr);
	ath_hal_intrset(ah, 0); // disable further intr's

	HAIKU_INTR_REGISTER_LEAVE();
	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct ath_softc* ath = (struct ath_softc*)device_get_softc(dev);
	struct ath_hal* ah = ath->sc_ah;

	ath_hal_intrset(ah, ath->sc_imask);
}
