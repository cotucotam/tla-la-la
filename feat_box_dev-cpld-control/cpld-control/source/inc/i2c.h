/**
 * Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __I2C_H_
#define __I2C_H_

#include <mpsse.h>
#include <stdio.h>

#if LIBFTDI1 == 1
#include <unistd.h>
#endif

#define SDA 7
#define SCL 6

#define PIN_SDA (0x01 << SDA)
#define PIN_SCL (0x01 << SCL)

#define ACK 0
#define NAK 1

#define TIME 100

void i2c_delay(void);

void i2c_init(struct mpsse_context *mpsse);

uint8_t i2c_read_sda(struct mpsse_context *mpsse);
uint8_t i2c_read_scl(struct mpsse_context *mpsse);
void i2c_clear_sda(struct mpsse_context *mpsse);
void i2c_clear_scl(struct mpsse_context *mpsse);

void i2c_start(struct mpsse_context *mpsse);
void i2c_stop(struct mpsse_context *mpsse);

void i2c_write_bit(struct mpsse_context *mpsse, uint8_t bit);
uint8_t i2c_read_bit(struct mpsse_context *mpsse);

uint8_t i2c_write_byte(struct mpsse_context *mpsse, uint8_t byte);
uint8_t i2c_read_byte(struct mpsse_context *mpsse, uint8_t ack);

uint8_t i2c_write_data(struct mpsse_context *mpsse, uint8_t device_address,
		       uint64_t address, uint8_t addr_length,
		       uint8_t *value, uint8_t val_length);
uint8_t i2c_read_data(struct mpsse_context *mpsse, uint8_t device_address,
		      uint64_t address, uint8_t addr_length,
		      uint8_t *value, uint8_t val_length);

#endif /* __I2C_H_ */
