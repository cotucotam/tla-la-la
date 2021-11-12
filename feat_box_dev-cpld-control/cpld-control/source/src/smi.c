/**
 * Copyright (C) 2020-2021 Renesas Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "smi.h"

struct smi_bufer {
	uint32_t data;
	uint8_t length;
};

/**
 * Initialize SMI protocol.
 *
 * @param	mpsse	MPSSE structure.
 *
 * @return	MPSSE_OK on success.
 *		MPSSE_FAIL on failure.
 */
int smi_init(struct mpsse_context *mpsse)
{
	int ret;
	uint8_t dat;

	/* Setup MDC, MDO as output */
	mpsse->bitbang = PIN_MDC | PIN_MDO;
	ftdi_set_bitmode(&(mpsse->ftdi), mpsse->bitbang, BITMODE_SYNCBB);
	ftdi_set_baudrate(&(mpsse->ftdi), 3000000);

	/* Set MDC, MDO to low for safe pattern */
	dat = PIN_MDC | PIN_MDO;

	ret = Write(mpsse, (char *)&dat, 1);
	if (ret == MPSSE_FAIL) {
		fprintf(stderr, "SMI: send data failed!\n");
		return ret;
	}

	return MPSSE_OK;
}

/**
 * Generate data to read register
 *
 * @param	buffer	buffer to storing data.
 * @param	address	register needs to read.
 *
 * @return	None.
 */
void smi_generate_read(uint8_t *buffer, uint16_t address)
{
	int8_t i, pos, bit;
	uint16_t idx = 0;
	struct smi_bufer buf[8] = {
		{ 0x01, 1 },	    // begin
		{ 0xFFFFFFFF, 32 }, // preamble
		{ 0x01, 2 },	    // start
		{ 0x02, 2 },	    // read
		{ address, 10 },    // address
		{ 0x00, 2 },	    // turnaround
		{ 0x00, 16 },	    // data
		{ 0x01, 1 }	    // end
	};

	for (i = 0; i < 8; i++) {
		for (pos = buf[i].length - 1; pos >= 0; pos--) {
			bit = (buf[i].data & (1 << pos)) ? PIN_MDO : 0;
			*(buffer + (2 * idx) + 0) = bit;
			*(buffer + (2 * idx) + 1) = bit | PIN_MDC;
			idx++;
		}
	}
}

/**
 * Generate data to write register
 *
 * @param	buffer	buffer to storing data.
 * @param	address	register needs to write.
 * @param	value	value to write.
 *
 * @return	None.
 */
void smi_generate_write(uint8_t *buffer, uint16_t address, uint16_t *value)
{
	int8_t i, pos, bit;
	uint16_t idx = 0;
	struct smi_bufer buf[8] = {
		{ 0x01, 1 },	    // begin
		{ 0xFFFFFFFF, 32 }, // preamble
		{ 0x01, 2 },	    // start
		{ 0x01, 2 },	    // write
		{ address, 10 },    // address
		{ 0x02, 2 },	    // turnaround
		{ *value, 16 },	    // data
		{ 0x01, 1 }	    // end
	};

	for (i = 0; i < 8; i++) {
		for (pos = buf[i].length - 1; pos >= 0; pos--) {
			bit = (buf[i].data & (1 << pos)) ? PIN_MDO : 0;
			*(buffer + (2 * idx) + 0) = bit;
			*(buffer + (2 * idx) + 1) = bit | PIN_MDC;
			idx++;
		}
	}
}

/**
 * Read n bytes data.
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
int smi_read(struct mpsse_context *mpsse,
	     uint64_t address,
	     uint8_t addr_length,
	     uint8_t *value,
	     uint8_t val_length)
{
	int ret = 0;
	uint8_t i, j, pos;
	uint8_t data[132];
	uint16_t idx;

	// read data to clean buffer
	while ((ret = ftdi_read_data(&mpsse->ftdi, data, 132)) > 0)
		;

	if (ret < 0) {
		fprintf(stderr, "SMI: clean buffer error (ret = %d)\n", ret);
		return ret;
	}

	for (i = 0; i < val_length / 2; i++) {

		// prepare data
		smi_generate_read(data, address + i);

		// send data
		ret = ftdi_write_data(&mpsse->ftdi, data, 132);
		if (ret < 0) {
			fprintf(stderr, "SMI: send data failed (ret = %d)!\n", ret);
			return ret;
		}

		// receive data
		ret = ftdi_read_data(&mpsse->ftdi, data, 132);
		if (ret < 0) {
			fprintf(stderr, "SMI: read data failed (ret = %d)!\n", ret);
			return ret;
		}

		for (j = 0; j < 16; j++) {
			idx = (50 + j) * 2;
			pos = i * 2 + 1 - (j / 8);
			*(value + pos) = (*(value + pos) << 1) | !!(data[idx] & PIN_MDI);
		}
	}

	return MPSSE_OK;
}

/**
 * Write n bytes data
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
int smi_write(struct mpsse_context *mpsse,
	      uint64_t address,
	      uint8_t addr_length,
	      uint8_t *value,
	      uint8_t val_length)
{
	int i, ret = 0;
	uint8_t data[132];

	for (i = 0; i < val_length / 2; i++) {
		// prepare for data
		smi_generate_write(data, address + i, (uint16_t *)(value + i * 2));

		// send data
		ret = ftdi_write_data(&mpsse->ftdi, data, 132);
		if (ret < 0) {
			fprintf(stderr, "SMI: write data failed (ret = %d)!\n", ret);
			return ret;
		}
	}

	return MPSSE_OK;
}
