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
}

bool verify_device_specifics(struct device_specifics *dev_specifics)
{
	if (dev_specifics == NULL)
		return false;
	if (dev_specifics->hw_revision_valid == NULL)
		return false;
	if (dev_specifics->get_hw_revision_range == NULL)
		return false;
	if (dev_specifics->card_name == NULL)
		return false;
	if (dev_specifics->chip_new == NULL)
		return false;
	if (dev_specifics->chip_free == NULL)
		return false;
	if (dev_specifics->detect_hw_presence == NULL)
		return false;
	if (dev_specifics->soft_reset == NULL)
		return false;
	if (dev_specifics->irq_handler == NULL)
		return false;
	if (dev_specifics->pcm_playback_ops == NULL)
		return false;
	if (dev_specifics->pcm_capture_ops == NULL)
		return false;
	return true;
}