/**
 * Copyright (C) 2020-2021 Renesas Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __CPLD_H_
#define __CPLD_H_

#include "i2c.h"
#include "spi.h"
#include "smi.h"
#include <libusb-1.0/libusb.h>

#define VENDOR 0x0403

#define NUM_PRODUCT 4
#define FT232R	    0x6001
#define FT2232	    0x6010
#define FT4232	    0x6011
#define FT232H	    0x6014

#define CPLD_SLAVE_ADDR 0xE0

enum register_mode {
	RW = 0U,
	R = 1U,
	W = 2U
};

enum protocol {
	SPI = 0U,
	IIC = 1U,
	SMI = 2U
};

struct register_context {
	char *name;
	uint64_t address;
	uint8_t addr_length;
	uint64_t value;
	uint8_t val_length;
	enum register_mode mode;
	struct register_context *pnext;
};

struct cpld_context {
	struct mpsse_context *mpsse;
	struct register_context *reg;
	char *board_name;
	uint16_t product_id;
	enum protocol protocol;
};

struct cpld_context *cpld_init(char *board, char *serial);
uint8_t cpld_list(void);
uint8_t cpld_change_serial(struct cpld_context *cpld, char *new_serial);
uint8_t cpld_read(struct cpld_context *cpld, uint64_t address);
uint8_t cpld_write(struct cpld_context *cpld, uint64_t address, uint8_t *value);
uint8_t cpld_write_nonvolatile(struct cpld_context *cpld, uint64_t address, uint8_t *value);
uint8_t cpld_dump(struct cpld_context *cpld, uint64_t address);

uint8_t cpld_get_info(struct cpld_context *cpld);
struct register_context *cpld_get_reg(struct cpld_context *cpld, uint64_t address);
void cpld_deinit(struct cpld_context *cpld);

#endif /* __CPLD_H_ */
