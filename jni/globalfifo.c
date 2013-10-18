#define LOG_TAG "Globalfifo Stub"

#include <hardware/hardware.h>
#include <fcntl.h>
#include <errno.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

#include "globalfifo.h""

#define DEVICE_NAME "/dev/param_fifo"
#define MODULE_NAME "globalfifo"
#define MODULE_AUTHOR "liwei.cai@feixun.com.cn"

/*设备打开和关闭接口*/
static int globalfifo_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device);
static int globalfifo_device_close(struct hw_device_t* device);

/*设备访问接口*/
static int globalfifo_set_val(struct globalfifo_device_t* dev, unsigned char* val, int count);
static int globalfifo_get_val(struct globalfifo_device_t* dev, unsigned char* val, int count);

/*模块方法表*/
static struct hw_module_methods_t globalfifo_module_methods = {
	open: globalfifo_device_open
};

/*模块实例变量*/
struct globalfifo_module_t HAL_MODULE_INFO_SYM = {
	common: {
		tag: HARDWARE_MODULE_TAG,
	 	version_major: 1,
	 	version_minor: 0,
	 	id: GLOBALFIFO_HARDWARE_MODULE_ID,
	 	name: MODULE_NAME,
	 	author: MODULE_AUTHOR,
	 	methods: &globalfifo_module_methods,
	}
};

static int globalfifo_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device) {

	struct globalfifo_device_t* dev;

	if (!strcmp(name, GLOBALFIFO_HARDWARE_MODULE_ID)) {

		dev = (struct globalfifo_device_t*)malloc(sizeof(struct globalfifo_device_t));
		if (!dev) {
			ALOGE("Globalfifo Stub: failed to alloc space.\n");
			return -EFAULT;
		}
		memset(dev, 0, sizeof(struct globalfifo_device_t));

		dev->common.tag = HARDWARE_DEVICE_TAG;
		dev->common.version = 0;
		dev->common.module = (hw_module_t*)module;
		dev->common.close = globalfifo_device_close;
		dev->set_val = &globalfifo_set_val;
		dev->get_val = &globalfifo_get_val;

		if ((dev->fd = open(DEVICE_NAME, O_RDWR)) == -1) {
			ALOGE("Globalfifo Stub: failed to open /dev/param_fifo-- %s.\n", strerror(errno));
			free(dev);
			return -EFAULT;
		}
		*device = &(dev->common);
		ALOGD("Globalfifo Stub: open /dev/param_fifo successfully.\n");

	} else {
		ALOGD("open /dev/param_fifo failed.\n");
		return -EFAULT;
	}
	return 0;
}

static int globalfifo_device_close(struct hw_device_t* device) {
	struct globalfifo_device_t* globalfifo_device = (struct globalfifo_device_t*)device;

	if (globalfifo_device) {
		close(globalfifo_device->fd);
		free(globalfifo_device);
	}
	return 0;
}

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

