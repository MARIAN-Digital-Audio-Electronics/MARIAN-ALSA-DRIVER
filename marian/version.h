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

#ifndef MARIAN_VERSION_H
#define MARIAN_VERSION_H

#define  MARIAN_VERSION_MAJOR 1
#define  MARIAN_VERSION_MINOR 2
#define  MARIAN_VERSION_PATCH 0

#define __STRINGIFY(x) #x
#define __STRINGIFY__(x) __STRINGIFY(x)
#define MARIAN_DRIVER_VERSION_STRING \
    __STRINGIFY__(MARIAN_VERSION_MAJOR) "." \
    __STRINGIFY__(MARIAN_VERSION_MINOR) "." \
    __STRINGIFY__(MARIAN_VERSION_PATCH)

#endif // MARIAN_VERSION_H
