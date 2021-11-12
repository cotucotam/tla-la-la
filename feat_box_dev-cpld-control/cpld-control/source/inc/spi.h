/**
 * Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __SPI_H_
#define __SPI_H_

#include <mpsse.h>
#include <stdio.h>

#if LIBFTDI1 == 1
#include <stdlib.h>
#include <unistd.h>
#endif

#define PIN_MOSI  INVERT_DCD
#define PIN_MISO  INVERT_DTR
#define PIN_SCK	  INVERT_RTS
#define PIN_SSTBZ INVERT_CTS

int spi_init(struct mpsse_context *mpsse);
int spi_read(struct mpsse_context *mpsse, uint64_t address, uint8_t addr_length,
	     uint8_t *value, uint8_t val_length);
int spi_write(struct mpsse_context *mpsse, uint64_t address, uint8_t addr_length,
	      uint8_t *value, uint8_t val_length);

#endif /* __SPI_H_ */
