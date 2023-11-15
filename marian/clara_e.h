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

#ifndef MARIAN_CLARA_E_H
#define MARIAN_CLARA_E_H

#define CLARA_E_DEVICE_ID 0x9050

// revisions this driver is able to handle
#define CLARA_E_MIN_HW_REVISION 0x06
#define CLARA_E_MAX_HW_REVISION 0x06

#define CLARA_E_CARD_NAME "ClaraE"

void clara_e_register_device_specifics(struct device_specifics *dev_specifics);

#endif
