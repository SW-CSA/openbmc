/*
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>


#define MDIO_PHY_TYPE 0
#define MDIO_SWITCH_TYPE 1
#define MDIO_READ 0
#define MDIO_WRITE 1
#define MIDO_RW_BUF_LEN 20

/*Switch registers*/
#define ACCESS_CTRL_REG 16
#define IO_CTRL_REG 17
#define STATUS_REG 18
#define DATA0_REG 24
#define DATA1_REG 25
#define DATA2_REG 26
#define DATA3_REG 27

#define SWITCH_WAIT_TIME 10*1000 //10ms


#define MDIO_SYSNODE "/sys/devices/virtual/mdio_bus/ftgmac100_mii/mdio"

void usage()
{
  fprintf(stderr,
          "Usage:\n"
          "mdio: <type> <read|write> <phy address> [page] <register address> [value to write]\n"
          "type:    -p: phy\n"
          "         -s: switch\n");
}

static int sysfs_read(char *path)
{
	FILE *fp;
	int ret;
	int val;

	fp = fopen(path, "r");
	if(!fp) {
		syslog(LOG_ERR, "open %s error!", path);
		return errno;
	}

	ret = fscanf(fp, "%x", &val);
	if(ret < 0) {
		syslog(LOG_ERR, "read %s error!", path);
		fclose(fp);
		return -1;
	}

	fclose(fp);

	return val;
}

static int sysfs_write(char *path, char *val)
{
	FILE *fp;
	int ret;

	fp = fopen(path, "w");
	if(!fp) {
		syslog(LOG_ERR, "open %s error!", path);
		return errno;
	}

	ret = fputs(val, fp);
	if(ret < 0) {
		syslog(LOG_ERR, "write %s error!", path);
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}


static int mdio_phy_read(int addr, int reg)
{
	int ret;
	char buf[MIDO_RW_BUF_LEN];

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "0x%x 0x%x", addr, reg);
	ret = sysfs_write(MDIO_SYSNODE, buf);
	if(ret < 0) {
		return -1;
	}

	ret = sysfs_read(MDIO_SYSNODE);
	if(ret < 0) {
		return -1;
	}

	return ret;
}

static int mdio_phy_write(int addr, int reg, int data)
{
	int ret;
	char buf[MIDO_RW_BUF_LEN];

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "0x%x 0x%x 0x%x", addr, reg, data);
	ret = sysfs_write(MDIO_SYSNODE, buf);
	if(ret < 0) {
		return -1;
	}

	return 0;
}

static int mdio_switch_wait(int addr)
{
	int val;
	int count = 100;

	/*# Read MII register IO_CTRL_REG:
          # Check op_code = "00"*/
	do {
		val = mdio_phy_read(addr, IO_CTRL_REG);
		val &= 0x3;
		usleep(SWITCH_WAIT_TIME);
	} while(val && --count > 0);

	if(count <= 0)
		return -1;

	return 0;
}

static int mdio_switch_read(int addr, int page, int reg)
{
	int val = 0;
	int tmp;

	/*set page*/
	val = 0x1 | ((page & 0xff) << 8);
	if(mdio_phy_write(addr, ACCESS_CTRL_REG, val) < 0) {
		syslog(LOG_ERR, "Switch access: Set page error!");
		return -1;
	}
	
	/*# Write MII register IO_CTRL_REG:
          # set "Operation Code as "00"
          # set "Register Address" to bit 15:8*/
	val = 0x00 | ((reg & 0xff) << 8);
	if(mdio_phy_write(addr, IO_CTRL_REG, val) < 0) {
		syslog(LOG_ERR, "Switch access: Set reg error!");
		return -1;
	}

	/*# Write MII register IO_CTRL_REG:
          # set "Operation Code as "10"
          # set "Register Address" to bit 15:8*/
	val = 0x2 | ((reg & 0xff) << 8);
	if(mdio_phy_write(addr, IO_CTRL_REG, val) < 0) {
		syslog(LOG_ERR, "Switch access: Set read opcode error!");
		return -1;
	}

	if(mdio_switch_wait(addr) < 0) {
		syslog(LOG_ERR, "Switch access: Wait timeout!");
		return -1;
	}
	
	//Read MII register DATA0_REG for bit 15:0
	val = mdio_phy_read(addr, DATA0_REG);
	if(val < 0) {
		syslog(LOG_ERR, "Switch access: read DATA0_REG error!");
		return -1;
	}
	//Read MII register DATA1_REG for bit 31:16
	tmp = mdio_phy_read(addr, DATA1_REG);
	if(tmp < 0) {
		syslog(LOG_ERR, "Switch access: read DATA1_REG error!");
		return -1;
	}
	val |= tmp << 16;
	//Read MII register DATA2_REG for bit 47:32
	tmp = mdio_phy_read(addr, DATA2_REG);
	if(tmp < 0) {
		syslog(LOG_ERR, "Switch access: read DATA2_REG error!");
		return -1;
	}
	val |= tmp << 32;
	//Read MII register DATA3_REG for bit 63:48
	tmp = mdio_phy_read(addr, DATA3_REG);
	if(tmp < 0) {
		syslog(LOG_ERR, "Switch access: read DATA3_REG error!");
		return -1;
	}
	val |= tmp << 48;

	return val;
}

static int mdio_switch_write(int addr, int page, int reg, int data)
{
	int ret;
	int val = 0;
	int tmp;

	/*set page*/
	val = 0x1 | ((page & 0xff) << 8);
	if(mdio_phy_write(addr, ACCESS_CTRL_REG, val) < 0) {
		syslog(LOG_ERR, "Switch access: Set page error!");
		return -1;
	}

	//Write MII register DATA0_REG for bit 15:0
	ret = mdio_phy_write(addr, DATA0_REG, data & 0xFFFF);
	if(ret < 0) {
		syslog(LOG_ERR, "Switch access: write DATA0_REG error!");
		return -1;
	}
	//Write MII register DATA1_REG for bit 31:16
	ret = mdio_phy_write(addr, DATA1_REG, (data >> 16) & 0xFFFF);
	if(ret < 0) {
		syslog(LOG_ERR, "Switch access: write DATA1_REG error!");
		return -1;
	}
	//Write MII register DATA2_REG for bit 47:32
	ret = mdio_phy_write(addr, DATA2_REG, (data >> 32) & 0xFFFF);
	if(ret < 0) {
		syslog(LOG_ERR, "Switch access: write DATA2_REG error!");
		return -1;
	}
	//Write MII register DATA3_REG for bit 63:48
	ret = mdio_phy_write(addr, DATA3_REG, (data >> 48) & 0xFFFF);
	if(ret < 0) {
		syslog(LOG_ERR, "Switch access: write DATA3_REG error!");
		return -1;
	}

	/*# Write MII register IO_CTRL_REG:
          # set "Operation Code as "00"
          # set "Register Address" to bit 15:8*/
	val = 0x00 | ((reg & 0xff) << 8);
	if(mdio_phy_write(addr, IO_CTRL_REG, val) < 0) {
		syslog(LOG_ERR, "Switch access: Set reg error!");
		return -1;
	}
	/*# Write MII register IO_CTRL_REG:
          # set "Operation Code as "01"
          # set "Register Address" to bit 15:8*/
	val = 0x1 | ((reg & 0xff) << 8);
	if(mdio_phy_write(addr, IO_CTRL_REG, val) < 0) {
		syslog(LOG_ERR, "Switch access: Set write opcode error!");
		return -1;
	}
	if(mdio_switch_wait(addr) < 0) {
		syslog(LOG_ERR, "Switch access: Wait timeout!");
		return -1;
	}

	return 0;
}


int main(int argc, char* const argv[])
{
	int opt;
	int type = -1;
	int is_write;
	int page;
	int phy_addr;
	int reg_addr;
	int data;
	int ret;
	
	while ((opt = getopt(argc, argv, "sp")) != -1) {
		switch (opt) {
			case 'p':
			type = MDIO_PHY_TYPE;
			break;
		case 's':
			type = MDIO_SWITCH_TYPE;
			break;
		default:
			usage();
			exit(-1);
		}
	}
	if(type == -1) {
		usage();
		exit(-1);
	}

	if (optind + 2 >= argc) {
		usage();
		exit(-1);
	}

	/* read or write */
	if (!strcasecmp(argv[optind], "read")) {
		is_write = MDIO_READ;
	} else if (!strcasecmp(argv[optind], "write")) {
		is_write = MDIO_WRITE;
	} else {
		usage();
		exit(-1);
	}

	if(type == MDIO_PHY_TYPE) {
		/*read/write phy register*/	
		/* phy address, 5 bits only, so must be <= 0x1f */
		phy_addr = strtoul(argv[optind + 1], NULL, 0);
		if (phy_addr > 0x1f) {
			usage();
			exit(-1);
		}
		
		/* register address, 5 bits only, so must be <= 0x1f */
		reg_addr = strtoul(argv[optind + 2], NULL, 0);
		
		if(is_write == MDIO_READ) {
			data = mdio_phy_read(phy_addr, reg_addr);
		} else {
			data = strtoul(argv[optind + 3], NULL, 0);
			if (data > 0xFFFF) {
				usage();
				exit(-1);
			}
			ret = mdio_phy_write(phy_addr, reg_addr, data);
		}
	} else {
		/*read/write switch register by MDIO*/
		phy_addr = strtoul(argv[optind + 1], NULL, 0);
		if (phy_addr > 0x1f) {
			usage();
			exit(-1);
		}
		/*page*/
		page = strtoul(argv[optind + 2], NULL, 0);

		/* register address, 5 bits only, so must be <= 0x1f */
		reg_addr = strtoul(argv[optind + 3], NULL, 0);
		if(is_write == MDIO_READ) {
			data = mdio_switch_read(phy_addr, page, reg_addr);
		} else {
			data = strtoul(argv[optind + 4], NULL, 0);
			if (data > 0xFFFF) {
				usage();
				exit(-1);
			}
			ret = mdio_switch_write(phy_addr, page, reg_addr, data);
		}
	}

	if(is_write == MDIO_READ) {
		printf("0x%02x\n", data);
	} else {
		if(ret != 0)
			printf("Write failed\n");
	}

  return 0;
}
