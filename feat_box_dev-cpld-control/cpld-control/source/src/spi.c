/**
 * Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "spi.h"

/**
 * Initialize SPI protocol.
 *
 * @param	mpsse	MPSSE structure.
 *
 * @return	MPSSE_OK on success.
 *		MPSSE_FAIL on failure.
 */
int spi_init(struct mpsse_context *mpsse)
{
	int i, ret;
	uint8_t bit;
	uint8_t addr = 0xfe;
	uint8_t dat[2 * 32];
	uint8_t cmd[(2 * 8) + 2];

	/* Setup MOSI, SCK, SSTBZ as output */
	mpsse->bitbang = PIN_MOSI | PIN_SCK | PIN_SSTBZ;
	SetDirection(mpsse, mpsse->bitbang);
	ftdi_set_baudrate(&(mpsse->ftdi), 57600);

	/* Do this to somehow synchronize the CPLD and let it communicate. */
	for (i = 0; i < 32; i++) {
		dat[(2 * i) + 0] = PIN_SSTBZ | PIN_SCK;
		dat[(2 * i) + 1] = PIN_SSTBZ;
	}

	dat[(2 * 31) + 0] = PIN_MOSI | PIN_SSTBZ | PIN_SCK;
	dat[(2 * 31) + 1] = PIN_MOSI | PIN_SSTBZ;

	ret = Write(mpsse, (char *)dat, sizeof(dat));
	if (ret == MPSSE_FAIL) {
		fprintf(stderr, "SPI: send data failed!\n");
		return ret;
	}

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

	ret = Write(mpsse, (char *)cmd, sizeof(cmd));
	if (ret == MPSSE_FAIL) {
		fprintf(stderr, "SPI: send command failed!\n");
		return ret;
	}

	/* Wait a bit after writing data, otherwise bad things happen */
	usleep(100);

	return ret;
}

/**
 * Read n bytes data from slave.
 *
 * @param	mpsse		MPSSE structure.
 * @param	address		Register address.
 * @param	addr_length	Register address length.
 * @param	value		Read value.
 * @param	val_length	Number of bytes need to read.
 *
 * @return	MPSSE_OK on success.
 *		MPSSE_FAIL on failure.
 */
int spi_read(struct mpsse_context *mpsse,
	     uint64_t address,
	     uint8_t addr_length,
	     uint8_t *value,
	     uint8_t val_length)
{
	int i, j, ret;
	uint8_t bit;
	uint8_t pin_data;
	uint8_t cmd[(2 * 8 * addr_length) + 2];

	for (i = 0; i < 8 * addr_length; i++) {
		bit = (address & (1 << (8 * addr_length - 1))) ? PIN_MOSI : 0;
		cmd[(2 * i) + 0] = bit | PIN_SSTBZ | PIN_SCK;
		cmd[(2 * i) + 1] = bit | PIN_SSTBZ;
		address <<= 1;
	}

	cmd[(2 * 8 * addr_length) + 0] = PIN_SCK;
	cmd[(2 * 8 * addr_length) + 1] = 0;

	ret = Write(mpsse, (char *)cmd, sizeof(cmd));
	if (ret == MPSSE_FAIL) {
		fprintf(stderr, "SPI: send command failed!\n");
		return ret;
	}

	/* Wait a bit after writing data, otherwise bad things happen */
	usleep(100);

	/* Read data */
	for (i = val_length - 1; i > -1; i--) {
		for (j = 0; j < 8; j++) {
			cmd[(2 * 0) + 0] = PIN_SSTBZ | PIN_SCK;
			cmd[(2 * 0) + 1] = PIN_SSTBZ;
			ret = Write(mpsse, (char *)cmd, 2);
			if (ret == MPSSE_FAIL) {
				fprintf(stderr, "SPI: send command failed!\n");
				return ret;
			}

			/* Wait a bit after writing data, otherwise bad things happen */
			usleep(100);

			pin_data = ReadPins(mpsse);
			*(value + i) = (*(value + i) << 1) | !!(pin_data & PIN_MISO);
		}
	}

	return ret;
}

/**
 * Write n bytes data to slave.
 *
 * @param	mpsse		MPSSE structure.
 * @param	address		Register address.
 * @param	addr_length	Register address length.
 * @param	value		Value need to write.
 * @param	val_length	Number of bytes need to write.
 *
 * @return	MPSSE_OK on success.
 *		MPSSE_FAIL on failure.
 */
int spi_write(struct mpsse_context *mpsse,
	      uint64_t address,
	      uint8_t addr_length,
	      uint8_t *value,
	      uint8_t val_length)
{
	int i, j, ret;
	uint8_t *dat = NULL;
	uint8_t bit, idx_bit = 0;
	uint8_t cmd[(2 * 8 * addr_length) + 2];

	dat = (uint8_t *)malloc(2 * val_length * 8 * sizeof(uint8_t));

	for (i = val_length - 1; i > -1; i--) {
		for (j = 0; j < 8; j++) {
			bit = (*(value + i) & (1 << 7)) ? PIN_MOSI : 0;
			*(dat + (2 * idx_bit) + 0) = bit | PIN_SSTBZ | PIN_SCK;
			*(dat + (2 * idx_bit) + 1) = bit | PIN_SSTBZ;
			*(value + i) = *(value + i) << 1;
			idx_bit++;
		}
	}

	ret = Write(mpsse, (char *)dat, 2 * val_length * 8);
	if (ret == MPSSE_FAIL) {
		fprintf(stderr, "SPI: send data failed!\n");
		free(dat);
		return ret;
	}

	/* Wait a bit after writing data, otherwise bad things happen */
	usleep(100);

	for (i = 0; i < 8 * addr_length; i++) {
		bit = (address & (1 << (8 * addr_length - 1))) ? PIN_MOSI : 0;
		cmd[(2 * i) + 0] = bit | PIN_SSTBZ | PIN_SCK;
		cmd[(2 * i) + 1] = bit | PIN_SSTBZ;
		address <<= 1;
	}

	cmd[(2 * 8 * addr_length) + 0] = PIN_MOSI | PIN_SCK;
	cmd[(2 * 8 * addr_length) + 1] = PIN_MOSI;

	ret = Write(mpsse, (char *)cmd, sizeof(cmd));
	if (ret == MPSSE_FAIL) {
		fprintf(stderr, "SPI: send command failed!\n");
		free(dat);
		return ret;
	}

	/* Wait a bit after writing data, otherwise bad things happen */
	usleep(100);

	free(dat);
	return ret;
}
