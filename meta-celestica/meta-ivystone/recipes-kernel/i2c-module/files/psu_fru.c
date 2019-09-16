/*
 * FRU driver for PSU and compatibles
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */



#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/i2c/pmbus.h>
#include "pmbus.h"
#include "i2c_dev_sysfs.h"

#define PSU_MFR_ID 0x0c
#define PSU_MFR_NAME 0x12
#define PSU_MFR_VERSION 0x36
#define PSU_MFR_SERIAL 0x3a




#define PSU_FRU_WAIT_TIME		1000	/* uS	*/
#define PSU_FRU_DATA_SIZE_MAX  32

enum chips {
	PSU_FRU = 1,
};


static const struct i2c_device_id psu_fru_id[] = {
	{"psu_fru", PSU_FRU },

	{ }
};


static ssize_t mfr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret, count = 10;
	unsigned char rdata[PSU_FRU_DATA_SIZE_MAX] = "\0";
	i2c_sysfs_attr_st *i2c_attr = TO_I2C_SYSFS_ATTR(attr);
	const i2c_dev_attr_st *dev_attr = i2c_attr->isa_i2c_attr;

	do {
		ret = i2c_dev_read_nbytes(dev, attr, rdata, dev_attr->ida_n_bits);
		if(ret < 0) {
			udelay(PSU_FRU_WAIT_TIME);
			continue;
		} else {
			break;
		}
	}while(count-- > 0);

	if(count <= 0)
		return -1;

	return scnprintf(buf, PAGE_SIZE, "%s\n", rdata);
}




static i2c_dev_data_st psu_fru_data;
static const i2c_dev_attr_st psu_fru_attr_table[] = {
	{
	"mfr_id",
	NULL,
	mfr_show,
	NULL,
	PSU_MFR_ID, 0, 5,
	},
	{
	"mfr_name",
	NULL,
	mfr_show,
	NULL,
	PSU_MFR_NAME, 0, 14,
	},
	{
	"mfr_version",
	NULL,
	mfr_show,
	NULL,
	PSU_MFR_VERSION, 0, 3,
	},
	{
	"mfr_serial",
	NULL,
	mfr_show,
	NULL,
	PSU_MFR_SERIAL, 0, 14,
	},
};

static int psu_fru_remove(struct i2c_client *client)
{
	i2c_dev_sysfs_data_clean(client, &psu_fru_data);
	return 0;
}

static int psu_fru_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int n_attrs;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_READ_WORD_DATA | I2C_FUNC_SMBUS_READ_BLOCK_DATA
			| I2C_FUNC_SMBUS_READ_I2C_BLOCK))
		return -ENODEV;

	n_attrs = sizeof(psu_fru_attr_table) / sizeof(psu_fru_attr_table[0]);

	return i2c_dev_sysfs_data_init(client, &psu_fru_data, psu_fru_attr_table, n_attrs);
}


static struct i2c_driver psu_fru_driver = {
	.driver = {
		   .name = "psu fru",
		   },
	.probe = psu_fru_probe,
	.remove = psu_fru_remove,
	.id_table = psu_fru_id,
};

module_i2c_driver(psu_fru_driver);

MODULE_AUTHOR("Micky Zhan@Celestica.com");
MODULE_DESCRIPTION("FRU driver for PSU");
MODULE_LICENSE("GPL");

