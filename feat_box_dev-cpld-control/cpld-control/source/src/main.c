/**
 * Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "cpld.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAJOR_VERSION 1
#define MINOR_VERSION 7

/**
 * usage
 */
void usage(char *pn)
{
	printf("CPLD control version %d.%d.1\n", MAJOR_VERSION, MINOR_VERSION);
	printf("\nThe valid <Board name>: M3SK, H3SK, V3HSK, V3MSK, V3U, S4\n\n");
	printf("%s -h ....................................................... ", pn);
	printf("Print this help.\n");

	printf("%s -l ....................................................... ", pn);
	printf("List available devices.\n");

	printf("%s -c <Board name> <Old serial number> <New serial number>... ", pn);
	printf("Change FTDI serial number\n");

	printf("%s -r <Board name> <FTDI iSerial> ........................... ", pn);
	printf("Print all CPLD registers.\n");

	printf("%s -r <Board name> <FTDI iSerial> <reg>* .................... ", pn);
	printf("Print 1 CPLD register.\n");
	printf("\t\t\t\t *One or more <reg> can be specified.\n");

	printf("%s -w <Board name> <FTDI iSerial> [<reg> <val>]* ............ ", pn);
	printf("Write CPLD register(s).\n");
	printf("\t\t\t\t *One or more [<reg> <val>] pairs can be specified.\n");

	printf("%s -wnv <Board name> <FTDI iSerial> [<reg> <val>]* .......... ", pn);
	printf("Write non-volatile CPLD register(s).\n");
	printf("\t\t\t\t *One or more [<reg> <val>] pairs can be specified.\n");
}

/**
 * Main function
 */
int main(int argc, char *argv[])
{
	struct cpld_context *cpld;
	uint64_t reg;
	uint64_t val;
	char *endptr;
	int i, ret = EXIT_FAILURE;

	if ((argc < 2) || (argc == 2 && !strcmp(argv[1], "-h"))) {
		usage(argv[0]);
		return EXIT_SUCCESS;
	}

	if (argc == 2 && !strcmp(argv[1], "-l")) {
		ret = cpld_list();
		return ret;
	}

	if (argc > 2 && !strcmp(argv[1], "-l")) {
		fprintf(stderr, "The -l option takes no arguments!\n");
		usage(argv[0]);
		return ret;
	}

	if (argc != 5 && !strcmp(argv[1], "-c")) {
		fprintf(stderr, "The -c option takes three arguments");
		fprintf(stderr, "(board name, old serial number and new serial number)!\n");
		usage(argv[0]);
		return ret;
	}

	if (strcmp(argv[1], "-r") && strcmp(argv[1], "-w") &&
	    strcmp(argv[1], "-c") && strcmp(argv[1], "-wnv") && argc >= 2) {
		printf("Unknown option!\n");
		usage(argv[0]);
		return ret;
	}

	if (argc < 4 && !strcmp(argv[1], "-r")) {
		fprintf(stderr, "The -d option takes at least one board name and one iSerial!\n");
		usage(argv[0]);
		return ret;
	}

	if (((argc < 6) || (((argc - 6) % 2) != 0)) && !strcmp(argv[1], "-w")) {
		fprintf(stderr, "The -w option takes one board name, one iSerial ");
		fprintf(stderr, "and at least one reg/val pair!\n");
		usage(argv[0]);
		return ret;
	}

	if (((argc < 6) || (((argc - 6) % 2) != 0)) && !strcmp(argv[1], "-wnv")) {
		fprintf(stderr, "The -wnv option takes one board name, one iSerial ");
		fprintf(stderr, "and at least one reg/val pair!\n");
		usage(argv[0]);
		return ret;
	}

	/* init CPLD */
	ret = EXIT_SUCCESS;
	cpld = cpld_init(argv[2], argv[3]);
	if (cpld == NULL) {
		fprintf(stderr, "Initialize failed!\n");
		return EXIT_FAILURE;
	}

	/* Change serial number */
	if (argc == 5 && !strcmp(argv[1], "-c")) {
		ret = cpld_change_serial(cpld, argv[4]);
		if (ret != 0) {
			fprintf(stderr, "Failed to change serial!\n");
			cpld_deinit(cpld);
			return ret;
		}
	}

	/* Dump registers */
	if (argc == 4 && !strcmp(argv[1], "-r")) {
		ret = cpld_dump(cpld, 0xFFFFF);
	} else if (argc > 4 && !strcmp(argv[1], "-r")) {
		for (i = 4; i < argc; i++) {
			reg = strtoull(argv[i], &endptr, 16);
			if (reg == ULLONG_MAX)
				fprintf(stderr, "The address %s is too large!\n", argv[i]);
			else
				ret |= cpld_dump(cpld, reg);
		}
	}

	/* Write registers */
	if (argc >= 5 && (((argc - 6) % 2) == 0) && !strcmp(argv[1], "-w")) {
		for (i = 4; i < argc; i += 2) {
			reg = strtoull(argv[i], &endptr, 16);
			val = strtoull(argv[i + 1], &endptr, 16);
			if (reg == ULLONG_MAX || val == ULLONG_MAX)
				fprintf(stderr, "The address %s or value %s is too large!\n",
					argv[i],
					argv[i + 1]);
			else
				ret = cpld_write(cpld, reg, (uint8_t *)&val);
		}
		ret |= cpld_dump(cpld, 0xFFFFF);
	}

	/* Write non-volatile registers */
	if (argc >= 5 && (((argc - 6) % 2) == 0) && !strcmp(argv[1], "-wnv")) {
		for (i = 4; i < argc; i += 2) {
			reg = strtoull(argv[i], &endptr, 16);
			val = strtoull(argv[i + 1], &endptr, 16);
			if (reg == ULLONG_MAX || val == ULLONG_MAX)
				fprintf(stderr, "The address %s or value %s is too large!\n",
					argv[i],
					argv[i + 1]);
			else
				ret = cpld_write_nonvolatile(cpld, reg, (uint8_t *)&val);
		}
		ret |= cpld_dump(cpld, 0xFFFFF);
	}

	cpld_deinit(cpld);
	return ret;
}
