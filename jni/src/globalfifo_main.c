#include "globalfifo.h"

static int globalfifo_set_val(struct globalfifo_device_t* dev, unsigned char* val, int count) {
	int ret = 0;

	ret = write(dev->fd, val, count);

	return ret;
}

static int globalfifo_get_val(struct globalfifo_device_t* dev, unsigned char* val, int count) {

	int ret = 0;

	if(!val) {
		LOGE(" Error val pointer.\n");
		return -EFAULT;
	}
	ret = read(dev->fd, val, count);

	return ret;
}

struct globalfifo_device_t* globalfifo_device_init()
{
	int result;
	struct globalfifo_device_t* dev = NULL;

	dev = (struct globalfifo_device_t*)malloc(sizeof(struct globalfifo_device_t));
	if (!dev) {
		LOGE("Failed to alloc space.\n");
		goto fail;
	}
	memset(dev, 0, sizeof(struct globalfifo_device_t));

	result = open(DEVICE_NAME, O_RDWR, 0777);
	if (result < 0) {
		LOGE("can't open param_fifo %d.",result);
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

