// TODO ToG: add license header

#include <linux/types.h>
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
		snd_printk(KERN_ERR
			"verify_device_specifics: dev_specifics is NULL\n");
		valid = false;
	}
	if (dev_specifics->hw_revision_valid == NULL) {
		snd_printk(KERN_ERR
			"verify_device_specifics: hw_revision_valid is NULL\n");
		valid = false;
	}
	if (dev_specifics->get_hw_revision_range == NULL) {
		snd_printk(KERN_ERR
			"verify_device_specifics: get_hw_revision_range is NULL\n");
		valid = false;
	}
	if (dev_specifics->card_name == NULL) {
		snd_printk(KERN_ERR
			"verify_device_specifics: card_name is NULL\n");
		valid = false;
	}
	if (dev_specifics->chip_new == NULL) {
		snd_printk(KERN_ERR
			"verify_device_specifics: chip_new is NULL\n");
		valid = false;
	}
	if (dev_specifics->chip_free == NULL) {
		snd_printk(KERN_ERR
			"verify_device_specifics: chip_free is NULL\n");
		valid = false;
	}
	if (dev_specifics->detect_hw_presence == NULL) {
		snd_printk(KERN_ERR
			"verify_device_specifics: detect_hw_presence is NULL\n");
		valid = false;
	}
	if (dev_specifics->soft_reset == NULL) {
		snd_printk(KERN_ERR
			"verify_device_specifics: soft_reset is NULL\n");
		valid = false;
	}
	if (dev_specifics->irq_handler == NULL) {
		snd_printk(KERN_ERR
			"verify_device_specifics: irq_handler is NULL\n");
		valid = false;
	}
	if (dev_specifics->pcm_playback_ops == NULL) {
		snd_printk(KERN_ERR
			"verify_device_specifics: pcm_playback_ops is NULL\n");
		valid = false;
	}
	if (dev_specifics->pcm_capture_ops == NULL) {
		snd_printk(KERN_ERR
			"verify_device_specifics: pcm_capture_ops is NULL\n");
		valid = false;
	}
	if (dev_specifics->timer_interval_ms == 0) {
		snd_printk(KERN_ERR
			"verify_device_specifics: timer_interval_ms is 0\n");
		valid = false;
	}
	if (dev_specifics->timer_callback == NULL) {
		snd_printk(KERN_ERR
			"verify_device_specifics: timer_callback is NULL\n");
		valid = false;
	}
	if (dev_specifics->create_controls == NULL) {
		snd_printk(KERN_ERR
			"verify_device_specifics: create_controls is NULL\n");
		valid = false;
	}

	return valid;
}
