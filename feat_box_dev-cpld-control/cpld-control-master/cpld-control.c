// SPDX-License-Identifier: GPL-2.0+
/*
 * ULCB board CPLD access support
 *
 * Note that the CPLD configuration survives reset by button or JTAG.
 * The CPLD configuration does not survive power cycle, which is nice.
 *
 * Copyright (C) 2019 Marek Vasut <marek.vasut+renesas@gmail.com>
 *
 * Note that the FTDI USB-serial converter driver will be unbound from
 * the FTDI device, it has to be manually re-bound via sysfs. Example:
 * $ ls /sys/bus/usb/drivers/ftdi_sio/
 *   5-1.2.3:1.0/ bind         module/      uevent       unbind
 * $ echo "5-1.2.3:1.0" > /sys/bus/usb/drivers/ftdi_sio/bind
 */
#include <errno.h>
#include <ftdi.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define PIN_TX  0x01
#define PIX_RX  0x02
#define PIN_RTS 0x04
#define PIN_CTS 0x08
#define PIN_DTR 0x10
#define PIN_DSR 0x20
#define PIN_DCD 0x40
#define PIN_RI  0x80

#define PIN_MOSI	PIN_DCD
#define PIN_MISO	PIN_DTR
#define PIN_SCK		PIN_RTS
#define PIN_SSTBZ	PIN_CTS

#define CPLD_ADDR_MODE		0x00 /* RW */
#define CPLD_ADDR_MUX		0x02 /* RW */
#define CPLD_ADDR_DIPSW6	0x08 /* R */
#define CPLD_ADDR_RESET		0x80 /* RW */
#define CPLD_ADDR_VERSION	0xFF /* R */

static struct ftdi_context ftdi;

static void cpld_sync(void)
{
	uint8_t dat[2 * 32];
	uint8_t cmd[(2 * 8) + 2];
	uint8_t addr = 0xfe;
	uint8_t bit;
	int i, ret;

	/* Do this to somehow synchronize the CPLD and let it communicate. */
	for (i = 0; i < 32; i++) {
		dat[(2 * i) + 0] = PIN_SSTBZ | PIN_SCK;
		dat[(2 * i) + 1] = PIN_SSTBZ;
	}

	dat[(2 * 31) + 0] = PIN_MOSI | PIN_SSTBZ | PIN_SCK;
	dat[(2 * 31) + 1] = PIN_MOSI | PIN_SSTBZ;

	ret = ftdi_write_data(&ftdi, dat, sizeof(dat));
	if (ret < 0)
		exit(ret);

	/* Wait a bit after writing data, otherwise bad things happen */
	usleep(100);

	for (i = 0; i < 8; i++) {
		bit = (addr & (1 << 7)) ? PIN_MOSI : 0;
		cmd[(2 * i) + 0] = bit | PIN_SSTBZ | PIN_SCK;
		cmd[(2 * i) + 1] = bit | PIN_SSTBZ;
		addr <<= 1;
	}

	cmd[(2 * 8) + 0] = PIN_MOSI | PIN_SCK;
	cmd[(2 * 8) + 1] = PIN_MOSI;

	ret = ftdi_write_data(&ftdi, cmd, sizeof(cmd));
	if (ret < 0)
		exit(ret);

	/* Wait a bit after writing data, otherwise bad things happen */
	usleep(100);
}

static uint32_t cpld_read(uint8_t addr)
{
	uint8_t cmd[(2 * 8) + 2];
	uint8_t bit;
	uint8_t pin_data;
	uint32_t data = 0;
	int i, ret;

	for (i = 0; i < 8; i++) {
		bit = (addr & (1 << 7)) ? PIN_MOSI : 0;
		cmd[(2 * i) + 0] = bit | PIN_SSTBZ | PIN_SCK;
		cmd[(2 * i) + 1] = bit | PIN_SSTBZ;
		addr <<= 1;
	}

	cmd[(2 * 8) + 0] = PIN_SCK;
	cmd[(2 * 8) + 1] = 0;

	ret = ftdi_write_data(&ftdi, cmd, sizeof(cmd));
	if (ret < 0)
		exit(ret);

	/* Wait a bit after writing data, otherwise bad things happen */
	usleep(100);

	/* Read data */
	for (i = 0; i < 32; i++) {
		cmd[(2 * 0) + 0] = PIN_SSTBZ | PIN_SCK;
		cmd[(2 * 0) + 1] = PIN_SSTBZ;
		ret = ftdi_write_data(&ftdi, cmd, 2);
		if (ret < 0)
			exit(ret);

		usleep(100);

		ret = ftdi_read_pins(&ftdi, &pin_data);
		if (ret < 0)
			exit(ret);

		data = (data << 1) | !!(pin_data & PIN_MISO);
	}

	return data;
}

static void cpld_write(uint8_t addr, uint32_t data)
{
	uint8_t cmd[(2 * 8) + 2];
	uint8_t dat[2 * 32];
	uint8_t bit;
	int i, ret;

	for (i = 0; i < 32; i++) {
		bit = (data & (1 << 31)) ? PIN_MOSI : 0;
		dat[(2 * i) + 0] = bit | PIN_SSTBZ | PIN_SCK;
		dat[(2 * i) + 1] = bit | PIN_SSTBZ;
		data <<= 1;
	}

	ret = ftdi_write_data(&ftdi, dat, sizeof(dat));
	if (ret < 0)
		exit(ret);

	/* Wait a bit after writing data, otherwise bad things happen */
	usleep(100);

	for (i = 0; i < 8; i++) {
		bit = (addr & (1 << 7)) ? PIN_MOSI : 0;
		cmd[(2 * i) + 0] = bit | PIN_SSTBZ | PIN_SCK;
		cmd[(2 * i) + 1] = bit | PIN_SSTBZ;
		addr <<= 1;
	}

	cmd[(2 * 8) + 0] = PIN_MOSI | PIN_SCK;
	cmd[(2 * 8) + 1] = PIN_MOSI;

	ret = ftdi_write_data(&ftdi, cmd, sizeof(cmd));
	if (ret < 0)
		exit(ret);

	/* Wait a bit after writing data, otherwise bad things happen */
	usleep(100);
}

static void cpld_dump(void)
{
	printf("CPLD version:\t\t\t0x%02x: 0x%08x\n",
	       CPLD_ADDR_VERSION, cpld_read(CPLD_ADDR_VERSION));
	printf("Mode setting (MD0..28):\t\t0x%02x: 0x%08x\n",
	       CPLD_ADDR_MODE, cpld_read(CPLD_ADDR_MODE));
	printf("Multiplexer settings:\t\t0x%02x: 0x%08x\n",
	       CPLD_ADDR_MUX, cpld_read(CPLD_ADDR_MUX));
	printf("DIPSW (SW6):\t\t\t0x%02x: 0x%08x\n",
	       CPLD_ADDR_DIPSW6, cpld_read(CPLD_ADDR_DIPSW6));
}

static int cpld_list(void)
{
	struct ftdi_device_list *list, *dev;
	char ser[32];
	int ret;

	ret = ftdi_usb_find_all(&ftdi, &list, 0x0403, 0x6001);
	if (ret < 0) {
		printf("Failed to list devices.\n");
		return ret;
	}

	for (dev = list; dev != NULL; dev = dev->next) {
		ret = ftdi_usb_get_strings(&ftdi, dev->dev, NULL, 0, NULL, 0,
					   ser, 32);
		if (ret < 0) {
			printf("Skipping device.\n");
			continue;
		}
		printf("Serial: %s\n", ser);
	}

	return 0;
}

static void cpld_help(char *pn)
{
	printf("CPLD control\n");
	printf("%s [-h] ............................... Print this help.\n", pn);
	printf("%s -l ................................. List available devices.\n", pn);
	printf("%s -d <FTDI iSerial> .................. Dump CPLD registers.\n", pn);
	printf("%s -w <FTDI iSerial> [<reg> <val>]* ... Write CPLD register(s).\n", pn);
	printf("      *One or more [<reg> <val>] pairs can be specified.\n", pn);
}

int main(int argc, char *argv[])
{
	int i, ret;

	ftdi_init(&ftdi);

	if ((argc < 2) ||
	    (argc == 2 && !strcmp(argv[1], "-h"))) {
		cpld_help(argv[0]);
		return 0;
	}

	if (argc == 2 && !strcmp(argv[1], "-l")) {
		ret = cpld_list();
		ftdi_deinit(&ftdi);
		return ret;
	}

	if (argc > 2 && !strcmp(argv[1], "-l")) {
		printf("The -l option takes no arguments!\n\n");
		cpld_help(argv[0]);
		return -EINVAL;
	}

	if (argc >= 2 && strcmp(argv[1], "-d") && strcmp(argv[1], "-w")) {
		printf("Unknown option!\n\n");
		cpld_help(argv[0]);
		return -EINVAL;
	}

	if (argc != 3 && !strcmp(argv[1], "-d")) {
		printf("The -d option takes exactly one argument!\n\n");
		cpld_help(argv[0]);
		return -EINVAL;
	}

	if (((argc < 5) || (((argc - 5) % 2) != 0)) && !strcmp(argv[1], "-w")) {
		printf("The -w option takes one iSerial argument and at least one reg/val pair!\n\n");
		cpld_help(argv[0]);
		return -EINVAL;
	}

	printf("Using device with iSerial: %s\n\n", argv[2]);

	ret = ftdi_usb_open_desc(&ftdi, 0x0403, 0x6001, NULL, argv[2]);
	if (ret < 0) {
		printf("Can't open device (ret=%i).\n", ret);
		exit(ret);
	}

	ftdi_usb_reset(&ftdi);
	ftdi_usb_purge_buffers(&ftdi);
	ftdi_set_event_char(&ftdi, 0, 0);
	ftdi_set_error_char(&ftdi, 0, 0);
	/* 57600 works, lower baudrates cause data corruption */
	ftdi_set_baudrate(&ftdi, 57600);
	ftdi_set_bitmode(&ftdi, PIN_MOSI | PIN_SCK | PIN_SSTBZ,
			 BITMODE_BITBANG);

	cpld_sync();

	/* Dump registers */
	if (argc == 3 && !strcmp(argv[1], "-d"))
		cpld_dump();

	/* Write registers */
	if (argc >= 5 && (((argc - 5) % 2) == 0) && !strcmp(argv[1], "-w")) {
		for (i = 3; i < argc; i += 2) {
			char *endptr;
			long int reg = strtol(argv[i], &endptr, 16);
			uint32_t val = strtoll(argv[i + 1], &endptr, 16);

			printf("Writing register 0x%02lx with value 0x%08lx\n",
				reg, val);

			cpld_write(reg, val);
		}
		cpld_dump();
	}

	ftdi_set_bitmode(&ftdi, PIN_MOSI | PIN_SCK | PIN_SSTBZ,
			 BITMODE_RESET);
	ftdi_disable_bitbang(&ftdi);
	ftdi_usb_close(&ftdi);
	ftdi_deinit(&ftdi);

	return 0;
}
