/*
 *  The i2c driver for CPU error
 *
 * Copyright 2019-present Celestica. All Rights Reserved.
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

//#define DEBUG

#include <linux/errno.h>
#include <linux/module.h>
#include <mach/gpio.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>


#ifdef DEBUG

#define CPU_ERROR_DEBUG(fmt, ...) do {                     \
    printk(KERN_DEBUG "%s:%d " fmt "\n",            \
           __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
  } while (0)

#else /* !DEBUG */

#define CPU_ERROR_DEBUG(fmt, ...)
#endif

#define CPU_MCA_GPIO 13
#define CPU_AER_0_GPIO 14
#define CPU_AER_1_GPIO 15
#define CPU_AER_2_GPIO 208

#define MAC_ERROR_POLL_TIME 10 //10s

static struct gpio_data_struct {
	int gpio;
	int irq;
};

static struct cpu_error_struct {
	struct gpio_data_struct mca_error;
	struct gpio_data_struct aer_0_error;
	struct gpio_data_struct aer_1_error;
	struct gpio_data_struct aer_2_error;
};

static struct cpu_error_struct *cpu_error;
static int mca_low = 0;
static int mca_high = 0;
static struct workqueue_struct *mca_wq = NULL;
static struct delayed_work mca_dwq;

static int aer_0_assert = 0;
static int aer_1_assert = 0;
static int aer_2_assert = 0;
static int mca_assert = 0;

static int error_assert = 0;

static irqreturn_t cpu_error_handler(int irq, void *dev_id)
{
	int gpio;
	int value;

	gpio = irq_to_gpio(irq);
	value = ast_get_gpio_value(gpio);
	CPU_ERROR_DEBUG("irq=%d, gpio=%d, value=%d\n", irq, gpio, value);
	switch(gpio) {
		case CPU_AER_0_GPIO:
			if(value) {
                if(aer_0_assert == 1)
				    printk(KERN_WARNING "ERR[0] is recovered\n");
                aer_0_assert = 0;
            } else {
				printk(KERN_CRIT "ERR[0] asserted, CPU hardware correctable error detected\n");
                aer_0_assert = 1;
            }
			break;
		case CPU_AER_1_GPIO:
			if(value) {
                if(aer_1_assert == 1)
				    printk(KERN_WARNING "ERR[1] is recovered\n");
                aer_1_assert = 0;
            } else {
				printk(KERN_CRIT "ERR[1] asserted, Non-fatal error detected\n");
                aer_1_assert = 1;
            }
			break;
		case CPU_AER_2_GPIO:
			if(value) {
                if(aer_2_assert == 1)
				    printk(KERN_WARNING "ERR[2] is recovered\n");
                aer_2_assert = 0;
            } else {
				printk(KERN_CRIT "ERR[2] asserted, Fatal error detected\n");
                aer_2_assert = 1;
            }
			break;
		case CPU_MCA_GPIO:
			if(value) {
				mca_high++;
			} else {
				mca_low++;
			}
			break;
	}

	return IRQ_HANDLED;
}

static void mca_error_delay_worker(struct work_struct *work)
{
	struct workqueue_struct *wq = mca_wq;

	if(mca_low == 1) {
		printk(KERN_CRIT "CPU IERR detected\n");
        mca_assert = 1;
	} else if(mca_low >= 8) {
		printk(KERN_CRIT "CPU MCERR detected\n");
        mca_assert = 1;
	}

	if(mca_high == 1) {
        if(mca_assert == 1)
		    printk(KERN_WARNING "CPU MCERR/IERR is recovered\n");
        mca_assert = 0;
	}
	mca_low = 0;
	mca_high = 0;

	queue_delayed_work(wq, work, MAC_ERROR_POLL_TIME * HZ);

	return;
}


static int gpio_data_init(struct cpu_error_struct *cpu_error)
{
	int ret;

	if(!cpu_error) {
		printk(KERN_ERR "%s: cpu error gpio data is null!\n", __func__);
		return -1;
	}
	cpu_error->mca_error.gpio = CPU_MCA_GPIO;
	cpu_error->mca_error.irq = gpio_to_irq(CPU_MCA_GPIO);
	CPU_ERROR_DEBUG("mca_error gpio=%d, irq = %d\n", cpu_error->mca_error.gpio, cpu_error->mca_error.irq);
	ret = request_irq(cpu_error->mca_error.irq, cpu_error_handler , IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "MCA error interrupt", NULL);
	if(ret) {
		printk(KERN_ERR "request irq %d IRQF_TRIGGER_FALLING error: %d!\n", cpu_error->mca_error.irq, ret);
		return -1;
	}

	cpu_error->aer_0_error.gpio = CPU_AER_0_GPIO;
	cpu_error->aer_0_error.irq = gpio_to_irq(CPU_AER_0_GPIO);
	CPU_ERROR_DEBUG("aer_0_error gpio=%d, irq = %d\n", cpu_error->aer_0_error.gpio, cpu_error->aer_0_error.irq);
	ret = request_irq(cpu_error->aer_0_error.irq, cpu_error_handler , IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "AER_0 error interrupt", NULL);
	if(ret) {
		printk(KERN_ERR "request irq %d IRQF_TRIGGER_FALLING error: %d!\n", cpu_error->aer_0_error.irq, ret);
		return -1;
	}

	cpu_error->aer_1_error.gpio = CPU_AER_1_GPIO;
	cpu_error->aer_1_error.irq = gpio_to_irq(CPU_AER_1_GPIO);
	CPU_ERROR_DEBUG("aer_1_error gpio=%d, irq = %d\n", cpu_error->aer_1_error.gpio, cpu_error->aer_1_error.irq);
	ret = request_irq(cpu_error->aer_1_error.irq, cpu_error_handler , IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "AER_1 error interrupt", NULL);
	if(ret) {
		printk(KERN_ERR "request irq %d error: %d!\n", cpu_error->aer_1_error.irq, ret);
		return -1;
	}

	cpu_error->aer_2_error.gpio = CPU_AER_2_GPIO;
	cpu_error->aer_2_error.irq = gpio_to_irq(CPU_AER_2_GPIO);
	CPU_ERROR_DEBUG("aer_2_error gpio=%d, irq = %d\n", cpu_error->aer_2_error.gpio, cpu_error->aer_2_error.irq);
	ret = request_irq(cpu_error->aer_2_error.irq, cpu_error_handler , IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "AER_2 error interrupt", NULL);
	if(ret) {
		printk(KERN_ERR "request irq %d IRQF_TRIGGER_FALLING error: %d!\n", cpu_error->aer_2_error.irq, ret);
		return -1;
	}

	return 0;
}

static ssize_t cpu_error_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{

	error_assert = (aer_0_assert || aer_1_assert || aer_2_assert || mca_assert);

	return sprintf(buf, "%d\n", error_assert);
}

static ssize_t cpu_error_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	int val;

	ret = kstrtol(buf, 0, &val);

	if(ret != 0) {
		return -EFAULT;
	}
	error_assert = val;

	return count;
}

static DEVICE_ATTR(error, S_IRUGO | S_IWUSR,
		cpu_error_show, cpu_error_store);

static struct attribute *cpu_error_attrs[] = {
	 &dev_attr_error.attr,
	 NULL
};
static struct attribute_group cpu_error_attr_group = {
     .attrs = cpu_error_attrs,
};


static const struct file_operations cpu_error_fops = {
	.owner          = THIS_MODULE,
	//.read           = cpu_error_read,
	//.write          = cpu_error_write,
};

static struct miscdevice cpu_error_misc_device = {
	.minor    = MISC_DYNAMIC_MINOR,
	.name     = "cpu_error",
	.fops     = &cpu_error_fops,
};

static int __init cpu_error_init(void)
{
	int ret;
	int irqno;

	printk(KERN_INFO "%s: run\n", __func__);
	cpu_error = (struct cpu_error_struct *)kzalloc(sizeof(struct cpu_error_struct), GFP_KERNEL);
	if(!cpu_error) {
		printk(KERN_ERR "malloc struct cpu_error_struct error!\n");
		return -1;
	}

	if(gpio_data_init(cpu_error) < 0)
		return -1;

	mca_wq = create_workqueue("CPU error workqueue");
	if (mca_wq == NULL) {
		printk(KERN_ERR "%s: error creating CPU error workqueue\n", __func__);
		return -1;
	} else {
		INIT_DELAYED_WORK(&mca_dwq, mca_error_delay_worker);
		queue_delayed_work(mca_wq, &mca_dwq, MAC_ERROR_POLL_TIME * HZ);
	}

	ret = misc_register(&cpu_error_misc_device);
	if (ret) {
		printk(KERN_ERR "%s: unable to register a misc device, ret=%d\n", __func__, ret);
		return ret;
	}
	ret = sysfs_create_group(&cpu_error_misc_device.this_device->kobj, &cpu_error_attr_group);
	if (ret) {
		printk(KERN_ERR "%s: create sysfs node error, ret=%d\n", __func__, ret);
		return ret;
	}

	CPU_ERROR_DEBUG("done\n");
	return 0;
}

static void __exit cpu_error_exit(void)
{
	kfree(cpu_error);
	sysfs_remove_group(&cpu_error_misc_device.this_device->kobj, &cpu_error_attr_group);
	misc_deregister(&cpu_error_misc_device);
	return ;
}

MODULE_AUTHOR("Mickey Zhan");
MODULE_DESCRIPTION("CPU Error Driver");
MODULE_LICENSE("GPL");

module_init(cpu_error_init);
module_exit(cpu_error_exit);
