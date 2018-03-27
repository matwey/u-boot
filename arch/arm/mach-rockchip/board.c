/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <debug_uart.h>
#include <ram.h>
#include <syscon.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/periph.h>
#include <asm/arch/boot_mode.h>
#ifdef CONFIG_DM_REGULATOR
#include <power/regulator.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_USB_FUNCTION_FASTBOOT)
int fb_set_reboot_flag(void)
{
	printf("Setting reboot to fastboot flag ...\n");
	/* Set boot mode to fastboot */
	writel(BOOT_FASTBOOT, CONFIG_ROCKCHIP_BOOT_MODE_REG);

	return 0;
}

#define FASTBOOT_KEY_GPIO 43 /* GPIO1_B3 */
static int fastboot_key_pressed(void)
{
	gpio_request(FASTBOOT_KEY_GPIO, "fastboot_key");
	gpio_direction_input(FASTBOOT_KEY_GPIO);
	return !gpio_get_value(FASTBOOT_KEY_GPIO);
}
#endif

__weak int rk_board_init(void)
{
	return 0;
}

__weak int rk_board_late_init(void)
{
	return 0;
}

int board_late_init(void)
{
#if defined(CONFIG_USB_FUNCTION_FASTBOOT)
	if (fastboot_key_pressed()) {
		printf("fastboot key pressed!\n");
		fb_set_reboot_flag();
	}
#endif

#if (CONFIG_ROCKCHIP_BOOT_MODE_REG > 0)
	setup_boot_mode();
#endif

	return rk_board_late_init();
}

int board_init(void)
{
	int ret;

#if !defined(CONFIG_SUPPORT_SPL)
	board_debug_uart_init();
#endif
#ifdef CONFIG_DM_REGULATOR
	ret = regulators_enable_boot_on(false);
	if (ret)
		debug("%s: Cannot enable boot on regulator\n", __func__);
#endif

	return rk_board_init();
}

#if !defined(CONFIG_SYS_DCACHE_OFF) && !defined(CONFIG_ARM64)
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif

#if defined(CONFIG_USB_GADGET) && defined(CONFIG_USB_GADGET_DWC2_OTG)
#include <usb.h>
#include <usb/dwc2_udc.h>

static struct dwc2_plat_otg_data otg_data = {
	.rx_fifo_sz	= 512,
	.np_tx_fifo_sz	= 16,
	.tx_fifo_sz	= 128,
};

int board_usb_init(int index, enum usb_init_type init)
{
	int node;
	const char *mode;
	bool matched = false;
	const void *blob = gd->fdt_blob;

	/* find the usb_otg node */
	node = fdt_node_offset_by_compatible(blob, -1,
					     "snps,dwc2");

	while (node > 0) {
		mode = fdt_getprop(blob, node, "dr_mode", NULL);
		if (mode && strcmp(mode, "otg") == 0) {
			matched = true;
			break;
		}

		node = fdt_node_offset_by_compatible(blob, node,
						     "snps,dwc2");
	}
	if (!matched) {
		debug("Not found usb_otg device\n");
		return -ENODEV;
	}
	otg_data.regs_otg = fdtdec_get_addr(blob, node, "reg");

	return dwc2_udc_probe(&otg_data);
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	return 0;
}
#endif
