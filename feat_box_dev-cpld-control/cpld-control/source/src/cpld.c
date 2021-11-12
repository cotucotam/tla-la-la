/**
 * Copyright (C) 2020-2021 Renesas Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "cpld.h"
#include <stdio.h>
#include <string.h>

/**
 * Get the index-th device with a given, vendor id, product id and serial.
 *
 * @param	vendor    Vendor ID.
 * @param	product   Product ID.
 * @param	serial    Serial Number.
 *
 * @return	Index-th device.
 * Return -1 if cannot find any devices with that vendor ID and that product ID.
 * Return -2 if cannot find any devices with that serial number.
 */
int cpld_get_index(int vendor, int product, char *serial)
{
	int ret, index = 0;
	char ser[32];
	struct ftdi_context ftdi;
	struct ftdi_device_list *list, *dev;

	ftdi_init(&ftdi);
	ret = ftdi_usb_find_all(&ftdi, &list, vendor, product);
	if (ret < 0) {
		fprintf(stderr, "Failed to find device!\n");
		ftdi_deinit(&ftdi);
		return -1;
	}

	for (dev = list; dev != NULL; dev = dev->next) {
		ret = ftdi_usb_get_strings(&ftdi, dev->dev, NULL, 0, NULL, 0, ser, 32);
		if (ret == 0 && strcmp(serial, ser) == 0) {
			ftdi_list_free(&list);
			ftdi_deinit(&ftdi);
			return index;
		}
		index++;
	}

	fprintf(stderr, "Failed to find serial number!\n");
	ftdi_list_free(&list);
	ftdi_deinit(&ftdi);
	return -2;
}

/**
     * Initialize CPLD.
 *
 * @param   board	Board name.
 * @param   serial	Device serial number.
 *
 * @return  A pointer to an CPLD context structure.
 */
struct cpld_context *cpld_init(char *board, char *serial)
{
	int ret, index;
	struct cpld_context *cpld;

	printf("Using device %s with iSerial: %s\n\n", board, serial);

	/* Initialize CPLD structure */
	cpld = (struct cpld_context *)malloc(sizeof(struct cpld_context));
	cpld->board_name = board;
	cpld->reg = NULL;
	ret = cpld_get_info(cpld);
	if (ret != 0)
		return NULL;

	/* Initialize MPSSE structure */
	index = cpld_get_index(VENDOR, cpld->product_id, serial);
	if (index < 0)
		return NULL;

	if (cpld->protocol == SPI) { // M3/H3 Starter Kit
		cpld->mpsse = OpenIndex(VENDOR, cpld->product_id, BITBANG, 0, 0, IFACE_A,
					NULL, NULL, index);
		spi_init(cpld->mpsse);
	} else if (cpld->protocol == SMI) { // V3M Starter Kit
		cpld->mpsse = OpenIndex(VENDOR, cpld->product_id, BITBANG, 0, 0, IFACE_A,
					NULL, NULL, index);
		smi_init(cpld->mpsse);
	} else { // V3U/V3H Starter Kit/S4
		cpld->mpsse = OpenIndex(VENDOR, cpld->product_id, BITBANG, 0, 0, IFACE_B,
					NULL, NULL, index);
		i2c_init(cpld->mpsse);
	}
	if (cpld->mpsse->open == 0) {
		fprintf(stderr, "Cannot open device!\n");
		return NULL;
	}

	return cpld;
}

/**
 * Append a register to list
 *
 * @param	cpld_reg	CPLD register structure.
 * @param	name		Register name.
 * @param	address		Register address.
 * @param	addr_length	Address length.
 * @param	val_length	Value length.
 * @param	mode		Register mode.
 *
 * @return	failed(1) if cannot create new register structure.
 */
uint8_t cpld_add_reg(struct register_context **cpld_reg,
		     char *name,
		     uint16_t address,
		     uint8_t addr_length,
		     uint8_t val_length,
		     enum register_mode mode)
{
	struct register_context *node = *cpld_reg;
	struct register_context *reg =
		(struct register_context *)malloc(sizeof(struct register_context));

	if (reg == NULL) {
		fprintf(stderr, "Cannot create node!\n");
		return 1;
	}

	/* add register infomation */
	reg->name = (char *)malloc(strlen(name) * sizeof(char));
	memcpy(reg->name, name, strlen(name));
	reg->address = address;
	reg->value = 0;
	reg->addr_length = addr_length;
	reg->val_length = val_length;
	reg->mode = mode;
	reg->pnext = NULL;

	/* add new register to the end of list */
	if (node == NULL) {
		*cpld_reg = reg;
		return 0;
	}

	while (node->pnext != NULL)
		node = node->pnext;

	node->pnext = reg;
	return 0;
}

/**
 * Get CPLD infomation.
 *
 * @param	cpld	CPLD structure.
 *
 * @return	integer value.
 * return 0 if success.
 * return 1 if failure.
 */
uint8_t cpld_get_info(struct cpld_context *cpld)
{
	if (strcmp(cpld->board_name, "H3SK") == 0 || strcmp(cpld->board_name, "M3SK") == 0) {
	/* H3/M3 Starter Kit */
		cpld->protocol = SPI;
		cpld->product_id = 0x6001;
		cpld_add_reg(&cpld->reg, "MODE",    0x00, 1, 4, RW);
		cpld_add_reg(&cpld->reg, "MUX",     0x02, 1, 4, RW);
		cpld_add_reg(&cpld->reg, "DIPSW6",  0x08, 1, 4, R);
		cpld_add_reg(&cpld->reg, "RESET",   0x80, 1, 4, RW);
		cpld_add_reg(&cpld->reg, "VERSION", 0xFF, 1, 4, R);
	} else if (strcmp(cpld->board_name, "V3U") == 0) {
	/* V3U */
		cpld->protocol = IIC;
		cpld->product_id = 0x6010;
		cpld_add_reg(&cpld->reg, "PRODUCT",     0x0000, 2,  4, R);
		cpld_add_reg(&cpld->reg, "VERSION",     0x0004, 2,  4, R);
		cpld_add_reg(&cpld->reg, "MODE_SET",    0x0008, 2,  8, RW);
		cpld_add_reg(&cpld->reg, "MODE_NEXT",   0x0010, 2,  8, R);
		cpld_add_reg(&cpld->reg, "MODE_LAST",   0x0018, 2,  8, R);
		cpld_add_reg(&cpld->reg, "DIPSW50",     0x0020, 2,  1, R);
		cpld_add_reg(&cpld->reg, "I2C_ADDR",    0x0022, 2,  1, RW);
		cpld_add_reg(&cpld->reg, "RESET",       0x0024, 2,  1, RW);
		cpld_add_reg(&cpld->reg, "POWER_CFG",   0x0025, 2,  1, RW);
		cpld_add_reg(&cpld->reg, "PERI_CFG",    0x0030, 2,  1, RW);
		cpld_add_reg(&cpld->reg, "UART_CFG",    0x0036, 2,  1, RW);
		cpld_add_reg(&cpld->reg, "UART_STATUS", 0x0037, 2,  1, R);
		cpld_add_reg(&cpld->reg, "CNT_POWER",   0x0080, 2,  4, R);
		cpld_add_reg(&cpld->reg, "CNT_RESET",   0x0084, 2,  4, R);
		cpld_add_reg(&cpld->reg, "PCB_VERSION", 0x1000, 2,  2, R);
		cpld_add_reg(&cpld->reg, "SOC_VERSION", 0x1002, 2,  2, R);
		cpld_add_reg(&cpld->reg, "PCB_SN",      0x1004, 2,  4, R);
		cpld_add_reg(&cpld->reg, "MAC",         0x1008, 2,  6, R);
	} else if (strcmp(cpld->board_name, "V3HSK") == 0) {
	/* V3H Starter Kit */
		cpld->protocol = IIC;
		cpld->product_id = 0x6010;
		cpld_add_reg(&cpld->reg, "PRODUCT",      0x0000, 2, 4, R);
		cpld_add_reg(&cpld->reg, "VERSION",      0x0004, 2, 4, R);
		cpld_add_reg(&cpld->reg, "MODE_SET",     0x0008, 2, 5, RW);
		cpld_add_reg(&cpld->reg, "MODE_NEXT",    0x0010, 2, 5, R);
		cpld_add_reg(&cpld->reg, "MODE_LAST",    0x0018, 2, 5, R);
		cpld_add_reg(&cpld->reg, "DIPSW4",       0x0020, 2, 1, R);
		cpld_add_reg(&cpld->reg, "DIPSW5",       0x0021, 2, 1, R);
		cpld_add_reg(&cpld->reg, "I2C_ADDR",     0x0022, 2, 1, RW);
		cpld_add_reg(&cpld->reg, "RESET",        0x0024, 2, 1, RW);
		cpld_add_reg(&cpld->reg, "POWER_CFG",    0x0025, 2, 1, RW);
		cpld_add_reg(&cpld->reg, "PMIC_CFG",     0x0026, 2, 1, RW);
		cpld_add_reg(&cpld->reg, "PCIE_CLK_CFG", 0x0027, 2, 1, RW);
		cpld_add_reg(&cpld->reg, "PERI_CFG",     0x0030, 2, 4, RW);
		cpld_add_reg(&cpld->reg, "LEDS",         0x0034, 2, 1, RW);
		cpld_add_reg(&cpld->reg, "LEDS_CFG",     0x0035, 2, 1, RW);
		cpld_add_reg(&cpld->reg, "UART_CFG",     0x0036, 2, 1, RW);
		cpld_add_reg(&cpld->reg, "UART_STATUS",  0x0037, 2, 1, R);
		cpld_add_reg(&cpld->reg, "PCB_VERSION",  0x1000, 2, 2, R);
		cpld_add_reg(&cpld->reg, "SOC_VERSION",  0x1002, 2, 2, R);
		cpld_add_reg(&cpld->reg, "PCB_SN",       0x1004, 2, 2, R);
		cpld_add_reg(&cpld->reg, "MAC",          0x1008, 2, 6, R);
	} else if (strcmp(cpld->board_name, "V3MSK") == 0) {
	/* V3M Starter Kit */
		cpld->protocol = SMI;
		cpld->product_id = 0x6001;
		cpld_add_reg(&cpld->reg, "PRODUCT",      0x000, 2, 4, R);
		cpld_add_reg(&cpld->reg, "VERSION",      0x002, 2, 4, R);
		cpld_add_reg(&cpld->reg, "MODE_SET",     0x004, 2, 4, RW);
		cpld_add_reg(&cpld->reg, "MODE_APPLIED", 0x006, 2, 4, R);
		cpld_add_reg(&cpld->reg, "DIPSW",        0x008, 2, 2, R);
		cpld_add_reg(&cpld->reg, "RESET",        0x00A, 2, 2, RW);
		cpld_add_reg(&cpld->reg, "POWER_CFG",    0x00B, 2, 2, RW);
		cpld_add_reg(&cpld->reg, "PERI_CFG",     0x00C, 2, 4, RW);
		cpld_add_reg(&cpld->reg, "LEDS",         0x00E, 2, 4, RW);
		cpld_add_reg(&cpld->reg, "PCB_VERSION",  0x300, 2, 2, R);
		cpld_add_reg(&cpld->reg, "SOC_VERSION",  0x301, 2, 2, R);
		cpld_add_reg(&cpld->reg, "PCB_SN",       0x302, 2, 4, R);
	} else if (strcmp(cpld->board_name, "S4") == 0) {
	/* S4 */
		cpld->protocol = IIC;
		cpld->product_id = 0x6010;
		cpld_add_reg(&cpld->reg, "PRODUCT",     0x0000, 2,  4, R);
		cpld_add_reg(&cpld->reg, "VERSION",     0x0004, 2,  4, R);
		cpld_add_reg(&cpld->reg, "MODE_SET",    0x0008, 2,  8, RW);
		cpld_add_reg(&cpld->reg, "MODE_NEXT",   0x0010, 2,  8, R);
		cpld_add_reg(&cpld->reg, "MODE_LAST",   0x0018, 2,  8, R);
		cpld_add_reg(&cpld->reg, "DIPSW8",      0x0020, 2,  1, R);
		cpld_add_reg(&cpld->reg, "I2C_ADDR",    0x0022, 2,  1, RW);
		cpld_add_reg(&cpld->reg, "RESET",       0x0024, 2,  1, RW);
		cpld_add_reg(&cpld->reg, "POWER_CFG",   0x0025, 2,  1, RW);
		cpld_add_reg(&cpld->reg, "PERI_CFG",    0x0030, 2,  1, RW);
		cpld_add_reg(&cpld->reg, "UART_CFG",    0x0036, 2,  1, RW);
		cpld_add_reg(&cpld->reg, "UART_STATUS", 0x0037, 2,  1, R);
		cpld_add_reg(&cpld->reg, "CNT_POWER",   0x0080, 2,  4, R);
		cpld_add_reg(&cpld->reg, "CNT_RESET",   0x0084, 2,  4, R);
		cpld_add_reg(&cpld->reg, "PCB_VERSION", 0x1000, 2,  2, R);
		cpld_add_reg(&cpld->reg, "SOC_VERSION", 0x1002, 2,  2, R);
		cpld_add_reg(&cpld->reg, "PCB_SN",      0x1004, 2,  4, R);
		cpld_add_reg(&cpld->reg, "MAC",         0x1008, 2,  6, R);
	} else {
		fprintf(stderr, "CPLD is not supported for %s\n", cpld->board_name);
		return 1;
	}
	return 0;
}

/**
 * Reset all wrong FTDI.
 *
 * @param	None.
 *
 * @return	Failed when cannot reset usb device.
 */
int cpld_reset_usb(void)
{
	int i = 0;
	int ret = 0;
	libusb_device_handle *dev;
	struct libusb_device *usb, **list;
	struct libusb_device_descriptor desc;

	ret = libusb_init(NULL);
	if (ret < 0) {
		fprintf(stderr, "libusb: Initialized failed!\n");
		return ret;
	}

	ret = libusb_get_device_list(NULL, &list);
	if (ret < 0) {
		fprintf(stderr, "libusb: Get list of device failed!\n");
		libusb_exit(NULL);
		return ret;
	}

	while ((usb = list[i++]) != NULL) {
		ret = libusb_get_device_descriptor(usb, &desc);
		if (ret < 0) {
			fprintf(stderr, "libusb: Fail to get device description!\n");
			goto libusb_err;
		}

		if (desc.idVendor == VENDOR && desc.idProduct == FT4232 &&
		    desc.iSerialNumber == 0) {
			ret = libusb_open(usb, &dev);
			if (ret < 0) {
				fprintf(stderr, "libusb: Cannot open usb device!\n");
				libusb_close(dev);
				goto libusb_err;
			}
			libusb_reset_device(dev);
			usleep(300000);
			libusb_close(dev);
		}
	}

libusb_err:
	libusb_free_device_list(list, 1);
	libusb_exit(NULL);
	return ret;
}

/**
 * List all FTDI devices found.
 *
 * @param	None.
 *
 * @return	Failed when no devices found.
 */
uint8_t cpld_list(void)
{
	int i, ret;
	char ser[32];
	struct ftdi_context ftdi;
	struct ftdi_device_list *list, *dev;
	/*
ftdi	pointer to ftdi_context
devlist	Pointer where to store list of found devices*/
	struct product_context {
		char *name;
		uint16_t value;
	};
	struct product_context product_id[NUM_PRODUCT] = {
	    {"FT232R", FT232R},
	    {"FT2232", FT2232},
	    {"FT4232", FT4232},
	    {"FT232H", FT232H}
	};

	// checking and resetting all wrong boards before listing.
	cpld_reset_usb();

	ftdi_init(&ftdi);
	for (i = 0; i < NUM_PRODUCT; i++) {
		ret = ftdi_usb_find_all(&ftdi, &list, VENDOR, product_id[i].value);
		if (ret < 0) {
			fprintf(stderr, "Failed to list devices!\n");
			ftdi_deinit(&ftdi);
			return ret;
		}

		for (dev = list; dev != NULL; dev = dev->next) {
			ret = ftdi_usb_get_strings(&ftdi, dev->dev,
						NULL, 0, NULL, 0, ser, 32);
						/*serial	Store serial string here if not NULL*/
			if (ret < 0)
				fprintf(stderr, "Skipping device %s!\n", product_id[i].name);
			else
				printf("%s: %s\n", product_id[i].name, ser);
		}
	}

	ftdi_list_free(&list);
	ftdi_deinit(&ftdi);
	return 0;
}

/**
 * Change FTDI serial number
 *
 * @param	cpld		CPLD structure.
 * @param	new_serial	New serial need to change to
 *
 * @return	Failed when errors occur.
 */
uint8_t cpld_change_serial(struct cpld_context *cpld, char *new_serial)
{
	uint8_t ret = 0;

	ftdi_eeprom_initdefaults(&(cpld->mpsse->ftdi), NULL, NULL, new_serial);
	ret = ftdi_erase_eeprom(&(cpld->mpsse->ftdi));
	if (ftdi_set_eeprom_value(&(cpld->mpsse->ftdi), MAX_POWER, 500) < 0)
		fprintf(stderr, "ftdi_set_eeprom_value: %d (%s)\n", ret,
			ftdi_get_error_string(&(cpld->mpsse->ftdi)));

	ret = (ftdi_eeprom_build(&(cpld->mpsse->ftdi)));
	if (ret < 0) {
		fprintf(stderr, "Erase failed: %s", ftdi_get_error_string(&(cpld->mpsse->ftdi)));
		return ret;
	}
	ret = ftdi_write_eeprom(&(cpld->mpsse->ftdi));
	if (ret < 0) {
		fprintf(stderr, "ftdi_eeprom_decode: %d (%s)\n", ret,
			ftdi_get_error_string(&(cpld->mpsse->ftdi)));
		return ret;
	}

	printf("Serial number has been changed, please run cpld-control -l to re-check!\n");
	return ret;
}

/**
 * Read value from an address of CPLD.
 *
 * @param	cpld	CPLD structure.
 * @param	address	CPLD address need to read.
 *
 * @return	uint8_t Read value
 * 0   if read successfully.
 * >0  if read failure.
 * 255 if unsupported address.
 */
uint8_t cpld_read(struct cpld_context *cpld, uint64_t address)
{
	uint8_t ret;
	struct register_context *reg = cpld_get_reg(cpld, address);
	uint64_t addr;

	if (reg == NULL) {
		fprintf(stderr, "The address 0x%0*jX is not supported!\n",
			cpld->reg->addr_length * 2, address);
		return 255;
	}
	if (reg->mode == W) {
		fprintf(stderr, "The address 0x%0*jX is write only!\n",
			cpld->reg->addr_length * 2, address);
		return 255;
	}

	if (cpld->protocol == SPI)
		ret = spi_read(cpld->mpsse, reg->address, reg->addr_length,
			       (uint8_t *)&(reg->value), reg->val_length);
	else if (cpld->protocol == SMI) {
		addr = address;
		// V3MSK issue: remove first 2 bytes if reading flash registers (0x2XX or 0x3XX)
		if (strcmp(cpld->board_name, "V3MSK") == 0 && (reg->address & ~0x1FF) == 0x200) {
			ret = smi_read(cpld->mpsse, addr, reg->addr_length,
				(uint8_t *)&(reg->value), 2);
			if (reg->val_length > 2)
				addr++;
		}
		ret = smi_read(cpld->mpsse, addr, reg->addr_length,
			(uint8_t *)&(reg->value), reg->val_length);
	} else
		ret = i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR, reg->address, reg->addr_length,
				    (uint8_t *)&(reg->value), reg->val_length);

	if (ret == 0)
		printf("%-15s 0x%0*jX: 0x%0*jX\n", reg->name,
		       reg->addr_length * 2, address,
		       reg->val_length * 2, reg->value);

	return ret;
}

/**
 * Write value to an address of CPLD.
 *
 * @param	cpld	CPLD structure.
 * @param	address	CPLD address need to write.
 * @param	value	Value need to write.
 *
 * @return	uint8_t Return value
 * 0   if write successfully.
 * >0  if write failure.
 * 255 if unsupported address.
 */
uint8_t cpld_write(struct cpld_context *cpld, uint64_t address, uint8_t *value)
{
	uint8_t ret;
	struct register_context *reg = cpld_get_reg(cpld, address);

	if (reg == NULL) {
		fprintf(stderr, "The address 0x%0*jX is not supported!\n",
			cpld->reg->addr_length * 2, address);
		return 255;
	}
	if (reg->mode == R) {
		fprintf(stderr, "The address 0x%0*jX is read only!\n",
			reg->addr_length * 2, address);
		return 255;
	}

	printf("Writing register 0x%0*jX with value 0x%0*jX\n",
	       reg->addr_length * 2, address,
	       reg->val_length * 2, *((uint64_t *) value));

	if (cpld->protocol == SPI)
		ret = spi_write(cpld->mpsse, reg->address, reg->addr_length,
				value, reg->val_length);
	else if (cpld->protocol == SMI)
		ret = smi_write(cpld->mpsse, reg->address, reg->addr_length,
				value, reg->val_length);
	else
		ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR, reg->address, reg->addr_length,
				     value, reg->val_length);

	return ret;
}

/**
 * Write (Non-volatile) value to an address of CPLD.
 *
 * @param	cpld	CPLD structure.
 * @param	address	CPLD address need to write.
 * @param	value	Value need to write.
 *
 * @return	uint8_t Return value
 * 0   if write successfully.
 * >0  if write failure.
 * 255 if unsupported address.
 */
uint8_t cpld_write_nonvolatile(struct cpld_context *cpld, uint64_t address, uint8_t *value)
{
	int i;
	uint8_t ret;
	uint8_t page_content[256];
	uint8_t flash_status = 0x01;
	struct register_context *reg = cpld_get_reg(cpld, address);

	if (reg == NULL) {
		fprintf(stderr, "The address 0x%0*jX is not supported!\n",
			 cpld->reg->addr_length * 2, address);
		return 255;
	}

	if (strcmp(cpld->board_name, "V3U") == 0) {
		if ((address != 0x0008) && (address != 0x0025) && (address != 0x0030) &&
		    (address != 0x0036) && (address != 0x1000) && (address != 0x1002) &&
		    (address != 0x1004) && (address != 0x1008)) {
			fprintf(stderr, "The address 0x%0*jX is not supported ",
				cpld->reg->addr_length * 2, address);
			fprintf(stderr, "for writing non-volatile!\n");
			return 255;
		}
		else if ((address == 0x0008) || (address == 0x0025) ||
			   (address == 0x0030) || (address == 0x0036))
			    ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR,
					     reg->address, reg->addr_length,
					     value, reg->val_length);

		       printf("Writing register 0x%0*jX with value 0x%0*jX\n",
		       reg->addr_length * 2, address,
		       reg->val_length * 2, *((uint64_t *) value));

		/**
		 * Find page
		 * Read previous page content. Due to hardware implementation
		 * For page 0, only need to read the first 56 bytes to save time -> make it 60
		 * For page 1, only need to read the first 14 bytes to save time -> make it 16
		 */

		if (address < 0x07FF) { // page 0
			ret = i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x0800, 2,
					    &page_content[0], 60);
			/* Erase previous page content */
			ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x07F0, 2,
					     &flash_status, 1);
		}
		else { // page 1
			ret = i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x1000, 2,
					    &page_content[0], 16);
			/* Erase previous page content */
			ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x07F1, 2,
					     &flash_status, 1);
		}

		do {
			i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x07F0, 2, &flash_status, 1);
		} while (flash_status != 0x01);

		/* Modify page content */
		if (address == 0x0008) // mode set register
			for (i = 0; i < reg->val_length; i++)
				page_content[8 + i] = *(value + i);
		if (address == 0x0025) // power configure register
			page_content[37] = *value;
		if (address == 0x0030) // peripheral configure register
			page_content[48] = *value;
		if (address == 0x0036) // UART configure register
			page_content[54] = *value;
		if (address == 0x1000) { // PCB version
			page_content[0] = *value;
			page_content[1] = *(value + 1);
		}
		if (address == 0x1002) { // SoC version
			page_content[2] = *value;
			page_content[3] = *(value + 1);
		}
		if (address == 0x1004) // PCB Serial number
			for (i = 0; i < reg->val_length; i++)
				page_content[4 + i] = *(value + i);
		if (address == 0x1008) // MAC address
			for (i = 0; i < reg->val_length; i++)
				page_content[8 + i] = *(value + i);

		/* Write back */
		if (address < 0x07FF) // page 0
			for (i = 0; i < 15; i++) {
				do {
					i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR,
						       0x07F0, 2, &flash_status, 1);
				} while (flash_status != 0x01);
				ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR,
						      0x0800 + i * 4, 2, &page_content[i * 4], 4);
			}
		else // page 1
			for (i = 0; i < 4; i++) {
				do {
					i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR,
						       0x07F0, 2, &flash_status, 1);
				} while (flash_status != 0x01);
				ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR,
						      0x1000 + i * 4, 2, &page_content[i * 4], 4);
			}
	}
	else if (strcmp(cpld->board_name, "S4") == 0) {
		if ((address != 0x0008) && (address != 0x0025) && (address != 0x0030) &&
		    (address != 0x0036) && (address != 0x1000) && (address != 0x1002) &&
		    (address != 0x1004) && (address != 0x1008)) {
			fprintf(stderr, "The address 0x%0*jX is not supported ",
				cpld->reg->addr_length * 2, address);
			fprintf(stderr, "for writing non-volatile!\n");
			return 255;
		} else if ((address == 0x0008) || (address == 0x0025) ||
			   (address == 0x0030) || (address == 0x0036))
			ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR,
					     reg->address, reg->addr_length,
					     value, reg->val_length);

		printf("Writing register 0x%0*jX with value 0x%0*jX\n",
		       reg->addr_length * 2, address,
		       reg->val_length * 2, *((uint64_t *) value));

		/**
		 * Find page
		 * Read previous page content. Due to hardware implementation
		 * For page 0, only need to read the first 56 bytes to save time -> make it 60
		 * For page 1, only need to read the first 14 bytes to save time -> make it 16
		 */
		if (address < 0x07FF) { // page 0
			ret = i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x0800, 2,
					    &page_content[0], 60);
			/* Erase previous page content */
			ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x07F0, 2,
					     &flash_status, 1);
		}
		else { // page 1
			ret = i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x1000, 2,
					    &page_content[0], 16);
			/* Erase previous page content */
			ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x07F1, 2,
					     &flash_status, 1);
		}

		do {
			i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x07F0, 2, &flash_status, 1);
		} while (flash_status != 0x01);

		/* Modify page content */
		if (address == 0x0008) // mode set register
			for (i = 0; i < reg->val_length; i++)
				page_content[8 + i] = *(value + i);
		if (address == 0x0025) // power configure register
			page_content[37] = *value;
		if (address == 0x0030) // peripheral configure register
			page_content[48] = *value;
		if (address == 0x0036) // UART configure register
			page_content[54] = *value;
		if (address == 0x1000) { // PCB version
			page_content[0] = *value;
			page_content[1] = *(value + 1);
		}
		if (address == 0x1002) { // SoC version
			page_content[2] = *value;
			page_content[3] = *(value + 1);
		}
		if (address == 0x1004) // PCB Serial number
			for (i = 0; i < reg->val_length; i++)
				page_content[4 + i] = *(value + i);
		if (address == 0x1008) // MAC address
			for (i = 0; i < reg->val_length; i++)
				page_content[8 + i] = *(value + i);
		/* Write back */
		if (address < 0x07FF) // page 0
			for (i = 0; i < 15; i++) {
				do {
					i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR,
						       0x07F0, 2, &flash_status, 1);
				} while (flash_status != 0x01);
				ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR,
						      0x0800 + i * 4, 2, &page_content[i * 4], 4);
			}
		else // page 1
			for (i = 0; i < 4; i++) {
				do {
					i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR,
						       0x07F0, 2, &flash_status, 1);
				} while (flash_status != 0x01);
				ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR,
						      0x1000 + i * 4, 2, &page_content[i * 4], 4);
			}
	}
	else if (strcmp(cpld->board_name, "V3HSK") == 0) {
		if ((address != 0x0008) && (address != 0x0025) && (address != 0x0026) &&
		    (address != 0x0027) && (address != 0x0030) && (address != 0x0034) &&
		    (address != 0x0035) && (address != 0x0036) && (address != 0x1000) &&
		    (address != 0x1002) && (address != 0x1004) && (address != 0x1008)) {
			fprintf(stderr, "The address 0x%0*jX is not supported ",
				cpld->reg->addr_length * 2, address);
			fprintf(stderr, "for writing non-volatile!\n");
			return 255;
		} else if ((address == 0x0008) || (address == 0x0025) || (address == 0x0026) ||
			   (address == 0x0027) || (address == 0x0030) || (address == 0x0034) ||
			   (address == 0x0035) || (address == 0x0036))
			ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR,
					     reg->address, reg->addr_length,
					     value, reg->val_length);

		printf("Writing register 0x%0*jX with value 0x%0*jX\n",
		       cpld->reg->addr_length * 2, address,
		       reg->val_length * 2, *((uint64_t *) value));

		/**
		 * Find page
		 * Read previous page content. Due to hardware implementation
		 * For page 0, only need to read the first 56 bytes to save time -> make it 60
		 * For page 1, only need to read the first 14 bytes to save time -> make it 16
		 */
		if (address < 0x07FF) { // page 0
			ret = i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x0800, 2,
					    &page_content[0], 60);
			/* Erase previous page content */
			ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x07F0, 2,
					     &flash_status, 1);
		} else { // page 1
			ret = i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x1000, 2,
					    &page_content[0], 16);
			/* Erase previous page content */
			ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x07F1, 2,
					     &flash_status, 1);
		}

		do {
			i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR, 0x07F0, 2, &flash_status, 1);
		} while (flash_status != 0x01);

		/* Modify page content */
		if (address == 0x0008) // mode set register
			for (i = 0; i < reg->val_length; i++)
				page_content[8 + i] = *(value + i);
		if (address == 0x0025) // power configure register
			page_content[37] = *value;
		if (address == 0x0026) // PMIC configure register
			page_content[38] = *value;
		if (address == 0x0027) // PCIe clock configure register
			page_content[39] = *value;
		if (address == 0x0030) // peripheral configure register
			page_content[48] = *value;
		if (address == 0x0034) // LED register
			page_content[52] = *value;
		if (address == 0x0035) // LED configure register
			page_content[53] = *value;
		if (address == 0x0036) // UART configure register
			page_content[54] = *value;
		if (address == 0x1000) { // PCB version
			page_content[0] = *value;
			page_content[1] = *(value + 1);
		}
		if (address == 0x1002) { // SoC version
			page_content[2] = *value;
			page_content[3] = *(value + 1);
		}
		if (address == 0x1004) { // PCB Serial number
			page_content[4] = *value;
			page_content[5] = *(value + 1);
		}
		if (address == 0x1008) // MAC address
			for (i = 0; i < reg->val_length; i++)
				page_content[8 + i] = *(value + i);

		/* Write back */
		if (address < 0x07FF) // page 0
			for (i = 0; i < 15; i++) {
				do {
					i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR,
						       0x07F0, 2, &flash_status, 1);
				} while (flash_status != 0x01);
				ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR,
						      0x0800 + i * 4, 2, &page_content[i * 4], 4);
			}
		else // page 1
			for (i = 0; i < 4; i++) {
				do {
					i2c_read_data(cpld->mpsse, CPLD_SLAVE_ADDR,
						       0x07F0, 2, &flash_status, 1);
				} while (flash_status != 0x01);
				ret = i2c_write_data(cpld->mpsse, CPLD_SLAVE_ADDR,
						      0x1000 + i * 4, 2, &page_content[i * 4], 4);
			}
	}
	else if (strcmp(cpld->board_name, "V3MSK") == 0) {
		if ((address != 0x004) && (address != 0x00B) && (address != 0x00C) &&
		    (address != 0x00E) && (address != 0x300) && (address != 0x301) &&
		    (address != 0x302)) {
			fprintf(stderr, "The address 0x%0*jX is not supported ",
				cpld->reg->addr_length * 2, address);
			fprintf(stderr, "for writing non-volatile!\n");
			return 255;
		} else if ((address == 0x004) || (address == 0x00B) ||
			   (address == 0x00C) || (address == 0x00E))
			ret = smi_write(cpld->mpsse, reg->address, reg->addr_length,
					 value, reg->val_length);

		printf("Writing register 0x%0*jX with value 0x%0*jX\n",
		       reg->addr_length * 2, address,
		       reg->val_length * 2, *((uint64_t *) value));

		/**
		 * Find page
		 * Read previous page content. Due to hardware implementation
		 * For page 0, only need to read the first 30 bytes to save time!
		 * For page 1, only need to read the first 8 bytes to save time!
		 */
		flash_status = 0x00;
		if (address < 0x2FF) { // page 0
			// V3MSK issue: remove first 2 bytes if reading flash registers
			ret = smi_read(cpld->mpsse, 0x200, 2, &page_content[0], 2);
			ret = smi_read(cpld->mpsse, 0x201, 2, &page_content[0], 30);

			/* Erase previous page content */
			ret = smi_write(cpld->mpsse, 0x1FE, 2, &flash_status, 2);
		} else { // page 1
			// V3MSK issue: remove first 2 bytes if reading flash registers
			ret = smi_read(cpld->mpsse, 0x300, 2, &page_content[0], 2);
			ret = smi_read(cpld->mpsse, 0x301, 2, &page_content[0], 8);

			/* Erase previous page content */
			ret = smi_write(cpld->mpsse, 0x1FF, 2, &flash_status, 2);
		}

		do {
			smi_read(cpld->mpsse, 0x009, 2, &flash_status, 2);
		} while (flash_status != 0x01);

		/**
		 * Modify page content
		 * In page 0, the data in flash is stored inverted
		 * so inverting the data before writing back is needed!
		 */
		if (address == 0x004) // mode set register
			for (i = 0; i < reg->val_length; i++)
				page_content[4 * 2 + i] = 0xFF ^ *(value + i);
		if (address == 0x00B) { // power configure register
			page_content[22] = 0xFF ^ *value;
			page_content[23] = 0xFF ^ *(value + 1) ^ 0x80;
		}
		if (address == 0x00C) { // peripheral configure register
			for (i = 0; i < reg->val_length; i++)
				page_content[12 * 2 + i] = 0xFF ^ *(value + i);
		}
		if (address == 0x00E) { // LED register
			page_content[28] = 0xFF ^ *value;
			page_content[29] = 0xFF ^ *(value + 1);
		}
		if (address == 0x300) { // PCB version
			page_content[0] = *value;
			page_content[1] = *(value + 1);
		}
		if (address == 0x301) { // SoC version
			page_content[2] = *value;
			page_content[3] = *(value + 1);
		}
		if (address == 0x302) // PCB Serial number
			for (i = 0; i < reg->val_length; i++)
				page_content[2 * 2 + i] = *(value + i);

		/* Write back */
		if (address < 0x2FF) // page 0
			for (i = 0; i < 30 / 2; i++) {
				ret = smi_write(cpld->mpsse, 0x200 + i,
						2, &page_content[i * 2], 2);
				do {
					smi_read(cpld->mpsse, 0x009, 2, &flash_status, 2);
				} while (flash_status != 0x01);
			}
		else // page 1
			for (i = 0; i < 8 / 2; i++) {
				ret = smi_write(cpld->mpsse, 0x300 + i,
						2, &page_content[i * 2], 2);
				do {
					smi_read(cpld->mpsse, 0x009, 2, &flash_status, 2);
				} while (flash_status != 0x01);
			}
	}
	else { // Not support
		fprintf(stderr, "Cannot write! Only support write non-volatile function for ");
		fprintf(stderr, "V3MSK, V3HSK, V3U and S4\n");
	}

	return ret;
}

/**
 * Dump value of all registers.
 *
 * @param	cpld	CPLD structure.
 * @param	address	Address to display (0xFFFFF to dump all registers).
 *
 * @return	Number of NACKs from slave.
 * 0   if read successfully.
 * >0  if read failure.
 * 255 if unsupported address.
 */
uint8_t cpld_dump(struct cpld_context *cpld, uint64_t address)
{
	int ret = 0;
	struct register_context *reg;

	if (address == 0xFFFFF) {
		reg = cpld->reg;
		while (reg != NULL) {
			if (reg->mode != W) {
				ret = cpld_read(cpld, reg->address);
				if (ret != 0)
					return ret;
			}
			reg = reg->pnext;
		}
	} else {
		ret = cpld_read(cpld, address);
		if (ret != 0)
			return ret;
	}
	return ret;
}

/**
 * Get register's infomation.
 *
 * @param	cpld	CPLD structure.
 * @param	address	Address to look up.
 *
 * @return	A register structure.
 */
struct register_context *cpld_get_reg(struct cpld_context *cpld, uint64_t address)
{
	struct register_context *reg = cpld->reg;

	while (reg != NULL) {
		if (reg->address == address)
			break;
		reg = reg->pnext;
	}
	return reg;
}

/**
 * Deinitialize CPLD structure
 *
 * @param	cpld	CPLD structure.
 *
 * @return	None.
 */
void cpld_deinit(struct cpld_context *cpld)
{
	struct register_context *reg;

	while (cpld->reg != NULL) {
		reg = cpld->reg;
		cpld->reg = cpld->reg->pnext;
		free(reg->name);
		free(reg);
	}
	Close(cpld->mpsse);
	free(cpld);
}
