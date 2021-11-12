/**
 * Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "i2c.h"

void i2c_delay(void)
{
	int i;

	for (i = 0; i < TIME / 2; ++i)
		;
}

/**
 * Initialize I2C protocol.
 *
 * @param	mpsse	MPSSE structure.
 *
 * @return	None
 */
void i2c_init(struct mpsse_context *mpsse)
{
	mpsse->bitbang = PIN_SCL | PIN_SDA;
	SetDirection(mpsse, mpsse->bitbang);
	usleep(1000);
}

/**
 * Set SDA as input.
 *
 * @param	mpsse	MPSSE structure.
 *
 * @return	Logic level of SDA, normal is HIGH due to pullup resistor.
 */
uint8_t i2c_read_sda(struct mpsse_context *mpsse)
{
	mpsse->bitbang &= ~PIN_SDA;
	SetDirection(mpsse, mpsse->bitbang);
	return PinState(mpsse, SDA, -1);
}

/**
 * Set SCL as input.
 *
 * @param	mpsse	MPSSE structure.
 *
 * @return	Logic level of SCL, normal is HIGH due to pullup resistor.
 */
uint8_t i2c_read_scl(struct mpsse_context *mpsse)
{
	mpsse->bitbang &= ~PIN_SCL;
	SetDirection(mpsse, mpsse->bitbang);
	return PinState(mpsse, SCL, -1);
}

/**
 * Set SDA as output, clear SDA to LOW.
 *
 * @param	mpsse	MPSSE structure.
 *
 * @return	None.
 */
void i2c_clear_sda(struct mpsse_context *mpsse)
{
	mpsse->bitbang |= PIN_SDA;
	SetDirection(mpsse, mpsse->bitbang);
}

/**
 * Set SCL as output, clear SCL to LOW.
 *
 * @param	mpsse	MPSSE structure.
 *
 * @return	None.
 */
void i2c_clear_scl(struct mpsse_context *mpsse)
{
	mpsse->bitbang |= PIN_SCL;
	SetDirection(mpsse, mpsse->bitbang);
}

/**
 * Send a start signal.
 *
 * @param	mpsse	MPSSE structure.
 *
 * @return	None.
 */
void i2c_start(struct mpsse_context *mpsse)
{
	i2c_read_scl(mpsse);
	while (i2c_read_scl(mpsse) == 0)
		;

	i2c_delay();
	i2c_read_sda(mpsse);
	i2c_delay();

	i2c_clear_sda(mpsse);
	i2c_delay();
	i2c_clear_scl(mpsse);
	i2c_delay();
}

/**
 * Send a stop signal.
 *
 * @param	mpsse	MPSSE structure.
 *
 * @return	None.
 */
void i2c_stop(struct mpsse_context *mpsse)
{
	i2c_clear_sda(mpsse);
	i2c_delay();

	i2c_read_scl(mpsse);
	while (i2c_read_scl(mpsse) == 0)
		;
	i2c_delay();

	i2c_read_sda(mpsse);
	i2c_delay();
}

/**
 * Write 1 bit to slave.
 *
 * @param	mpsse	MPSSE structure.
 * @param	bit	Bit to write.
 *
 * @return	None.
 */
void i2c_write_bit(struct mpsse_context *mpsse, uint8_t bit)
{
	if (bit)
		i2c_read_sda(mpsse);
	else
		i2c_clear_sda(mpsse);

	i2c_delay();

	i2c_read_scl(mpsse);
	i2c_delay();
	while (i2c_read_scl(mpsse) == 0)
		;

	i2c_clear_scl(mpsse);
}

/**
 * Read 1 bit from slave.
 *
 * @param	mpsse	MPSSE structure.
 *
 * @return	Read bit.
 */
uint8_t i2c_read_bit(struct mpsse_context *mpsse)
{
	uint8_t bit;

	i2c_read_sda(mpsse);
	i2c_delay();

	i2c_read_scl(mpsse);
	while (i2c_read_scl(mpsse) == 0)
		;
	i2c_delay();

	bit = i2c_read_sda(mpsse);
	i2c_clear_scl(mpsse);

	return bit;
}

/**
 * Write 1 byte to slave.
 *
 * @param	mpsse	MPSSE structure.
 * @param	byte	Byte to write.
 *
 * @return	ACK or NAK signal from slave.
 */
uint8_t i2c_write_byte(struct mpsse_context *mpsse, uint8_t byte)
{
	uint8_t ack, index;

	for (index = 0; index < 8; ++index) {
		i2c_write_bit(mpsse, (byte & 0x80) != 0);
		byte <<= 1;
	}

	ack = i2c_read_bit(mpsse);

	return ack;
}

/**
 * Read 1 byte from slave.
 *
 * @param   mpsse   MPSSE structure.
 * @param   ack     Set ACK/NAK bit after read 1 byte, NAK when send last byte.
 *
 * @return	The read byte from slave.
 */
uint8_t i2c_read_byte(struct mpsse_context *mpsse, uint8_t ack)
{
	uint8_t byte = 0x00, index;

	for (index = 0; index < 8; ++index)
		byte = (byte << 1) | i2c_read_bit(mpsse);

	i2c_write_bit(mpsse, ack);

	return byte;
}

/**
 * Write n bytes data to slave.
 *
 * @param	mpsse		MPSSE structure.
 * @param	device_address	Device address.
 * @param	address		Register address.
 * @param	addr_length	Register address length.
 * @param	value		Value need to write.
 * @param	val_length	Number of bytes need to write.
 *
 * @return	Number of NACKs from slave.
 */
uint8_t i2c_write_data(struct mpsse_context *mpsse,
		       uint8_t device_address,
		       uint64_t address,
		       uint8_t addr_length,
		       uint8_t *value,
		       uint8_t val_length)
{
	int index;
	uint8_t ret = 0;

	i2c_start(mpsse);

	ret += i2c_write_byte(mpsse, device_address & 0xfe);

	for (index = addr_length - 1; index >= 0; --index)
		ret += i2c_write_byte(mpsse, (address >> (8 * index)) & 0xFF);

	for (index = 0; index < val_length; ++index)
		ret += i2c_write_byte(mpsse, *(value + index));

	i2c_stop(mpsse);

	if (ret != 0)
		fprintf(stderr, "NACK: %d\n", ret);
	return ret;
}

/**
 * Read n bytes data from slave.
 *
 * @param	mpsse		MPSSE structure.
 * @param	device_address	Device address.
 * @param	address		Register address.
 * @param	addr_length	Register address length.
 * @param	value		Read value.
 * @param	val_length	Number of bytes need to read.
 *
 * @return	Number of NACKs from slave.
 */
uint8_t i2c_read_data(struct mpsse_context *mpsse,
		      uint8_t device_address,
		      uint64_t address,
		      uint8_t addr_length,
		      uint8_t *value,
		      uint8_t val_length)
{
	int index;
	uint8_t ret = 0, ack;
	i2c_start(mpsse);
	ret += i2c_write_byte(mpsse, device_address & 0xfe);
	for (index = addr_length - 1; index >= 0; --index)
		ret += i2c_write_byte(mpsse, (address >> (8 * index)) & 0xFF);
	i2c_start(mpsse);
	ret += i2c_write_byte(mpsse, device_address | 0x01);
	for (index = 0; index < val_length; ++index) {
		if (index + 1 == val_length)
			ack = NAK;
		else
			ack = ACK;
		*(value + index) = i2c_read_byte(mpsse, ack);
	}
	i2c_stop(mpsse);
	if (ret != 0)
		fprintf(stderr, "NACK: %d\n", ret);
	return ret;
}
