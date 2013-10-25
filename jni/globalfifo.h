#ifndef ANDROID_GLOBALFIFO_INTERFACE_H
#define ANDROID_GLOBALFIFO_INTERFACE_H
#include <hardware/hardware.h>

__BEGIN_DECLS

/*定义模块ID*/
// #define GLOBALFIFO_HARDWARE_MODULE_ID "globalfifo"
#define TOTAL_SIZE 		0x100 		/* TODO change the size by self */

/*硬件模块结构体*/
//struct globalfifo_module_t {
//	struct hw_module_t common;
//};

/*硬件接口结构体*/
struct globalfifo_device_t {
//	struct hw_device_t common;
	int fd;
	int (*set_val)(struct globalfifo_device_t* dev, unsigned char* val, int count);
	int (*get_val)(struct globalfifo_device_t* dev, unsigned char* val, int count);
};

__END_DECLS

#endif
