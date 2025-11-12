/*
 * MARIAN PCIe soundcards ALSA driver
 *
 * Author: Tobias Groß <theguy@audio-fpga.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details at:
 * http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef MARIAN_CLARA_EMIN_H
#define MARIAN_CLARA_EMIN_H

#define CLARA_EMIN_DEVICE_ID 0x9820

// revisions this driver is able to handle
#define CLARA_EMIN_MIN_HW_REVISION 0x04
#define CLARA_EMIN_MAX_HW_REVISION 0x05

#define CLARA_EMIN_CARD_NAME "ClaraEmin"

void clara_emin_register_device_specifics(struct device_specifics
	*dev_specifics);

#endif
