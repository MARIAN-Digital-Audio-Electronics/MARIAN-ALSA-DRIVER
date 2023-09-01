// TODO ToG: Add license header

#include <linux/irqreturn.h>
#include "clara.h"
#include "dma_ng.h"

#define REG_ADDR_INCREASE 4 // register byte alignment
#define ADDR_RESET_DMA_ENGINE_REG 0x00
#define ADDR_PREPARE_RUN_REG 0x84
#define MASK_ENGINE_RUN 1
#define MASK_ENGINE_PREPARE (1<<1)
#define ADDR_BASE_PLAYBACK_CHANNELS_REGS 0x100
#define ADDR_BASE_CAPTURE_CHANNELS_REGS 0x180
#define ADDR_NUM_BLOCKS_REG 0x10
#define ADDR_IRQ_DISABLE_REG 0xAC
#define MASK_IRQ_DISABLE_PLAYBACK (1<<1)
#define MASK_IRQ_DISABLE_CAPTURE (1<<2)
#define MASK_IRQ_DMA_LOOPBACK (1<<3)
#define MASK_STATUS_IDLE (1<<4)
#define MASK_IRQ_STATUS_CAPTURE (1<<11)
#define MASK_STATUS_CAPTURE_PAGE (1<<12)
#define MASK_IRQ_STATUS_PREPARED (1<<14)
#define ADDR_NUM_SLICES_REG 0xB0
#define ADDR_BASE_PLAYBACK_HOST_ADDR_REGS 0x200
#define ADDR_BASE_CAPTURE_HOST_ADDR_REGS 0x300
#define ADDR_XILINX_H2C_REG 0x4
#define ADDR_XILINX_C2H_REG 0x1004
#define ADDR_XILINX_IRQ_ENABLE_REG 0x2004
// to make things not too complicated, we fix the number of channels per slice
#define CHANNELS_PER_SLICE 512

int dma_enable_interrupts(struct generic_chip *chip)
{
	// enable xilinx core interrupts and transport engine
	write_reg32_bar1(chip, ADDR_XILINX_H2C_REG, 1);
	write_reg32_bar1(chip, ADDR_XILINX_C2H_REG, 1);
	write_reg32_bar1(chip, ADDR_XILINX_IRQ_ENABLE_REG, 1);
	// enable capture interrupts (besides the prepare IRQ the only one
	// we are interested in)
	// since at the moment we have no shadow registers, we need to
	// take care of the DMA loopback as well whiche is located in the
	// same register
	write_reg32_bar0(chip, ADDR_IRQ_DISABLE_REG,
		0xFFFFFFFF & ~(MASK_IRQ_DMA_LOOPBACK | MASK_IRQ_DISABLE_CAPTURE));
//	write_reg32_bar0(chip, ADDR_IRQ_DISABLE_REG, 0x02);
	return 0;
}

int dma_disable_interrupts(struct generic_chip *chip)
{
	// One strategy would be to completely silence the card
	// but I decided to not do this.
	// Any interrupt occuring after this call will be handled by the
	// ISR anyways.
//	disable xilinx core interrupts and transport engine
//	write_reg32_bar1(chip, ADDR_XILINX_H2C_REG, 0);
//	write_reg32_bar1(chip, ADDR_XILINX_C2H_REG, 0);
//	write_reg32_bar1(chip, ADDR_XILINX_IRQ_ENABLE_REG, 0);
//	disable all interrupts
//	write_reg32_bar0(chip, ADDR_IRQ_DISABLE_REG,
//		MASK_IRQ_DISABLE_CAPTURE | MASK_IRQ_DISABLE_PLAYBACK);
	return 0;
}

int dma_reset_engine(struct generic_chip *chip)
{
	u32 val = 0;
	printk(KERN_ERR "dma_reset_engine");
	// TODO ToG: clear channel enables
	// TODO ToG: clear slice addresses
	write_reg32_bar0(chip, ADDR_PREPARE_RUN_REG, 0);
	write_reg32_bar0(chip, ADDR_RESET_DMA_ENGINE_REG, 0);

	// TODO ToG: we might need to wait a little while before this check
	val = generic_get_irq_status(chip);
	if (!(val & MASK_STATUS_IDLE)) {
		printk(KERN_ERR "dma_reset_engine: machine not idle after "
			"reset: 0x%08X\n", val);
		chip->dma_status = DMA_STATUS_UNKNOWN;
		return -EIO;
	}

	chip->dma_status = DMA_STATUS_IDLE;
	return 0;
}

int dma_prepare(struct generic_chip *chip, unsigned int channels,
	bool playback, u64 host_base_addr, unsigned int num_blocks)
{
	// TODO ToG: this needs to go in the PCM setup section,
	// not in the DMA setup section

	// set clock mode, clock range, clock source
//	write_reg32_bar0(chip, 0x7C, 0x00000000); // clock divider
//	write_reg32_bar0(chip, 0x80, 0x00000000); // clock mode
//	write_reg32_bar0(chip, 0x8C, 0x00000001); // clock range
//	write_reg32_bar0(chip, 0x90, 0x00000000); // clock source

	// TODO ToG: either make dependent on playback/capture already
	// running or move to another place altogether
//	if (dma_reset_engine(chip) < 0)
//		return -EIO;

	// TODO ToG: setup channel enables
	write_reg32_bar0(chip , ADDR_BASE_PLAYBACK_CHANNELS_REGS, 0xf);
	write_reg32_bar0(chip , ADDR_BASE_CAPTURE_CHANNELS_REGS, 0xf);
	// TODO ToG: setup blocks (before slices!)
	write_reg32_bar0(chip, ADDR_NUM_BLOCKS_REG, num_blocks);
	// TODO ToG: setup slice addresses
	write_reg32_bar0(chip, ADDR_NUM_SLICES_REG, CHANNELS_PER_SLICE);
	if (playback) {
		write_reg32_bar0(chip,
			ADDR_BASE_PLAYBACK_HOST_ADDR_REGS,
			LOW_ADDR(host_base_addr));
		write_reg32_bar0(chip,
			ADDR_BASE_PLAYBACK_HOST_ADDR_REGS + 4,
			HIGH_ADDR(host_base_addr));
	}
	else {
		write_reg32_bar0(chip,
			ADDR_BASE_CAPTURE_HOST_ADDR_REGS,
			LOW_ADDR(host_base_addr));
		write_reg32_bar0(chip,
			ADDR_BASE_CAPTURE_HOST_ADDR_REGS + 4,
			HIGH_ADDR(host_base_addr));
	}

	dma_enable_interrupts(chip);
	return 0;
}

int dma_start(struct generic_chip *chip)
{
	write_reg32_bar0(chip, ADDR_PREPARE_RUN_REG, MASK_ENGINE_PREPARE | MASK_ENGINE_RUN);
	chip->dma_status = DMA_STATUS_RUNNING;
	return 0;
}

int dma_stop(struct generic_chip *chip)
{
	write_reg32_bar0(chip, ADDR_PREPARE_RUN_REG, 0);
	chip->dma_status = DMA_STATUS_IDLE;
	return 0;
}

irqreturn_t dma_irq_handler(int irq, void *dev_id)
{
	struct generic_chip *chip = dev_id;
	static unsigned int count = 0;
	u32 val = generic_get_irq_status(chip);
	if (val == 0)
		return IRQ_NONE;

	if (val & MASK_IRQ_STATUS_PREPARED) {
		snd_printk(KERN_DEBUG "dma_irq_handler: prepare IRQ\n");
	}
	if (val & MASK_IRQ_STATUS_CAPTURE) {
		if (chip->playback_substream)
			snd_pcm_period_elapsed(chip->playback_substream);
		if (chip->capture_substream)
			snd_pcm_period_elapsed(chip->capture_substream);
	}
	if (!chip->playback_substream && !chip->capture_substream) {
		dma_disable_interrupts(chip);
		snd_printk(KERN_ERR "dma_irq_handler: caught dangling IRQ\n");
	}

	count++;
	return IRQ_HANDLED;
}
