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

#ifndef MARIAN_DBG_OUT_H
#define MARIAN_DBG_OUT_H

#include <sound/core.h>

#define DBG_LVL_ERROR	1
#define DBG_LVL_WARN	2
#define DBG_LVL_INFO	3
#define DBG_LVL_DEBUG	4

#ifdef DBG_LEVEL

	#if DBG_LEVEL >= DBG_LVL_ERROR
		#define PRINT_ERROR(fmt, args...) \
			do { \
				snd_printk(KERN_ERR KBUILD_MODNAME ": " fmt, ##args); \
			} while (0)
	#else
		#define PRINT_ERROR(fmt, args...) do {} while (0)
	#endif // DBG_LEVEL >= DBG_LVL_ERROR

	#if DBG_LEVEL >= DBG_LVL_WARN
		#define PRINT_WARN(fmt, args...) \
			do { \
				snd_printk(KERN_WARNING KBUILD_MODNAME ": " fmt, ##args); \
			} while (0)
	#else
		#define PRINT_WARN(fmt, args...) do {} while (0)
	#endif // DBG_LEVEL >= DBG_LVL_WARN

	#if DBG_LEVEL >= DBG_LVL_INFO
		#define PRINT_INFO(fmt, args...) \
			do { \
				snd_printk(KERN_INFO KBUILD_MODNAME ": " fmt, ##args); \
			} while (0)
	#else
		#define PRINT_INFO(fmt, args...) do {} while (0)
	#endif // DBG_LEVEL >= DBG_LVL_INFO


	#if DBG_LEVEL >= DBG_LVL_DEBUG
		#define PRINT_DEBUG(fmt, args...) \
			do { \
				snd_printk(KERN_DEBUG KBUILD_MODNAME ": " fmt, ##args); \
			} while (0)
	#else
		#define PRINT_DEBUG(fmt, args...) do {} while (0)
	#endif // DBG_LEVEL >= DBG_LVL_DEBUG

#else
	#define PRINT_ERROR(fmt, args...) do {} while (0)
	#define PRINT_WARN(fmt, args...) do {} while (0)
	#define PRINT_INFO(fmt, args...) do {} while (0)
	#define PRINT_DEBUG(fmt, args...) do {} while (0)
#endif // DBG_LEVEL

#endif // MARIAN_DBG_OUT_H
