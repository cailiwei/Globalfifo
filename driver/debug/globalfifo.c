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

#include <linux/types.h>
#include <linux/device.h>
#include <linux/proc_fs.h>

#include <cdev/fifo_cdev.h>


struct fifo_cdev *fifo_cdevp = NULL; /* The globalfifo point ptr */

static struct class *fifo_cdev_class = NULL;

static int fifo_cdev_major = GLOBALFIFO_MAJOR;
static int fifo_cdev_minor = 0;

/*
 * fifo_cdev_open:
 */
int fifo_cdev_open(struct inode *inode, struct file *filp)
{
	struct fifo_cdev* dev;

	dev = container_of(inode->i_cdev, struct fifo_cdev, dev);
	filp->private_data = dev;

	return 0;
}

/*
 * fifo_cdev_fasync:
 *
 */
static int fifo_cdev_fasync(int fd, struct file *filp, int mode)
{
	struct fifo_cdev *dev = filp->private_data;
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}

/*
 * fifo_cdev_release:
 *
 */
int fifo_cdev_release(struct inode *inode, struct file *filp)
{
	fifo_cdev_fasync(-1, filp, 0);

	return 0;
}

/*
 * fifo_cdev_ioctl:
 *
 */
static long fifo_cdev_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct fifo_cdev *dev = filp->private_data;

	switch(cmd)
	{
		case FIFO_CLEAR:
			down(&(dev->sem));
			dev->current_len = 0;
			memset(dev->mem, 0, GLOBALFIFO_SIZE);
			up(&(dev->sem));

			printk(KERN_INFO "clw: %s set to zero \n", __func__);
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

/*
 * fifo_cdev_poll:
 *
 */
static unsigned int fifo_cdev_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	struct fifo_cdev *dev = filp->private_data;

	down(&(dev->sem));

	poll_wait(filp, &dev->r_wait, wait);
	poll_wait(filp, &dev->w_wait, wait);

	if (dev->current_len != 0)
	{
		mask |= POLLIN | POLLRDNORM; /*标示数据可获得*/
	}
	if (dev->current_len != GLOBALFIFO_SIZE)
	{
		mask |= POLLOUT | POLLWRNORM; /* 标示数据可写入*/
	}

	up(&(dev->sem));
	return mask;
}

/*
 * fifo_cdev_read:
 *
 */
static ssize_t fifo_cdev_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	int ret;
	struct fifo_cdev *dev = filp->private_data;
	unsigned int step = dev->current_len;

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

	if (count > step) count = step;

	/* Read the total data */
	count = step;
	if (copy_to_user(buf, dev->mem, count))
	{
		ret = -EFAULT;
		goto out;
	} else {
		memcpy(dev->mem, dev->mem + count, dev->current_len - count);
		dev->current_len -= count;
#ifdef FIFO_CDEV_DEBUG
		printk("clw: Read %d bytes(s), current_len: %d\n", 
				count, dev->current_len);
#endif
		wake_up_interruptible(&dev->w_wait); 
		ret = count;
	}

out:
	up(&(dev->sem));
out2:
	remove_wait_queue(&dev->w_wait, &wait);
	set_current_state(TASK_RUNNING);
	return ret;
}

/*
 * fifo_cdev_write:
 *
 */
static ssize_t fifo_cdev_write(struct file *filp,const char __user *buf,
		size_t count,loff_t *ppos)
{
	int ret ;
	struct fifo_cdev *dev = filp->private_data;
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
	if (copy_from_user(dev->mem + dev->current_len, buf, count))
	{
		ret = -EFAULT;
		goto out;
	} else {
		dev->current_len += count;
#ifdef FIFO_CDEV_DEBUG
		printk("clw: Written %d bytes, current_len:%d\n",
				count,dev->current_len);
#endif
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

static const struct file_operations fifo_cdev_fops = {
	.owner = THIS_MODULE,
	.read  = fifo_cdev_read,
	.write = fifo_cdev_write,
	.unlocked_ioctl = fifo_cdev_ioctl,
	.poll = fifo_cdev_poll,
	.open = fifo_cdev_open,
	.release = fifo_cdev_release,
	.fasync = fifo_cdev_fasync,
};

/*
 * __fifo_cdev_get_mem: 读取寄存器mem的值到缓冲区buf中，内部使用
 *
 * @param dev: the globalfifo fifo_cdev
 * @param buf: the 'cat' display buffer
 *
 * @return display buffer size
 */
static ssize_t __fifo_cdev_get_mem(struct fifo_cdev* dev, unsigned char* buf)
{
	int i = 0, j = 0, num = 0, cnt = 0, ret = 0, step = 0;
	unsigned char *fifo_cdev_mem = NULL;
	struct param_dev *parp = NULL;
	int temp = 0;

	if(down_interruptible(&(dev->sem))) {
		return -ERESTARTSYS;
	}
	if (!(dev->params_info)) {
		printk("clw: Could not get param_info struct\n");
		return -ERESTARTSYS;
	}

	fifo_cdev_mem = &(dev->mem[0]);

	parp = dev->params_info->cmds;

	step = dev->current_len;
	cnt = dev->mem[0];
//	printk("step: %d, cnt = %d\n", step, cnt);
	if (parp && step && cnt) {
		ret += sprintf(buf + ret, "Read the LCD 0-%02d params:\nArray%02d[%02d]: ",
			   step - cnt - 3, num, parp->size);

		for (i = cnt + 2; i < step; i++) {
			ret += sprintf(buf + ret, "0x%02x,", dev->mem[i]);

			if (++j == parp->size) {
				j = 0;
				if (i == step - 1) break;

				if (temp < step) {
					temp += (parp++)->size;
					ret += sprintf(buf + ret, "\nArray%02d[%02d]: ",
						   ++num, parp->size);
				};
			}
		}
	} else {
		ret += sprintf(buf + ret, "The levy fifo is empty!!!\n\
				Print the empty data:\n");
		for (i = 0; i < GLOBALFIFO_SIZE; i++) {
			ret += snprintf(buf + ret, PAGE_SIZE, "0x%02x,", *fifo_cdev_mem);

			fifo_cdev_mem++;
			if ((i + 1) % 4 == 0) {
				ret += sprintf(buf + ret, "\t");
			}

			if ((i + 1) % 8 == 0) {
				ret += sprintf(buf + ret, "\n");
			}
		}
	}

	buf[ret - 1] = '\n';

	up(&(dev->sem));
	return ret > PAGE_SIZE ? PAGE_SIZE : ret;
}

/*
 * __fifo_cdev_get_mem: 把缓冲区buf的值写到设备寄存器mem中去，内部使用
 *
 * @param dev: the globalfifo fifo_cdev
 * @param buf: the 'echo' string value
 * @param count: the string size
 *
 * @return
 */
static ssize_t __fifo_cdev_set_mem(struct fifo_cdev* dev,
	   	const unsigned char *buf, size_t count) 
{
	unsigned char num[3] = { 0 }, command = 0;
	int temp = 0, lenth = 0, no = 0, i = 0, params_cnt = 0;
	unsigned char *t = num;

#ifdef FIFO_CDEV_DEBUG
	printk("%s: count:%d, buf: %s", __func__, count, buf);
#endif
	if (down_interruptible(&(dev->sem))) {
		return -ERESTARTSYS;
	}

	if ( strlen(buf) < PAGE_SIZE)
		lenth = strlen(buf);
	else
		lenth = PAGE_SIZE;

	params_cnt = dev->mem[0] + 1;

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
#ifdef FIFO_CDEV_DEBUG
//		printk("clw: num:%s, t:%s, buf:%s\n",num, t, buf);
#endif
		no = simple_strtoul(t, NULL, 0);
		temp = simple_strtoul(buf, NULL, 0);
//		printk("clw: no:%d, temp:%d\n", no, temp);
		switch (command) {
			case 'n':
				/* step to data array */
				no += params_cnt + 1;
				if (no <= GLOBALFIFO_SIZE) {
					dev->mem[no] = (unsigned char)temp;
#ifdef FIFO_CDEV_DEBUG
					printk("clw: mem[%d]=0x%02x\n", no, temp);
#endif
				} else
					printk("Given num is too big!!!\n");
				break;
			case 'r':
				i = dev->current_len;
				do {
					i--;
				} while ((i >= params_cnt) && (no != dev->mem[i]));

#ifdef FIFO_CDEV_DEBUG
				printk("clw: mem[%d]=0x%02x ==> mem[%d]=0x%02x\n", 
						i, no, i, temp);
#endif
				if (i > params_cnt)
					dev->mem[i] = (unsigned char)temp;
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

static ssize_t fifo_cdev_mem_show(struct device* dev, struct device_attribute* attr, char* buf)
{
	struct fifo_cdev* gdev = (struct fifo_cdev*)dev_get_drvdata(dev);

	return __fifo_cdev_get_mem(gdev, buf);
}

static ssize_t fifo_cdev_mem_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	struct fifo_cdev* gdev = (struct fifo_cdev*)dev_get_drvdata(dev);

	return __fifo_cdev_set_mem(gdev, buf, count);
}
static DEVICE_ATTR(mem_param, S_IRUGO|S_IWUSR, fifo_cdev_mem_show, fifo_cdev_mem_store);

static ssize_t fifo_cdev_proc_read(char* page, char** start, off_t offset, int count, int* eof, void* data)
{
	if (offset > 0)	{
		*eof = 1;
		return 0;
	}

	return __fifo_cdev_get_mem(fifo_cdevp, page);
}
static ssize_t fifo_cdev_proc_write(struct file* filp, const char __user *buff, unsigned long len, void* data)
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

	err = __fifo_cdev_set_mem(fifo_cdevp, page, len);

out:
	free_page((unsigned long)page);
	return err;
}

static void fifo_cdev_create_proc(void) {

	struct proc_dir_entry* entry;
	entry = create_proc_entry("fifo_cdev", S_IRUGO | S_IWUSR | S_IWGRP, NULL);
	if (entry) {
		entry->data = NULL;
		entry->read_proc = fifo_cdev_proc_read;
		entry->write_proc = fifo_cdev_proc_write;
	}
}
static void fifo_cdev_remove_proc(void) {
	remove_proc_entry("fifo_cdev", NULL);
}

static int __fifo_cdev_setup_cdev(struct fifo_cdev *dev, int index)
{
	int err, devno = MKDEV(fifo_cdev_major, index);
//	int i;

	cdev_init(&(dev->dev), &fifo_cdev_fops);
	dev->dev.owner = THIS_MODULE;
	dev->dev.ops = &fifo_cdev_fops;

	err = cdev_add(&(dev->dev), devno, 1);
	if (err) {
		printk(KERN_INFO "%s: Error adding fifo_cdev dev.\n", __func__);
		return err;
	}

//	for (i = 0; i < GLOBALFIFO_SIZE; i++) {
//		dev->mem[i] = i % 256;
//	}
//	dev->current_len = GLOBALFIFO_SIZE;
//	init_MUTEX(&(fifo_cdevp->sem)); /*初始化信号量*/
	sema_init(&(dev->sem), 1); /*初始化信号量*/
	init_waitqueue_head(&dev->r_wait); /*初始化读等待队列头*/
	init_waitqueue_head(&dev->w_wait); /*初始化写等待队列头*/

	return 0;
}

static int __init fifo_cdev_init(void)
{
	int ret = -1;
	struct device* temp = NULL;
	dev_t devno = 0;

	if (fifo_cdev_major) {
		devno = MKDEV(fifo_cdev_major, fifo_cdev_minor);
		ret = register_chrdev_region(devno, 1, "fifo_cdev");
	} else { /*动态申请设备号*/
		ret = alloc_chrdev_region(&devno, 0, 1, "fifo_cdev");
		fifo_cdev_major = MAJOR(devno);
		fifo_cdev_minor = MINOR(devno);
	}
	if (ret < 0) {
		printk(KERN_ALERT "%s: Failed to alloc char dev region.\n",
			   __func__);
		goto fail;
	}

	fifo_cdevp = kmalloc(sizeof(struct fifo_cdev), GFP_KERNEL);
	if (!fifo_cdevp) {
		ret = -ENOMEM;
		printk(KERN_ALERT "%s: Failed to alloc fifo_cdev.\n", __func__);
		goto unregister;
	}

	memset(fifo_cdevp, 0, sizeof(struct fifo_cdev));
	if (__fifo_cdev_setup_cdev(fifo_cdevp, fifo_cdev_minor))
		goto cleanup;

	/*在/sys/class/目录下创建设备类别目录fifo_cdev*/
	fifo_cdev_class = class_create(THIS_MODULE, "fifo_cdev");
	if (IS_ERR(fifo_cdev_class)) {
		ret = PTR_ERR(fifo_cdev_class);
		printk(KERN_ALERT "%s: Failed to alloc fifo_cdev class.\n",
			   __func__);
		goto destroy_cdev;
	}

	/*在/dev/目录和/sys/class/fifo_cdev目录下分别创建param_fifo*/
	temp = device_create(fifo_cdev_class, NULL, devno,
		   	"%s", "param_fifo");
	if (IS_ERR(temp)) {
		ret = PTR_ERR(temp);
		printk(KERN_ALERT "%s: Failed to alloc fifo_cdev device.\n",
			   	__func__);
		goto destroy_class;
	}

	/*在/sys/class/fifo_cdev/fifo_cdev目录下创建属性文件mem_param*/
	ret = device_create_file(temp, &dev_attr_mem_param);
	if (ret < 0) {
		printk(KERN_ALERT "%s: Failed to alloc create attribute mem.\n",
			   	__func__);
		goto destroy_device;
	}
	dev_set_drvdata(temp, fifo_cdevp);

	/* 创建/proc/fifo_cdev */
	fifo_cdev_create_proc();
	printk(KERN_INFO "%s: Succedded to initialize fifo_cdev device.\n", __func__);
	return 0;

destroy_device:
	device_destroy(fifo_cdev_class, devno);

destroy_class:
	class_destroy(fifo_cdev_class);

destroy_cdev:
	cdev_del(&(fifo_cdevp->dev));

cleanup:
	kfree(fifo_cdevp);

unregister:
	unregister_chrdev_region(devno, 1);

fail:
	return ret;
}

static void __exit fifo_cdev_exit(void) {

	/* delete/proc/fifo_cdev */
	fifo_cdev_remove_proc();

	if (fifo_cdev_class) {
		device_destroy(fifo_cdev_class, MKDEV(fifo_cdev_major, fifo_cdev_minor));
		class_destroy(fifo_cdev_class);
	}
	if (fifo_cdevp)	{
		cdev_del(&(fifo_cdevp->dev));
		kfree(fifo_cdevp);
	}
	unregister_chrdev_region(MKDEV(fifo_cdev_major, fifo_cdev_minor), 1);
}

module_param(fifo_cdev_major, int, S_IRUGO);

module_init(fifo_cdev_init);
module_exit(fifo_cdev_exit);

MODULE_AUTHOR("liwei.cai<liwei.cai@feixun.com.cn>");
MODULE_DESCRIPTION("fifo_cdev module driver");
MODULE_LICENSE("GPL v2");
