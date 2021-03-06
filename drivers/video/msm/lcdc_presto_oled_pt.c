/* Copyright (c) 2009-2010, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/pwm.h>
#ifdef CONFIG_SPI_QUP
#include <linux/spi/spi.h>
#include <mach/gpio.h>
#else
#include <mach/gpio.h>
#endif
#include "msm_fb.h"

//#define DEBUG
/* #define SYSFS_DEBUG_CMD */

#if defined(CONFIG_MACH_MSM8X60_PRESTO) || defined(CONFIG_MACH_MSM8X60_QUANTINA) //EJ for presto
#define PANTECH_OLED_BL_CONTROL
#endif /* CONFIG_MACH_MSM8X60_PRESTO */

#if defined(CONFIG_MACH_MSM8X60_PRESTO) || defined(CONFIG_MACH_MSM8X60_QUANTINA)
#define PRESTO_OLED_GAMMA_SEQUENCE
#endif /* CONFIG_MACH_MSM8X60_PRESTO || CONFIG_MACH_MSM8X60_QUANTINA */

#ifdef CONFIG_SPI_QUP
#define LCDC_SAMSUNG_SPI_DEVICE_NAME	"lcdc_samsung_ams367pe02"
static struct spi_device *lcdc_spi_client;
#else
static int spi_cs;
static int spi_sclk;
static int spi_mosi;
#endif

struct samsung_state_type {
	boolean disp_initialized;
	boolean display_on;
	boolean disp_powered_up;
	int brightness;
};

struct samsung_spi_data {
	u8 addr;
	u8 len;
#if defined(CONFIG_MACH_MSM8X60_PRESTO) || defined(CONFIG_MACH_MSM8X60_QUANTINA) //EJ for presto
	u8 data[32];
#else /* CONFIG_MACH_MSM8X60_PRESTO || CONFIG_MACH_MSM8X60_QUANTINA */
	u8 data[22];
#endif /* CONFIG_MACH_MSM8X60_PRESTO || CONFIG_MACH_MSM8X60_QUANTINA */
};

static struct samsung_spi_data panel_sequence[] = {
	{ .addr = 0xf8, .len = 14, .data = { 0x01, 0x27, 0x27, 0x07, 0x07,
	 0x54, 0x9f, 0x63, 0x86, 0x1a, 0x33, 0x0d, 0x00, 0x00 } },
};
#ifdef PRESTO_OLED_GAMMA_SEQUENCE
static struct samsung_spi_data display_sequence[] = {
	{ .addr = 0xf2, .len = 5, .data = { 0x02, 0x03, 0x1c, 0x10, 0x10 } },
	{ .addr = 0xf7, .len = 3, .data = { 0x03, 0x00, 0x00 } }, // kkcho_temp
};
#else /* PRESTO_OLED_GAMMA_SEQUENCE */
static struct samsung_spi_data display_sequence[] = {
	{ .addr = 0xf2, .len = 5, .data = { 0x02, 0x03, 0x1c, 0x10, 0x10 } },
	{ .addr = 0xf7, .len = 3, .data = { 0x00, 0x00, 0x30 } },
};
#endif /* PRESTO_OLED_GAMMA_SEQUENCE */

#ifdef PRESTO_OLED_GAMMA_SEQUENCE
/* lum=300 cd/m2 */
static struct samsung_spi_data gamma_sequence_300[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x3A, 0x46,
	 0x18, 0xAC, 0xB7, 0xA1, 0xA7, 0xB0, 0x9B, 0xBB, 0xBF, 0xB1, 0x00, 0xBD,
	 0x00, 0xB0, 0x00, 0xFB } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
#else /* PRESTO_OLED_GAMMA_SEQUENCE */
/* lum=300 cd/m2 */
static struct samsung_spi_data gamma_sequence_300[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x7d, 0x77,
	 0x5b, 0xbe, 0xc1, 0xb1, 0xb3, 0xb7, 0xa6, 0xc3, 0xc5, 0xb9, 0x00, 0xb3,
	 0x00, 0xaf, 0x00, 0xe8 } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
#endif /* PRESTO_OLED_GAMMA_SEQUENCE */
#ifdef PRESTO_OLED_GAMMA_SEQUENCE
/* lum = 280 cd/m2*/
static struct samsung_spi_data gamma_sequence_280[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x3B, 0x46,
	 0x18, 0xAE, 0xB8, 0xA2, 0xA7, 0xB0, 0x9B, 0xBB, 0xC0, 0xB2, 0x00, 0xB9,
	 0x00, 0xAC, 0x00, 0xF5 } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
/* lum = 260 cd/m2*/
static struct samsung_spi_data gamma_sequence_260[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x3C, 0x47,
	 0x19, 0xAE, 0xB7, 0xA2, 0xA9, 0xB2, 0x9D, 0xBC, 0xC1, 0xB2, 0x00, 0xB2,
	 0x00, 0xA6, 0x00, 0xED } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
/* lum = 240 cd/m2*/
static struct samsung_spi_data gamma_sequence_240[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x3C, 0x47,
	 0x19, 0xAF, 0xB8, 0xA3, 0xAA, 0xB3, 0x9E, 0xBD, 0xC2, 0xB4, 0x00, 0xAC,
	 0x00, 0xA0, 0x00, 0xE4 } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
/* lum = 220 cd/m2*/
static struct samsung_spi_data gamma_sequence_220[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x3E, 0x44,
	 0x19, 0xB0, 0xB9, 0xA4, 0xAA, 0xB3, 0x9E, 0xBF, 0xC5, 0xB6, 0x00, 0xA5,
	 0x00, 0x99, 0x00, 0xDB } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
/* lum = 200 cd/m2*/
static struct samsung_spi_data gamma_sequence_200[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x40, 0x44,
	 0x1A, 0xB0, 0xBA, 0xA4, 0xAB, 0xB4, 0xA0, 0xC1, 0xC6, 0xB7, 0x00, 0x9E,
	 0x00, 0x93, 0x00, 0xD2 } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
#endif /* PRESTO_OLED_GAMMA_SEQUENCE */
#ifdef PRESTO_OLED_GAMMA_SEQUENCE
/* lum = 180 cd/m2*/
static struct samsung_spi_data gamma_sequence_180[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x41, 0x44,
	 0x1A, 0xB1, 0xBA, 0xA5, 0xAD, 0xB6, 0xA2, 0xC2, 0xC7, 0xB8, 0x00, 0x96,
	 0x00, 0x8C, 0x00, 0xC8 } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
#else /* PRESTO_OLED_GAMMA_SEQUENCE */
/* lum = 180 cd/m2*/
static struct samsung_spi_data gamma_sequence_180[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x83, 0x78,
	 0x60, 0xc5, 0xc6, 0xb8, 0xba, 0xbe, 0xad, 0xcb, 0xcd, 0xc2, 0x00, 0x92,
	 0x00, 0x8e, 0x00, 0xbc } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
#endif /* PRESTO_OLED_GAMMA_SEQUENCE */
#ifdef PRESTO_OLED_GAMMA_SEQUENCE
/* lum = 160 cd/m2*/
static struct samsung_spi_data gamma_sequence_160[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x40, 0x41,
	 0x18, 0xB1, 0xBA, 0xA6, 0xAE, 0xB7, 0xA2, 0xC3, 0xC8, 0xBA, 0x00, 0x90,
	 0x00, 0x86, 0x00, 0xBF } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
/* lum = 140 cd/m2*/
static struct samsung_spi_data gamma_sequence_140[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x43, 0x3E,
	 0x18, 0xB1, 0xBA, 0xA6, 0xB0, 0xB9, 0xA4, 0xC4, 0xCA, 0xBC, 0x00, 0x88,
	 0x00, 0x7E, 0x00, 0xB4 } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
/* lum = 120 cd/m2*/
static struct samsung_spi_data gamma_sequence_120[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x42, 0x3A,
	 0x17, 0xB3, 0xBB, 0xA7, 0xB1, 0xBA, 0xA6, 0xC6, 0xCC, 0xBD, 0x00, 0x7F,
	 0x00, 0x76, 0x00, 0xA9 } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
/* lum = 100 cd/m2*/
static struct samsung_spi_data gamma_sequence_100[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x44, 0x35,
	 0x18, 0xB4, 0xBC, 0xA8, 0xB2, 0xBB, 0xA7, 0xC8, 0xCE, 0xC0, 0x00, 0x76,
	 0x00, 0x6D, 0x00, 0x9C } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
#endif /* PRESTO_OLED_GAMMA_SEQUENCE */
#ifdef PRESTO_OLED_GAMMA_SEQUENCE
/* lum = 80 cd/m2*/
static struct samsung_spi_data gamma_sequence_80[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x46, 0x2E,
	 0x18, 0xB4, 0xBB, 0xA7, 0xB4, 0xBD, 0xAA, 0xCA, 0xD0, 0xC2, 0x00, 0x6C,
	 0x00, 0x64, 0x00, 0x8F } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
#else /* PRESTO_OLED_GAMMA_SEQUENCE */
/* lum = 80 cd/m2*/
static struct samsung_spi_data gamma_sequence_80[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x94, 0x73,
	 0x6c, 0xcb, 0xca, 0xbe, 0xc4, 0xc7, 0xb8, 0xd3, 0xd5, 0xcb, 0x00, 0x6d,
	 0x00, 0x69, 0x00, 0x8b } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
#endif /* PRESTO_OLED_GAMMA_SEQUENCE */
#ifdef PRESTO_OLED_GAMMA_SEQUENCE
/* lum = 60 cd/m2*/
static struct samsung_spi_data gamma_sequence_60[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x48, 0x26,
	 0x17, 0xB6, 0xBD, 0xA9, 0xB6, 0xBF, 0xAD, 0xCD, 0xD3, 0xC5, 0x00, 0x62,
	 0x00, 0x58, 0x00, 0x85 } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
/* lum = 40 cd/m2*/
static struct samsung_spi_data gamma_sequence_40[] = {
	{ .addr = 0xfa, .len = 22, .data = { 0x02, 0x18, 0x08, 0x24, 0x50, 0x1E,
	 0x17, 0xB7, 0xBF, 0xAB, 0xB8, 0xC1, 0xB0, 0xCF, 0xD4, 0xC7, 0x00, 0x59,
	 0x00, 0x4F, 0x00, 0x78 } },
	{ .addr = 0xFA, .len = 1, .data = { 0x03 } },
};
#endif /* PRESTO_OLED_GAMMA_SEQUENCE */

#ifdef PRESTO_OLED_GAMMA_SEQUENCE
static struct samsung_spi_data etc_sequence[] = {
	{ .addr = 0xF6, .len = 3, .data = { 0x00, 0x8e, 0x07 } },
	{ .addr = 0xB3, .len = 1, .data = { 0x6C } },
	{ .addr = 0xB5, .len = 32, .data = { 0x2C, 0x12, 0x0C, 0x0A, 0x10, 0x0E,
	 0x17, 0x13, 0x1F, 0x1A, 0x2A, 0x24, 0x1F, 0x1B, 0x1A, 0x17,
	 0x2B, 0x26, 0x22, 0x20, 0x3A, 0x34, 0x30, 0x2C, 0x29, 0x26,
	 0x25, 0x23, 0x21, 0x20, 0x1E, 0x1E } },
	{ .addr = 0xB6, .len = 16, .data = {0x00, 0x00, 0x11, 0x22, 0x33, 0x44,
	 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66  } },
	{ .addr = 0xB7, .len = 32, .data = { 0x2C, 0x12, 0x0C, 0x0A, 0x10, 0x0E,
	 0x17, 0x13, 0x1F, 0x1A, 0x2A, 0x24, 0x1F, 0x1B, 0x1A, 0x17,
	 0x2B, 0x26, 0x22, 0x20, 0x3A, 0x34, 0x30, 0x2C, 0x29, 0x26,
	 0x25, 0x23, 0x21, 0x20, 0x1E, 0x1E  } },
	{ .addr = 0xB8, .len = 16, .data = { 0x00, 0x00, 0x11, 0x22, 0x33, 0x44,
	 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66  } },
	{ .addr = 0xB9, .len = 32, .data = { 0x2C, 0x12, 0x0C, 0x0A, 0x10, 0x0E,
	 0x17, 0x13, 0x1F, 0x1A, 0x2A, 0x24, 0x1F, 0x1B, 0x1A, 0x17,
	 0x2B, 0x26, 0x22, 0x20, 0x3A, 0x34, 0x30, 0x2C, 0x29, 0x26,
	 0x25, 0x23, 0x21, 0x20, 0x1E, 0x1E  } },
	{ .addr = 0xBA, .len = 16, .data = { 0x00, 0x00, 0x11, 0x22, 0x33, 0x44,
	 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66  } },
};
#else /* PRESTO_OLED_GAMMA_SEQUENCE */
static struct samsung_spi_data etc_sequence[] = {
	{ .addr = 0xF6, .len = 3, .data = { 0x00, 0x8e, 0x07 } },
	{ .addr = 0xB3, .len = 1, .data = { 0x0C } },
};
#endif /* PRESTO_OLED_GAMMA_SEQUENCE */

static struct samsung_state_type samsung_state = { .brightness = 180 };
static struct msm_panel_common_pdata *lcdc_samsung_pdata;

#if defined(CONFIG_MACH_MSM8X60_PRESTO) || defined(CONFIG_MACH_MSM8X60_QUANTINA)  // kkcho_temp_presto
#define GPIO_HIGH_VALUE	1
#define GPIO_LOW_VALUE	       0
#define LCD_RESET 157

static uint32_t lcdc_gpio_config[] = {
	GPIO_CFG(LCD_RESET, 0,
	GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static void lcdc_gpio_init(int enable)
{
	int n;
	for (n = 0; n < ARRAY_SIZE(lcdc_gpio_config); ++n)
		gpio_tlmm_config(lcdc_gpio_config[n], 0);
}
#endif /* CONFIG_MACH_MSM8X60_PRESTO || CONFIG_MACH_MSM8X60_QUANTINA */

#ifndef CONFIG_SPI_QUP
static void samsung_spi_write_byte(boolean dc, u8 data)
{
	uint32 bit;
	int bnum;

	gpio_set_value(spi_sclk, 0);
	gpio_set_value(spi_mosi, dc ? 1 : 0);
	udelay(1);			/* at least 20 ns */
	gpio_set_value(spi_sclk, 1);	/* clk high */
	udelay(1);			/* at least 20 ns */

	bnum = 8;			/* 8 data bits */
	bit = 0x80;
	while (bnum--) {
		gpio_set_value(spi_sclk, 0); /* clk low */
		gpio_set_value(spi_mosi, (data & bit) ? 1 : 0);
		udelay(1);
		gpio_set_value(spi_sclk, 1); /* clk high */
		udelay(1);
		bit >>= 1;
	}
	gpio_set_value(spi_mosi, 0);

}

#if !defined(CONFIG_MACH_MSM8X60_PRESTO)
static void samsung_spi_read_bytes(u8 cmd, u8 *data, int num)
{
	int bnum;

	/* Chip Select - low */
	gpio_set_value(spi_cs, 0);
	udelay(2);

	/* command byte first */
	samsung_spi_write_byte(0, cmd);
	udelay(2);

	gpio_direction_input(spi_mosi);

	if (num > 1) {
		/* extra dummy clock */
		gpio_set_value(spi_sclk, 0);
		udelay(1);
		gpio_set_value(spi_sclk, 1);
		udelay(1);
	}

	/* followed by data bytes */
	bnum = num * 8;	/* number of bits */
	*data = 0;
	while (bnum) {
		gpio_set_value(spi_sclk, 0); /* clk low */
		udelay(1);
		*data <<= 1;
		*data |= gpio_get_value(spi_mosi) ? 1 : 0;
		gpio_set_value(spi_sclk, 1); /* clk high */
		udelay(1);
		--bnum;
		if ((bnum % 8) == 0)
			++data;
	}

	gpio_direction_output(spi_mosi, 0);

	/* Chip Select - high */
	udelay(2);
	gpio_set_value(spi_cs, 1);
}
#endif /* CONFIG_MACH_MSM8X60_PRESTO */
#endif

#ifdef DEBUG
#if !defined(CONFIG_MACH_MSM8X60_PRESTO)
static const char *byte_to_binary(const u8 *buf, int len)
{
	static char b[32*8+1];
	char *p = b;
	int i, z;

	for (i = 0; i < len; ++i) {
		u8 val = *buf++;
		for (z = 1 << 7; z > 0; z >>= 1)
			*p++ = (val & z) ? '1' : '0';
	}
	*p = 0;

	return b;
}
#endif /* CONFIG_MACH_MSM8X60_PRESTO */
#endif

#define BIT_OFFSET	(bit_size % 8)
#define ADD_BIT(val) do { \
		tx_buf[bit_size / 8] |= \
			(u8)((val ? 1 : 0) << (7 - BIT_OFFSET)); \
		++bit_size; \
	} while (0)

#define ADD_BYTE(data) do { \
		tx_buf[bit_size / 8] |= (u8)(data >> BIT_OFFSET); \
		bit_size += 8; \
		if (BIT_OFFSET != 0) \
			tx_buf[bit_size / 8] |= (u8)(data << (8 - BIT_OFFSET));\
	} while (0)

static int samsung_serigo(struct samsung_spi_data data)
{
#ifdef CONFIG_SPI_QUP
	char                tx_buf[32];
	int                 bit_size = 0, i, rc;
	struct spi_message  m;
	struct spi_transfer t;

	if (!lcdc_spi_client) {
		pr_err("%s lcdc_spi_client is NULL\n", __func__);
		return -EINVAL;
	}

	memset(&t, 0, sizeof t);
	memset(tx_buf, 0, sizeof tx_buf);
	t.tx_buf = tx_buf;
	spi_setup(lcdc_spi_client);
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	ADD_BIT(FALSE);
	ADD_BYTE(data.addr);
	for (i = 0; i < data.len; ++i) {
		ADD_BIT(TRUE);
		ADD_BYTE(data.data[i]);
	}

	/* add padding bits so we round to next byte */
	t.len = (bit_size+7) / 8;
	if (t.len <= 4)
		t.bits_per_word = bit_size;

	rc = spi_sync(lcdc_spi_client, &m);
#ifdef DEBUG
	pr_info("%s: addr=0x%02x, #args=%d[%d] [%s], rc=%d\n",
		__func__, data.addr, t.len, t.bits_per_word,
		byte_to_binary(tx_buf, t.len), rc);
#endif
	return rc;
#else
	int i;

	/* Chip Select - low */
	gpio_set_value(spi_cs, 0);
	udelay(2);

	samsung_spi_write_byte(FALSE, data.addr);
	udelay(2);

	for (i = 0; i < data.len; ++i) {
		samsung_spi_write_byte(TRUE, data.data[i]);
		udelay(2);
	}

	/* Chip Select - high */
	gpio_set_value(spi_cs, 1);
#ifdef DEBUG
	pr_info("%s: cmd=0x%02x, #args=%d\n", __func__, data.addr, data.len);
#endif
	return 0;
#endif
}

static int samsung_write_cmd(u8 cmd)
{
#ifdef CONFIG_SPI_QUP
	char                tx_buf[2];
	int                 bit_size = 0, rc;
	struct spi_message  m;
	struct spi_transfer t;

	if (!lcdc_spi_client) {
		pr_err("%s lcdc_spi_client is NULL\n", __func__);
		return -EINVAL;
	}

	memset(&t, 0, sizeof t);
	memset(tx_buf, 0, sizeof tx_buf);
	t.tx_buf = tx_buf;
	spi_setup(lcdc_spi_client);
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	ADD_BIT(FALSE);
	ADD_BYTE(cmd);

	t.len = 2;
	t.bits_per_word = 9;

	rc = spi_sync(lcdc_spi_client, &m);
#ifdef DEBUG
	pr_info("%s: addr=0x%02x, #args=%d[%d] [%s], rc=%d\n",
		__func__, cmd, t.len, t.bits_per_word,
		byte_to_binary(tx_buf, t.len), rc);
#endif
	return rc;
#else
	/* Chip Select - low */
	gpio_set_value(spi_cs, 0);
	udelay(2);

	samsung_spi_write_byte(FALSE, cmd);

	/* Chip Select - high */
	udelay(2);
	gpio_set_value(spi_cs, 1);
#ifdef DEBUG
	pr_info("%s: cmd=0x%02x\n", __func__, cmd);
#endif
	return 0;
#endif
}

static int samsung_serigo_list(struct samsung_spi_data *data, int count)
{
	int i, rc;
	for (i = 0; i < count; ++i, ++data) {
		rc = samsung_serigo(*data);
		if (rc)
			return rc;
#if !defined(CONFIG_MACH_MSM8X60_PRESTO)
		msleep(10);
#endif /* !CONFIG_MACH_MSM8X60_PRESTO */
	}
	return 0;
}

#ifndef CONFIG_SPI_QUP
static void samsung_spi_init(void)
{
	spi_sclk = *(lcdc_samsung_pdata->gpio_num);
	spi_cs   = *(lcdc_samsung_pdata->gpio_num + 1);
	spi_mosi = *(lcdc_samsung_pdata->gpio_num + 2);

	/* Set the output so that we don't disturb the slave device */
	gpio_set_value(spi_sclk, 1);
	gpio_set_value(spi_mosi, 0);

	/* Set the Chip Select deasserted (active low) */
#if !defined(CONFIG_MACH_MSM8X60_PRESTO)
	gpio_set_value(spi_cs, 1);
#endif /* !CONFIG_MACH_MSM8X60_PRESTO */
}
#endif

static void samsung_disp_powerup(void)
{
	if (!samsung_state.disp_powered_up && !samsung_state.display_on)
		samsung_state.disp_powered_up = TRUE;
}

static void pantech_samsung_disp_on(void)
{
	/* 0x01: Software Reset */
	samsung_write_cmd(0x01);

	msleep(10);
	samsung_serigo_list(panel_sequence,
		sizeof(panel_sequence)/sizeof(*panel_sequence));
	samsung_serigo_list(display_sequence,
		sizeof(display_sequence)/sizeof(*display_sequence));

	samsung_serigo_list(etc_sequence,
		sizeof(etc_sequence)/sizeof(*etc_sequence));

	/* 0x11: Sleep Out */
	samsung_write_cmd(0x11);

	msleep(120);
	/* 0x29: Display On */
	samsung_write_cmd(0x29);
}

static void samsung_disp_on(void)
{
	if (samsung_state.disp_powered_up && !samsung_state.display_on) {
		pantech_samsung_disp_on();
		samsung_state.display_on = TRUE;
	}
}

static int lcdc_sam_init_temp(void);
//static int  aaaa= 0;
static int onlyone = 0;
static int lcdc_samsung_panel_on(struct platform_device *pdev)
{
	pr_info("%s\n", __func__);
	if (onlyone != 0) {
		onlyone = 1;
		lcdc_sam_init_temp();
	}
#ifdef CONFIG_F_SKYDISP_BEAM_ON_BUG_FIX
	if (samsung_state.display_on == false) {
#endif
		gpio_set_value(LCD_RESET, GPIO_LOW_VALUE);
		mdelay(6);
		gpio_set_value(LCD_RESET, GPIO_HIGH_VALUE);
		mdelay(12);

		if (!samsung_state.disp_initialized) {
#ifndef CONFIG_SPI_QUP
			lcdc_samsung_pdata->panel_config_gpio(1);
			samsung_spi_init();
#endif
			samsung_disp_powerup();
			samsung_disp_on();
			samsung_state.disp_initialized = TRUE;
#ifdef CONFIG_F_SKYDISP_BEAM_ON_BUG_FIX
		}
#endif
	}
	return 0;
}

static int lcdc_samsung_panel_off(struct platform_device *pdev)
{
	pr_info("%s\n", __func__);
	if (samsung_state.disp_powered_up && samsung_state.display_on) {
		gpio_set_value(157, 0);

		/* 0x10: Sleep In */
		samsung_write_cmd(0x10);
		msleep(120);

		samsung_state.display_on = FALSE;
		samsung_state.disp_initialized = FALSE;

		gpio_set_value(spi_sclk, 1);
		gpio_set_value(spi_mosi, 1);
	}
	return 0;
}

#ifdef CONFIG_SKY_CHARGING
static int old_bl_level = 0;
extern unsigned int sky_charging_status(void);
#endif /* CONFIG_SKY_CHARGING */
#ifdef PANTECH_OLED_BL_CONTROL
static void lcdc_samsung_oled_set_backlight(struct msm_fb_data_type *mfd)
{
	pr_info("%s  bl_level = %d \n", __func__,mfd->bl_level);

	if (mfd->bl_level > 0 &&  old_bl_level == 0) {
		old_bl_level = mfd->bl_level;
		samsung_write_cmd(0x13);	/* 0x13: Normal Mode On */
		msleep(20);
	}

	switch (mfd->bl_level) {
	case 14:
		samsung_serigo_list(gamma_sequence_300,
			sizeof(gamma_sequence_300)/sizeof(*gamma_sequence_300));
		break;
	case 13:
		samsung_serigo_list(gamma_sequence_280,
			sizeof(gamma_sequence_280)/sizeof(*gamma_sequence_280));
		break;
	case 12:
		samsung_serigo_list(gamma_sequence_260,
			sizeof(gamma_sequence_260)/sizeof(*gamma_sequence_260));
		break;
	case 11:
		samsung_serigo_list(gamma_sequence_240,
			sizeof(gamma_sequence_240)/sizeof(*gamma_sequence_240));
		break;
	case 10:
		samsung_serigo_list(gamma_sequence_220,
			sizeof(gamma_sequence_220)/sizeof(*gamma_sequence_220));
		break;
	case 9:
		samsung_serigo_list(gamma_sequence_200,
			sizeof(gamma_sequence_200)/sizeof(*gamma_sequence_200));
		break;
	case 8:
		samsung_serigo_list(gamma_sequence_180,
			sizeof(gamma_sequence_180)/sizeof(*gamma_sequence_180));
		break;
	case 7:
		samsung_serigo_list(gamma_sequence_160,
			sizeof(gamma_sequence_160)/sizeof(*gamma_sequence_160));
		break;
	case 6:
		samsung_serigo_list(gamma_sequence_140,
			sizeof(gamma_sequence_140)/sizeof(*gamma_sequence_140));
		break;
	case 5:
	default:
		samsung_serigo_list(gamma_sequence_120,
			sizeof(gamma_sequence_120)/sizeof(*gamma_sequence_120));
		break;
	case 4:
		samsung_serigo_list(gamma_sequence_100,
			sizeof(gamma_sequence_100)/sizeof(*gamma_sequence_100));
		break;
	case 3:
		samsung_serigo_list(gamma_sequence_80,
			sizeof(gamma_sequence_80)/sizeof(*gamma_sequence_80));
		break;
	case 2:
		samsung_serigo_list(gamma_sequence_60,
			sizeof(gamma_sequence_60)/sizeof(*gamma_sequence_60));
		break;
	case 1:
		samsung_serigo_list(gamma_sequence_40,
			sizeof(gamma_sequence_40)/sizeof(*gamma_sequence_40));
		break;
#ifdef CONFIG_F_SKYDISP_BEAM_ON_BUG_FIX
	case 0:
		//lcdc_samsung_panel_off(mfd->pdev);
		if (old_bl_level != 0) {
			old_bl_level = 0;
			samsung_write_cmd(0x22);	/* 0x22:  pixel off */
			if (sky_charging_status())
				msleep(100);
			else
				msleep(50);
		}
		break;
#endif
	}
}
#endif /* PANTECH_OLED_BL_CONTROL */

#ifdef SYSFS_DEBUG_CMD
static ssize_t samsung_rda_cmd(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = snprintf(buf, PAGE_SIZE, "n/a\n");
	pr_info("%s: 'n/a'\n", __func__);
	return ret;
}

static ssize_t samsung_wta_cmd(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	uint32 cmd;

	sscanf(buf, "%x", &cmd);
	samsung_write_cmd((u8)cmd);

	return ret;
}

static DEVICE_ATTR(cmd, S_IRUGO | S_IWUGO, samsung_rda_cmd, samsung_wta_cmd);
static struct attribute *fs_attrs[] = {
	&dev_attr_cmd.attr,
	NULL,
};
static struct attribute_group fs_attr_group = {
	.attrs = fs_attrs,
};
#endif

static struct msm_fb_panel_data samsung_panel_data = {
	.on = lcdc_samsung_panel_on,
	.off = lcdc_samsung_panel_off,
#ifdef PANTECH_OLED_BL_CONTROL
	.set_backlight = lcdc_samsung_oled_set_backlight,
#endif
};

static int __devinit samsung_probe(struct platform_device *pdev)
{
	struct msm_panel_info *pinfo;
#ifdef SYSFS_DEBUG_CMD
	struct platform_device *fb_dev;
	struct msm_fb_data_type *mfd;
	int rc;
#endif

#if defined(CONFIG_MACH_MSM8X60_PRESTO) || defined(CONFIG_MACH_MSM8X60_QUANTINA)  // panel_init for PRESTO
	lcdc_gpio_init(1);
#endif /* CONFIG_MACH_MSM8X60_PRESTO || CONFIG_MACH_MSM8X60_QUANTINA */

	pr_info("%s: id=%d\n", __func__, pdev->id);
	lcdc_samsung_pdata = pdev->dev.platform_data;

	pinfo = &samsung_panel_data.panel_info;
	pinfo->xres = 480;
	pinfo->yres = 800;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 24;
	pinfo->fb_num = 2;
#if defined(PANTECH_OLED_BL_CONTROL)
	pinfo->clk_rate = 24000000; /* Max 27.77MHz */
	pinfo->bl_max = 10; //12 -> 6  for power saving
#else /* PANTECH_OLED_BL_CONTROL */
	pinfo->clk_rate = 25600000; /* Max 27.77MHz */
	pinfo->bl_max = 15;
#endif /* PANTECH_OLED_BL_CONTROL */
	pinfo->bl_min = 1;

	/* AMS367PE02 Operation Manual, Page 7 */
	pinfo->lcdc.h_back_porch = 16-2;	/* HBP-HLW */
	pinfo->lcdc.h_front_porch = 16;
	pinfo->lcdc.h_pulse_width = 2;
	/* AMS367PE02 Operation Manual, Page 6 */
	pinfo->lcdc.v_back_porch = 3-2;		/* VBP-VLW */
	pinfo->lcdc.v_front_porch = 28;
	pinfo->lcdc.v_pulse_width = 2;

	pinfo->lcdc.border_clr = 0;
	pinfo->lcdc.underflow_clr = 0xff;
	pinfo->lcdc.hsync_skew = 0;
	pdev->dev.platform_data = &samsung_panel_data;

#ifndef SYSFS_DEBUG_CMD
	msm_fb_add_device(pdev);
#else
	fb_dev = msm_fb_add_device(pdev);
	mfd = platform_get_drvdata(fb_dev);
	rc = sysfs_create_group(&mfd->fbi->dev->kobj, &fs_attr_group);
	if (rc) {
		pr_err("%s: sysfs group creation failed, rc=%d\n", __func__,
			rc);
		return rc;
	}
#endif
	return 0;
}

#ifdef CONFIG_SPI_QUP
static int __devinit lcdc_samsung_spi_probe(struct spi_device *spi)
{
	pr_info("%s\n", __func__);
	lcdc_spi_client = spi;
	lcdc_spi_client->bits_per_word = 32;
	return 0;
}
static int __devexit lcdc_samsung_spi_remove(struct spi_device *spi)
{
	lcdc_spi_client = NULL;
	return 0;
}
static struct spi_driver lcdc_samsung_spi_driver = {
	.driver.name   = LCDC_SAMSUNG_SPI_DEVICE_NAME,
	.driver.owner  = THIS_MODULE,
	.probe         = lcdc_samsung_spi_probe,
	.remove        = __devexit_p(lcdc_samsung_spi_remove),
};
#endif

static struct platform_driver this_driver = {
	.probe		= samsung_probe,
	.driver.name	= "lcdc_samsung_oled",
};

static int __init lcdc_samsung_panel_init(void)
{
	int ret;
	ret = platform_driver_register(&this_driver);
	if (ret) {
		pr_err("%s: driver register failed, rc=%d\n", __func__, ret);
		return ret;
	}
	return ret;
}

static int lcdc_sam_init_temp(void)
{
	int ret = 0;
	pr_info("%s\n", __func__);
	lcdc_gpio_init(1);
	mdelay(5);

#ifdef CONFIG_SPI_QUP
	ret = spi_register_driver(&lcdc_samsung_spi_driver);

	if (ret) {
		pr_err("%s: spi register failed: rc=%d\n", __func__, ret);
		platform_driver_unregister(&this_driver);
	} else
		pr_info("%s: SUCCESS (SPI)\n", __func__);
#else
	pr_info("%s: SUCCESS (BitBang)\n", __func__);
#endif
	return ret;
}
module_init(lcdc_samsung_panel_init);
static void __exit lcdc_samsung_panel_exit(void)
{
	pr_info("%s\n", __func__);
#ifdef CONFIG_SPI_QUP
	spi_unregister_driver(&lcdc_samsung_spi_driver);
#endif
	platform_driver_unregister(&this_driver);
}
module_exit(lcdc_samsung_panel_exit);