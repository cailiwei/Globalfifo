/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/slab.h>

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/proc_fs.h>

#include <cdev/globalfifo.h>

#define FIFO_CLEAR 			0x01 	/* 清零全局内存的长度 */
#define GLOBALFIFO_MAJOR 	0 		/* 200 预设的levy_fifo的主设备号 */

struct levy_fifo_dev *levy_fifo_devp = NULL;
static struct class *levy_fifo_class = NULL;

static int levy_fifo_major = GLOBALFIFO_MAJOR;
static int levy_fifo_minor = 0;

/*
 * param_put: put param array to fifo_dev.
 *
 * @param src: the param array
 * @param cnt: the src array lenth
 * @param dst: store the src array
 *
 * @return
 */
struct levy_fifo_dev *param_put(struct param_dev *src, int cnt, struct levy_fifo_dev *dst)
{
	int i, j, step = 0;
	struct param_dev *parp;
	struct levy_fifo_dev *temp;

	if ((!dst) || (!src)) {
		printk("clw: can't get struct\n");
		return 0;
	}
	if(down_interruptible(&(dst->sem))) {
		return 0;
	}

	parp = src;
	temp = dst;
	/* 保存数组的起始位置和大小 */
	dst->val = parp;
	dst->count = cnt;
	for (i = 0; i < cnt; i++) {

		/* 将每个数组的元素保存到全局内存中 */
		for (j = 0; j < (parp+i)->size; j++) {
			dst->mem[step] = (parp+i)->param[j];
			printk("0x%02x,", dst->mem[step]);
			step++;
		}
		printk("\n");
	}
	/* 保存当前长度 */
	dst->current_len = step;
	up(&(dst->sem));

	return temp;
}

/*
 * param_get: get param array from fifo_dev.
 *
 * @param src: the fifo_dev data
 * @param dst: store the src array
 *
 * @return
 */
struct param_dev *param_get(struct param_dev *dst, struct levy_fifo_dev *src)
{
	int i, j = 0, step = 0;
	struct param_dev *temp;

	if ((!dst) || (!src)) {
		printk("clw: can't get struct\n");
		return 0;
	}
	if(down_interruptible(&(src->sem))) {
		return 0;
	}
	temp = dst;
	step = src->current_len;
//	printk("step: %d\n", step);
	for (i = 0; i < step; i++) {

		dst->param[j] = src->mem[i];
		printk("0x%02x,", dst->param[j]);
		if (++j == dst->size) {
			j = 0;
			printk("---%d---\n", dst->size);
			if(!++dst) break;
		}
	}
	up(&(src->sem));

	return temp;
}

/*
 * levy_fifo_open:
 */
int levy_fifo_open(struct inode *inode, struct file *filp)
{
	struct levy_fifo_dev* dev;

	dev = container_of(inode->i_cdev, struct levy_fifo_dev, dev);
	filp->private_data = dev;

	return 0;
}

/*
 * levy_fifo_fasync:
 *
 */
static int levy_fifo_fasync(int fd, struct file *filp, int mode)
{
	struct levy_fifo_dev *dev = filp->private_data;
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}

/*
 * levy_fifo_release:
 *
 */
int levy_fifo_release(struct inode *inode, struct file *filp)
{
	/* 将文件从异步通知列表中删除 */
	levy_fifo_fasync(-1, filp, 0);

	return 0;
}

/*
 * levy_fifo_ioctl:
 *
 */
static long levy_fifo_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct levy_fifo_dev *dev = filp->private_data;

	switch(cmd)
	{
		case FIFO_CLEAR:
			down(&(dev->sem));
			dev->current_len = 0;
			memset(dev->mem, 0, GLOBALFIFO_SIZE);
			up(&(dev->sem));

			printk(KERN_INFO "clw: ---> levy_fifo is set to zero \n");
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

/*
 * levy_fifo_poll:
 *
 */
static unsigned int levy_fifo_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	struct levy_fifo_dev *dev = filp->private_data;

	down(&(dev->sem));

	poll_wait(filp, &dev->r_wait, wait);
	poll_wait(filp, &dev->w_wait, wait);

	/*fifo非空*/
	if (dev->current_len != 0)
	{
		mask |= POLLIN | POLLRDNORM; /*标示数据可获得*/
	}
	/* fifo非满*/
	if (dev->current_len != GLOBALFIFO_SIZE)
	{
		mask |= POLLOUT | POLLWRNORM; /* 标示数据可写入*/
	}

	up(&(dev->sem));
	return mask;
}

/*
 * levy_fifo_read:
 *
 */
static ssize_t levy_fifo_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	int ret;
	struct levy_fifo_dev *dev = filp->private_data;
	struct param_dev *parp = NULL;
	unsigned char step = dev->current_len, i = 0, cnt = dev->count; /* store the data array size */
	unsigned char array[cnt + 2];

	DECLARE_WAITQUEUE(wait, current); /* 定义等待队列 */

	down(&(dev->sem));
	add_wait_queue(&dev->r_wait,&wait); /* 进入读等待队列头 */

	/* 等待FIFO非空 */
	if (dev->current_len == 0) {
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto out;
		}

		__set_current_state(TASK_INTERRUPTIBLE);
		up(&(dev->sem));

		schedule(); /* 调度其他进程执行 */
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			goto out2;
		}
		down(&dev->sem); /* 获取信号量 */
	}

	parp = dev->val;

	array[i++] = cnt;
	while (i <= cnt) array[i++] = (parp++)->size;

	if (count > step) count = step;
	array[i] = count;

	if (copy_to_user(buf, array, sizeof(array))) {
		ret = -EFAULT;
		goto out;
	}

	buf += sizeof(array);
	if (copy_to_user(buf, dev->mem, count)) {
		ret = -EFAULT;
		goto out;
	} else {
		memcpy(dev->mem, dev->mem + count, dev->current_len - count);
		dev->current_len -= count;
		printk("clw: Read %d bytes(s), current_len: %d\n", count, dev->current_len);
		wake_up_interruptible(&dev->w_wait); /*唤醒写等待队列*/
		ret = count + sizeof(array);
	}

out:
	up(&(dev->sem));
out2:
	remove_wait_queue(&dev->w_wait, &wait);
	set_current_state(TASK_RUNNING);
	return ret;
}

/*
 * levy_fifo_write:
 *
 */
static ssize_t levy_fifo_write(struct file *filp,const char __user *buf,
		size_t count,loff_t *ppos)
{
	int ret ;
	struct levy_fifo_dev *dev = filp->private_data;
	DECLARE_WAITQUEUE(wait, current);
	down(&(dev->sem));
	add_wait_queue(&dev->w_wait, &wait);

	/* 等待FIFO非满 */
	if (dev->current_len == GLOBALFIFO_SIZE) {
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto out;
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		up(&(dev->sem));

		schedule();
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			goto out2;
		}
		down(&(dev->sem));
	}

	if (count > GLOBALFIFO_SIZE - dev->current_len)
		count = GLOBALFIFO_SIZE - dev->current_len;
	if (copy_from_user(dev->mem + dev->current_len, buf, count)) {
		ret = -EFAULT;
		goto out;
	} else {
		dev->current_len += count;
		printk("clw: Written %d bytes, current_len:%d\n", count,dev->current_len);
		wake_up_interruptible(&dev->r_wait);

		/* 产生异步读信号*/
		if (dev->async_queue)
			kill_fasync(&dev->async_queue, SIGIO, POLL_IN);

		ret = count;
	}
out:
	up(&(dev->sem));
out2:
	remove_wait_queue(&dev->w_wait, &wait);
	set_current_state(TASK_RUNNING);
	return ret;
}

static const struct file_operations levy_fifo_fops = {
	.owner =THIS_MODULE,
	.read = levy_fifo_read,
	.write = levy_fifo_write,
	.unlocked_ioctl = levy_fifo_ioctl,
	.poll = levy_fifo_poll,
	.open = levy_fifo_open,
	.release = levy_fifo_release,
	.fasync = levy_fifo_fasync,
};

/*
 * __levy_fifo_dev_get_mem: 读取寄存器mem的值到缓冲区buf中，内部使用
 *
 * @param dev: the globalfifo levy_fifo_dev
 * @param buf: the 'cat' display buffer
 *
 * @return display buffer size
 */
static ssize_t __levy_fifo_dev_get_mem(struct levy_fifo_dev* dev, unsigned char* buf)
{
	int i = 0, j = 0, num = 0, ret = 0, step = 0;
	unsigned char *levy_fifo_dev_mem = NULL;
	struct param_dev *parp;
	int temp = 0;

	if(down_interruptible(&(dev->sem))) {
		return -ERESTARTSYS;
	}

	levy_fifo_dev_mem = &(dev->mem[0]);
	parp = dev->val;
	step = dev->current_len;
//	printk("step: %d\n", step);
	if (parp && step) {
		ret += sprintf(buf, "Read the LCD 0-%d params:\nArray%d[%d]: ", step-1, num, parp->size);
		buf = buf + ret;
		for (i = 0; i < step; i++) {
			ret += sprintf(buf, "0x%02x,", dev->mem[i]);
			buf = buf + 5;
			if (++j == parp->size) {
				j = 0;
				if (i == step - 1) break;
				if (temp < step) {
					temp += (parp++)->size;
					ret += sprintf(buf, "\nArray%d[%d]: ", ++num, parp->size);
					buf = buf + 12;
				};
			}
		}
	} else {
		ret += sprintf(buf, "The levy fifo is empty!!!\nPrint the empty data:\n");
		buf = buf + ret;
		for (i = 0; i < GLOBALFIFO_SIZE; i++) {
			ret += snprintf(buf, PAGE_SIZE, "0x%02x,", *levy_fifo_dev_mem);
			levy_fifo_dev_mem++;
			buf = buf + 5;
			if ((i + 1) % 4 == 0) {
				ret += sprintf(buf, "\t");
				buf++;
			}
			if ((i + 1) % 8 == 0) {
				ret += sprintf(buf, "\n");
				buf++;
			}
		}
	}
	ret += sprintf(buf, "\n");
	up(&(dev->sem));

	return /*ret > PAGE_SIZE ? PAGE_SIZE :*/ret;
}

/*
 * __levy_fifo_dev_get_mem: 把缓冲区buf的值写到设备寄存器mem中去，内部使用
 *
 * @param dev: the globalfifo levy_fifo_dev
 * @param buf: the 'echo' string value
 * @param count: the string size
 *
 * @return
 */
static ssize_t __levy_fifo_dev_set_mem(struct levy_fifo_dev* dev, const unsigned char *buf, size_t count) {
	unsigned char num[3] = { 0 }, command = 0;
	int temp = 0, lenth = 0, no = 0, i = 0, j = GLOBALFIFO_SIZE;
	unsigned char *t = num;

	printk("%s: count:%d, buf: %s", __func__, count, buf);
	if (down_interruptible(&(dev->sem))) {
		return -ERESTARTSYS;
	}

	if ( strlen(buf) < PAGE_SIZE)
		lenth = strlen(buf);
	else
		lenth = PAGE_SIZE;

	if (*buf++ == 'w') {

		command = *buf++;
		while(*buf == ' ') buf++;

		do {
			if (i > 4)
				printk("The input num is too large!!!\n");
			else
				num[i++] = *buf++;
		} while (*buf != ' ');

		while(*buf == ' ') buf++;

		sprintf(t, "%s", num);
//		printk("clw: num:%s, t:%s, buf:%s\n",num, t, buf);

		no = simple_strtoul(t, NULL, 0);
		temp = simple_strtoul(buf, NULL, 0);
//		printk("clw: no:%d, temp:%d\n", no, temp);
		switch (command) {
			case 'n':
				if (no <= GLOBALFIFO_SIZE) {
					dev->mem[no] = (unsigned char)temp;
					printk("clw: mem[%d]=0x%02x\n", no, temp);
				} else
					printk("Given num is too big!!!\n");
				break;
			case 'r':
				do {
					j--;
				} while ((j >= 0) && (no != dev->mem[j]));

				printk("clw: mem[%d]=0x%02x ==> mem[%d]=0x%02x\n", j, no, j, temp);
				if (j > 0)
					dev->mem[j] = (unsigned char)temp;
				else
					printk("clw: Can't find the reg: 0x%02x!!!\n", no);
				break;
			default:
				printk("clw: Input error!!!\n");
		}
	} else {
		printk("clw: Input error!!!\n");
	}
	up(&(dev->sem));

	return count;
}

static ssize_t levy_fifo_dev_mem_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	struct levy_fifo_dev* gdev = (struct levy_fifo_dev*)dev_get_drvdata(dev);

	return __levy_fifo_dev_get_mem(gdev, buf);
}

static ssize_t levy_fifo_dev_mem_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	struct levy_fifo_dev* gdev = (struct levy_fifo_dev*)dev_get_drvdata(dev);

	return __levy_fifo_dev_set_mem(gdev, buf, count);
}
static DEVICE_ATTR(mem_param, S_IRUGO|S_IWUSR, levy_fifo_dev_mem_show, levy_fifo_dev_mem_store);

static ssize_t levy_fifo_dev_proc_read(char* page, char** start, off_t offset, int count, int* eof, void* data)
{
	if (offset > 0)	{
		*eof = 1;
		return 0;
	}

	return __levy_fifo_dev_get_mem(levy_fifo_devp, page);
}
static ssize_t levy_fifo_dev_proc_write(struct file* filp, const char __user *buff, unsigned long len, void* data)
{
	int err = 0;
	char* page = NULL;

	if (len > GLOBALFIFO_SIZE) {
		printk(KERN_ALERT "%s: The buff is too large: %lu.\n", __func__, len);
		return -EFAULT;
	}

	page = (char*)__get_free_page(GFP_KERNEL);
	if (!page) {
		printk(KERN_ALERT "%s: Failed to alloc page.\n", __func__);
		return -ENOMEM;
	}

	if(copy_from_user(page, buff, len)) {
		printk(KERN_ALERT "%s: Failed to copy buff from user.\n", __func__);
		err = -EFAULT;
		goto out;
	}

	err = __levy_fifo_dev_set_mem(levy_fifo_devp, page, len);

out:
	free_page((unsigned long)page);
	return err;
}

static void levy_fifo_dev_create_proc(void) {

	struct proc_dir_entry* entry;
	entry = create_proc_entry("levy_fifo", S_IRUGO | S_IWUSR | S_IWGRP, NULL);
	if (entry) {
		entry->data = NULL;
		entry->read_proc = levy_fifo_dev_proc_read;
		entry->write_proc = levy_fifo_dev_proc_write;
	}
}
static void levy_fifo_dev_remove_proc(void) {
	remove_proc_entry("levy_fifo", NULL);
}

static int __levy_fifo_setup_cdev(struct levy_fifo_dev *dev, int index)
{
	int err, devno = MKDEV(levy_fifo_major, index);
//	int i;

	cdev_init(&(dev->dev), &levy_fifo_fops);
	dev->dev.owner = THIS_MODULE;
	dev->dev.ops = &levy_fifo_fops;

	err = cdev_add(&(dev->dev), devno, 1);
	if (err) {
		printk(KERN_INFO "%s: Error adding levy_fifo dev.\n", __func__);
		return err;
	}

//	for (i = 0; i < GLOBALFIFO_SIZE; i++) {
//		dev->mem[i] = i % 256;
//	}
//	dev->current_len = GLOBALFIFO_SIZE;
//	init_MUTEX(&(levy_fifo_devp->sem)); /*初始化信号量*/
	sema_init(&(dev->sem), 1); /*初始化信号量*/
	init_waitqueue_head(&dev->r_wait); /*初始化读等待队列头*/
	init_waitqueue_head(&dev->w_wait); /*初始化写等待队列头*/

	return 0;
}

static int __init levy_fifo_init(void)
{
	int ret = -1;
	struct device* temp = NULL;
	dev_t devno = 0;

	if (levy_fifo_major) {
		devno = MKDEV(levy_fifo_major, levy_fifo_minor);
		ret = register_chrdev_region(devno, 1, "levy_fifo");
	} else { /*动态申请设备号*/
		ret = alloc_chrdev_region(&devno, 0, 1, "levy_fifo");
		levy_fifo_major = MAJOR(devno);
		levy_fifo_minor = MINOR(devno);
	}
	if (ret < 0) {
		printk(KERN_ALERT "%s: Failed to alloc char dev region.\n", __func__);
		goto fail;
	}

	levy_fifo_devp = kmalloc(sizeof(struct levy_fifo_dev), GFP_KERNEL);
	if (!levy_fifo_devp) {
		ret = -ENOMEM;
		printk(KERN_ALERT "%s: Failed to alloc levy_fifo_dev.\n", __func__);
		goto unregister;
	}

	memset(levy_fifo_devp, 0, sizeof(struct levy_fifo_dev));
	if (__levy_fifo_setup_cdev(levy_fifo_devp, levy_fifo_minor))
		goto cleanup;

	/*在/sys/class/目录下创建设备类别目录levy_fifo*/
	levy_fifo_class = class_create(THIS_MODULE, "levy_fifo");
	if (IS_ERR(levy_fifo_class)) {
		ret = PTR_ERR(levy_fifo_class);
		printk(KERN_ALERT "%s: Failed to alloc levy_fifo class.\n", __func__);
		goto destroy_cdev;
	}

	/*在/dev/目录和/sys/class/levy_fifo目录下分别创建设备文件param_fifo*/
	temp = device_create(levy_fifo_class, NULL, devno, "%s", "param_fifo");
	if (IS_ERR(temp)) {
		ret = PTR_ERR(temp);
		printk(KERN_ALERT "%s: Failed to alloc levy_fifo device.\n", __func__);
		goto destroy_class;
	}

	/*在/sys/class/levy_fifo/levy_fifo目录下创建属性文件mem_param*/
	ret = device_create_file(temp, &dev_attr_mem_param);
	if (ret < 0) {
		printk(KERN_ALERT "%s: Failed to alloc create attribute mem.\n", __func__);
		goto destroy_device;
	}
	dev_set_drvdata(temp, levy_fifo_devp);

	/* 创建/proc/levy_fifo */
	levy_fifo_dev_create_proc();
	printk(KERN_INFO "%s: Succedded to initialize levy_fifo device.\n", __func__);
	return 0;

destroy_device:
	device_destroy(levy_fifo_class, devno);

destroy_class:
	class_destroy(levy_fifo_class);

destroy_cdev:
	cdev_del(&(levy_fifo_devp->dev));

cleanup:
	kfree(levy_fifo_devp);

unregister:
	unregister_chrdev_region(devno, 1);

fail:
	return ret;
}

static void __exit levy_fifo_exit(void) {

	/* delete/proc/levy_fifo */
	levy_fifo_dev_remove_proc();

	if (levy_fifo_class) {
		device_destroy(levy_fifo_class, MKDEV(levy_fifo_major, levy_fifo_minor));
		class_destroy(levy_fifo_class);
	}
	if (levy_fifo_devp)	{
		cdev_del(&(levy_fifo_devp->dev));
		kfree(levy_fifo_devp);
	}
	unregister_chrdev_region(MKDEV(levy_fifo_major, levy_fifo_minor), 1);
	printk("clw: ---> %s\n", __func__);
}

module_param(levy_fifo_major, int, S_IRUGO);

module_init(levy_fifo_init);
module_exit(levy_fifo_exit);

MODULE_AUTHOR("liwei.cai<liwei.cai@feixun.com.cn>");
MODULE_DESCRIPTION("levy_fifo module driver");
MODULE_LICENSE("GPL v2");
