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

#define GLOBALFIFO_SIZE 		0x1000	/* TODO change the param size 4k */

struct param_dev {
	int size; /* the param size */
	unsigned char *param; /* store the param */
};

struct levy_fifo_dev {
	struct cdev dev; 					/* cdev结构体 */
	unsigned int current_len;  			/* fifo有效数据长度 */
	unsigned char mem[GLOBALFIFO_SIZE]; /* 全局内存 */
	struct param_dev *val;				/* 全局内存的指针 */
	unsigned int count;					/* 数据分组的指针数组个数 */
	struct semaphore sem;  				/* 并发控制用的信号量 */
	wait_queue_head_t r_wait;  			/* 阻塞读用的等待队列头 */
	wait_queue_head_t w_wait;  			/* 阻塞写用的等待队列头 */
	struct fasync_struct *async_queue; 	/* 异步结构体指针，用于读 */
};

extern struct levy_fifo_dev *levy_fifo_devp;

struct levy_fifo_dev *param_put(struct param_dev *src, int cnt, struct levy_fifo_dev *dst);
struct param_dev *param_get(struct param_dev *dst, struct levy_fifo_dev *src);

#endif
