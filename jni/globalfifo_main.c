#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

#include "globalfifo.h"

#define DEVICE_NAME "/dev/param_fifo"
#define MODULE_NAME "globalfifo"

static int globalfifo_set_val(struct globalfifo_device_t* dev, unsigned char* val, int count) {

	ALOGD("Globalfifo Stub: set value %d to device.\n", count);

	write(dev->fd, val, count);

	return 0;
}

static int globalfifo_get_val(struct globalfifo_device_t* dev, unsigned char* val, int count) {

	int i, j;
	unsigned char array_num = 0;
	unsigned char *temp = 0;

	if(!val) {
		ALOGE("Globalfifo Stub: error val pointer.\n");
		return -EFAULT;
	}
	read(dev->fd, val, count);
	array_num = *val;
	ALOGD("Globalfifo Stub: get value %d from device:\n",array_num);
	temp = val + *val;
	val++;
	for (i = 0; i < array_num; i++) {
		ALOGD("Array%d[%d]: ", i, *val);
		for (j = 0; j < *val++; j++) {
			ALOGD("0x%02x,", *temp++);	
		}
		ALOGD("\n");
	}
	return 0;
}

struct globalfifo_device_t* globalfifo_device_init()
{
	int result;
	struct globalfifo_device_t* dev = NULL;

	dev = (struct globalfifo_device_t*)malloc(sizeof(struct globalfifo_device_t));
	if (!dev) {
		ALOGE("Globalfifo Stub: failed to alloc space.\n");
		goto fail;
	}
	memset(dev, 0, sizeof(struct globalfifo_device_t));

	result = open(DEVICE_NAME, O_RDWR, 0777);
	if (result < 0) {
		ALOGE("can't open param_fifo%d",result);
		goto fail;
	} else {
		dev->fd = result;
		dev->set_val = &globalfifo_set_val;
		dev->get_val = &globalfifo_get_val;
	}

	return dev;

fail:
	free(dev);
	return dev;
}

