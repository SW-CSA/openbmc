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
#ifndef __SYS_EEPORM_H__
#define __SYS_EEPORM_H__

extern uint32_t *global_crc32_table;

int read_sys_eeprom(void *eeprom_data, int offset, int len);
int write_sys_eeprom(void *eeprom_data, int len);

#endif
