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

#ifndef _GLOBALFIFO_H_
#define _GLOBALFIFO_H_

#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/wait.h>
#include <linux/fs.h>

#define GLOBALFIFO_SIZE 	0x1000	/* TODO change param size 4k */
#define FIFO_CLEAR 			0x01 	/* 清零全局内存的长度 */
#define GLOBALFIFO_MAJOR 	0 		/* 200 预设的fifo_cdev的主设备号 */

//#define FIFO_CDEV_DEBUG 	1

struct param_dev {
	int size; 				/* the param size */
	unsigned char cmd; 		/* The cmd */
	unsigned char *param; 	/* store the param */
};

struct param_cmds_info {
	int cnt; 				/* store the size of cmds */
	struct param_dev *cmds; /* store the param info */
};

struct fifo_cdev {
	struct cdev dev; 					/* cdev结构体 */
	unsigned int current_len;  			/* fifo有效数据长度 */
	unsigned char mem[GLOBALFIFO_SIZE]; /* 全局内存 */
	struct param_cmds_info *params_info;		/* 全局内存的指针 */
	struct semaphore sem;  				/* 并发控制用的信号量 */
	wait_queue_head_t r_wait;  			/* 阻塞读用的等待队列头 */
	wait_queue_head_t w_wait;  			/* 阻塞写用的等待队列头 */
	struct fasync_struct *async_queue; 	/* 异步结构体指针，用于读 */
};

extern struct fifo_cdev *fifo_cdevp; /* The globalfifo ptr */

extern struct fifo_cdev *put_param_into_cdev(struct param_cmds_info *src,
		struct fifo_cdev *dst);
extern struct param_dev *get_param_from_cdev(struct fifo_cdev *src,
		struct param_cmds_info *dst);

#endif
