/* ------------------------------------------------------------------
 *  ALSA compatibility stub for removed snd_printk()
 *  Only used when building against newer kernels.
 * ------------------------------------------------------------------ */
#ifndef SND_PRINTK_COMPAT_H
#define SND_PRINTK_COMPAT_H

#include <linux/printk.h>

/*
 * snd_printk(fmt, ...)
 * Historically a wrapper around printk() that prefixed messages
 * with source file and line when CONFIG_SND_VERBOSE_PRINTK was set.
 * For compatibility we just forward to printk() directly.
 */
#ifndef snd_printk
# define snd_printk(fmt, ...) printk(fmt, ##__VA_ARGS__)
#endif

#endif /* SND_PRINTK_COMPAT_H */
