/*
 * TLV EEPROM utility and based on ONIE platform
 # Copyright ONIE
 * Copyright 2018-present Celestica. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "onie_tlvinfo.h"
#include "sys_eeprom.h"
#include <mtd/mtd-user.h>

/*
 * read_sys_eeprom - read the hwinfo from MTD EEPROM
 */
int read_sys_eeprom(void *eeprom_data, int offset, int len)
{
	int rc;
	int ret = 0;
	int fd;
	u_int8_t *c;

	c = eeprom_data;

	fd = open(SYS_EEPROM_SYSFS_FILE_PATH, O_RDONLY);
	if (fd < 0) {
		fprintf (stderr, "Can't open %s: %s\n",
			SYS_EEPROM_SYSFS_FILE_PATH, strerror (errno));
		return -1;
	}

	if (lseek (fd, offset + SYS_EEPROM_OFFSET, SEEK_SET) == -1) {
		fprintf (stderr, "Seek error on %s: %s\n",
			SYS_EEPROM_SYSFS_FILE_PATH, strerror (errno));
		return -1;
	}

	rc = read (fd, c, len);
	if (rc != len) {
		fprintf (stderr, "Read error on %s: %s\n",
			SYS_EEPROM_SYSFS_FILE_PATH, strerror (errno));
		return -1;
	}

	if (close (fd)) {
		fprintf (stderr, "I/O error on %s: %s\n",
			SYS_EEPROM_SYSFS_FILE_PATH, strerror (errno));
		return -1;
	}
	return ret;
}

/*
 * write_sys_eeprom - write the hwinfo to MTD EEPROM
 */
int write_sys_eeprom(void *eeprom_data, int len)
{
	int rc;
	int ret = 0;
	int fd;
	u_int8_t *c;

	c = eeprom_data;

	fd = open(SYS_EEPROM_SYSFS_FILE_PATH, O_RDWR);
	if (fd < 0) {
		fprintf (stderr, "Can't open %s: %s\n",
			SYS_EEPROM_SYSFS_FILE_PATH, strerror (errno));
		return -1;
	}

	if (lseek (fd, SYS_EEPROM_OFFSET, SEEK_SET) == -1) {
		fprintf (stderr, "Seek error on %s: %s\n",
			SYS_EEPROM_SYSFS_FILE_PATH, strerror (errno));
		return -1;
	}

	rc = write (fd, c, len);
	if (rc != len) {
		fprintf (stderr, "Write error on %s: %s\n",
			SYS_EEPROM_SYSFS_FILE_PATH, strerror (errno));
		return -1;
	}

	if (close (fd)) {
		fprintf (stderr, "I/O error on %s: %s\n",
			SYS_EEPROM_SYSFS_FILE_PATH, strerror (errno));
		return -1;
	}

	return ret;
}
