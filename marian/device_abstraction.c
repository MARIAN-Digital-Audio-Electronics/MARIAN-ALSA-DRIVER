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

#include <linux/types.h>
#include "dbg_out.h"
#include "device_abstraction.h"

void clear_device_specifics(struct device_specifics *dev_specifics)
{
	if (dev_specifics == NULL)
		return;
	dev_specifics->hw_revision_valid = NULL;
	dev_specifics->get_hw_revision_range = NULL;
	dev_specifics->card_name = NULL;
	dev_specifics->chip_new = NULL;
	dev_specifics->chip_free = NULL;
	dev_specifics->detect_hw_presence = NULL;
	dev_specifics->soft_reset = NULL;
	dev_specifics->indicate_state = NULL;
	dev_specifics->alloc_dma_buffers = NULL;
	dev_specifics->measure_wordclock_hz = NULL;
	dev_specifics->irq_handler = NULL;
	dev_specifics->pcm_playback_ops = NULL;
	dev_specifics->pcm_capture_ops = NULL;
	dev_specifics->timer_interval_ms = 0;
	dev_specifics->timer_callback = NULL;
	dev_specifics->create_controls = NULL;
}

bool verify_device_specifics(struct device_specifics *dev_specifics)
{
	bool valid = true;
	if (dev_specifics == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: dev_specifics is NULL\n");
		valid = false;
	}
	if (dev_specifics->hw_revision_valid == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: hw_revision_valid is NULL\n");
		valid = false;
	}
	if (dev_specifics->get_hw_revision_range == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: get_hw_revision_range is NULL\n");
		valid = false;
	}
	if (dev_specifics->card_name == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: card_name is NULL\n");
		valid = false;
	}
	if (dev_specifics->chip_new == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: chip_new is NULL\n");
		valid = false;
	}
	if (dev_specifics->chip_free == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: chip_free is NULL\n");
		valid = false;
	}
	if (dev_specifics->detect_hw_presence == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: detect_hw_presence is NULL\n");
		valid = false;
	}
	if (dev_specifics->soft_reset == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: soft_reset is NULL\n");
		valid = false;
	}
	if (dev_specifics->indicate_state == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: indicate_state is NULL\n");
		valid = false;
	}
	if (dev_specifics->alloc_dma_buffers == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: alloc_dma_buffers is NULL\n");
		valid = false;
	}
	if (dev_specifics->measure_wordclock_hz == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: measure_wordclock_hz is NULL\n");
		valid = false;
	}
	if (dev_specifics->irq_handler == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: irq_handler is NULL\n");
		valid = false;
	}
	if (dev_specifics->pcm_playback_ops == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: pcm_playback_ops is NULL\n");
		valid = false;
	}
	if (dev_specifics->pcm_capture_ops == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: pcm_capture_ops is NULL\n");
		valid = false;
	}
	if (dev_specifics->timer_interval_ms == 0) {
		PRINT_ERROR(
			"verify_device_specifics: timer_interval_ms is 0\n");
		valid = false;
	}
	if (dev_specifics->timer_callback == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: timer_callback is NULL\n");
		valid = false;
	}
	if (dev_specifics->create_controls == NULL) {
		PRINT_ERROR(
			"verify_device_specifics: create_controls is NULL\n");
		valid = false;
	}

	return valid;
}
