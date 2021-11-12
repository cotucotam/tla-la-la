/**
 * Copyright (C) 2020-2021 Renesas Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __SMI_H_
#define __SMI_H_

#include <mpsse.h>
#include <stdio.h>

#if LIBFTDI1 == 1
#include <stdlib.h>
#include <unistd.h>
#endif

#define PIN_MDC  INVERT_RTS
#define PIN_MDI  INVERT_CTS
#define PIN_MDO  INVERT_DTR

int smi_init(struct mpsse_context *mpsse);

int smi_read(struct mpsse_context *mpsse, uint64_t address, uint8_t addr_length,
	     uint8_t *value, uint8_t val_length);
int smi_write(struct mpsse_context *mpsse, uint64_t address, uint8_t addr_length,
	      uint8_t *value, uint8_t val_length);

int smi_read_frame(struct mpsse_context *mpsse, uint16_t address, uint8_t *value);
int smi_write_frame(struct mpsse_context *mpsse, uint16_t address, uint16_t value);

#endif /* __SMI_H_ */
